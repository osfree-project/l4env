#! /usr/bin/perl -W
#
# If someone feels like doing this with 'make' please feel free to do so,
# I'll take perl meanwhile and get something working.
#

use strict;

my $cross_compile_prefix = $ENV{CROSS_COMPILE} || '';
my $arch         = $ENV{OPT_ARCH}     || "x86";

my $module_path  = $ENV{SEARCHPATH}   || ".";
my $prog_objcopy = $ENV{OBJCOPY}      || "${cross_compile_prefix}objcopy";
my $prog_cc      = $ENV{CC}           || "${cross_compile_prefix}gcc";
my $prog_ld      = $ENV{LD}           || "${cross_compile_prefix}ld";
my $prog_cp      = $ENV{PROG_CP}      || "cp";
my $prog_gzip    = $ENV{PROG_GZIP}    || "gzip";
my $compress     = $ENV{OPT_COMPRESS} || 0;
my $strip        = $ENV{OPT_STRIP}    || 1;
my $flags_cc     = ($arch eq 'amd64' ? "-m64" : "");

my $make_inc_file = $ENV{MAKE_INC_FILE} || "mod.make.inc";

my $modulesfile      = $ARGV[0];
my $entryname        = $ARGV[1];

sub usage()
{
  print STDERR "$0 modulefile entry\n";
}



# extract list of modules from an entry in the module.list file
sub get_module_list($$)
{
  my ($mod_file, $title) = @_;

  open(M, $mod_file) || die "Cannot open $mod_file!";

  my @mods;
  # preseed first 3 modules
  $mods[0] = { command => 'fiasco',   cmdline => 'fiasco',   type => 'bin'};
  $mods[1] = { command => 'sigma0',   cmdline => 'sigma0',   type => 'bin'};
  $mods[2] = { command => 'roottask', cmdline => 'roottask', type => 'bin'};

  my $arglen = 200;
  my $line = 0;
  my $do_entry = 0;
  my $found_entry = 0;
  my $global = 1;
  my $modaddr_title;
  my $modaddr_global;
  while (<M>) {
    $line++;
    chomp;
    s/#.*$//;
    s/^\s*//;
    next if /^$/;

    if (/^modaddr\s+(\S+)/) {
      $modaddr_global = $1 if  $global;
      $modaddr_title  = $1 if !$global;
      next;
    }

    my ($type, $command, $args) = split /\s+/, $_, 3;
    $type = lc($type);

    if ($type =~ /^(entry|title)$/) {
      $do_entry = (lc($title) eq lc($command));
      $found_entry |= $do_entry;
      $global = 0;
      next;
    }

    next unless $do_entry;

    if ($type ne 'bin' and $type ne 'data'
        and $type ne 'bin-nostrip' and $type ne 'data-nostrip'
	and $type ne 'roottask' and $type ne 'kernel' and $type ne 'sigma0') {
      die "$line: Invalid type \"$type\"";
    }

    my $full = "$command";
    $full .= " $args" if defined $args;

    $full =~ s/"/\\"/g;
    $args =~ s/"/\\"/g if defined $args;

    if (length($full) > $arglen) {
      print "$line: \"$full\" too long...\n";
      exit 1;
    }

    # special case for roottask
    if ($type =~ /(rmgr|roottask)/i) {
      $mods[2]{cmdline}  = $command;
      $mods[2]{cmdline} .= ' '.$args if defined $args;
      next;
    } elsif (lc($type) eq 'kernel') {
      $mods[0]{cmdline}  = $command;
      $mods[0]{cmdline} .= ' '.$args if defined $args;
      next;
    } elsif (lc($type) eq 'sigma0') {
      $mods[1]{cmdline}  = $command;
      $mods[1]{cmdline} .= ' '.$args if defined $args;
      next;
    }

    push @mods, {
                  command => $command,
                  cmdline => $full,
                  type => $type,
                };

  }

  close M;

  die "Unknown entry \"$title\" in $modulesfile!" unless $found_entry;

  # construct roottask cmdline, just for convenience
  my $mod_bin;
  my $roottask_cmdline;
  for my $r (@mods) {
    if ($r->{type} =~ /^bin(-nostrip)?$/) {
      $mod_bin = $r->{command};
    } elsif ($r->{type} =~ /^data(-nostrip)?$/) {
      if ($mod_bin) {
	$roottask_cmdline .= " task modname \\\"$mod_bin\\\"";
	undef $mod_bin;
      }
      $roottask_cmdline .= " module";
    }
  }

  $mods[2]{cmdline} .= $roottask_cmdline if defined $roottask_cmdline;

  (
    mods    => [ @mods ],
    modaddr => $modaddr_title || $modaddr_global,
  );
}


# Search for a file by using module_path
# return undef if it could not be found, the complete path otherwise
sub search_module($)
{
  my $file = shift;

  foreach my $p (split(/:+/, $module_path)) {
    return "$p/$file" if -e "$p/$file";
  }

  undef;
}

# Write a string to a file, overwriting it.
# 1:    filename
# 2..n: Strings to write to the file
sub write_to_file
{
  my $file = shift;

  open(A, ">$file") || die "Cannot open $file!";
  while ($_ = shift) {
    print A;
  }
  close A;
}

sub first_word($)
{
  (split /\s+/, shift)[0]
}

# build object files from the modules
sub build_obj($$$)
{
  my ($cmdline, $modname, $no_strip) = @_;
  my $_file = first_word($cmdline);

  my $file = search_module($_file) || die "Cannot find file $_file!";

  printf STDERR "Merging image %s to %s\n", $file, $modname;
  # make sure that the file isn't already compressed
  system("$prog_gzip -dc $file > $modname.ugz 2> /dev/null");
  $file = "$modname.ugz" if !$?;
  system("$prog_objcopy -S $file $modname.obj 2> /dev/null")
    if $strip && !$no_strip;
  system("$prog_cp         $file $modname.obj")
    if $? || !$strip || $no_strip;
  my $uncompressed_size = -s "$modname.obj";
  system("$prog_gzip -9f $modname.obj && mv $modname.obj.gz $modname.obj")
    if $compress;
  write_to_file("$modname.extra.s",
      ".section .rodata.module_info               \n",
      ".align 4                                   \n",
      "_bin_${modname}_name:                      \n",
      ".ascii \"$_file\"; .byte 0                 \n",
      ".align 4                                   \n",
      ".section .module_info                      \n",
      ".long _binary_${modname}_start             \n",
      ".long ", (-s "$modname.obj"), "            \n",
      ".long $uncompressed_size                   \n",
      ".long _bin_${modname}_name                 \n",
      ($arch eq 'x86' || $arch eq 'amd64'
       ? #".section .module_data, \"a\", \@progbits   \n" # Not Xen
         ".section .module_data, \"awx\", \@progbits   \n" # Xen
       : ".section .module_data, #alloc           \n"),
      ".p2align 12                                \n",
      ".global _binary_${modname}_start           \n",
      ".global _binary_${modname}_end             \n",
      "_binary_${modname}_start:                  \n",
      ".incbin \"$modname.obj\"                   \n",
      "_binary_${modname}_end:                    \n",
      );
  system("$prog_cc $flags_cc -c -o $modname.bin $modname.extra.s");
  unlink("$modname.extra.s", "$modname.obj", "$modname.ugz");
}

sub build_mbi_modules_obj
{
  my @mods = @_;
  my $asm_string;

  # generate mbi module structures
  $asm_string .= ".align 4                         \n".
                 ".section .data.modules_mbi       \n".
                 ".global _modules_mbi_start;      \n".
                 "_modules_mbi_start:              \n";

  for (my $i = 0; $i < @mods; $i++) {
    $asm_string .= ".long 0                        \n".
                   ".long 0                        \n".
		   ".long _modules_mbi_cmdline_$i  \n".
		   ".long 0                        \n";
  }

  $asm_string .= ".global _modules_mbi_end;        \n".
                 "_modules_mbi_end:                \n";

  $asm_string .= ".section .data.cmdlines          \n";

  # generate cmdlines
  for (my $i = 0; $i < @mods; $i++) {
    $asm_string .= "_modules_mbi_cmdline_$i:                  \n".
                   ".ascii \"$mods[$i]->{cmdline}\"; .byte 0; \n";
  }

  write_to_file("mbi_modules.s", $asm_string);
  system("$prog_cc $flags_cc -c -o mbi_modules.bin mbi_modules.s");
  unlink("mbi_modules.s");

}

sub build_objects(@)
{
  my %entry = @_;
  my @mods = @{$entry{mods}};
  my $objs = "mbi_modules.bin";
  
  unlink($make_inc_file);

  # generate module-names
  for (my $i = 0; $i < @mods; $i++) {
    $mods[$i]->{modname} = sprintf "mod%02d", $i;
  }

  build_mbi_modules_obj(@mods);

  for (my $i = 0; $i < @mods; $i++) {
    build_obj($mods[$i]->{cmdline}, $mods[$i]->{modname},
	      $mods[$i]->{type} =~ /.+-nostrip$/);
    $objs .= " $mods[$i]->{modname}.bin";
  }

  my $make_inc_str = "MODULE_OBJECT_FILES += $objs\n";
  $make_inc_str   .= "MOD_ADDR            := $entry{modaddr}"
    if defined $entry{modaddr};

  write_to_file($make_inc_file, $make_inc_str);
}

# ------------------------------------------------------------------------

if (!$ARGV[1]) {
  print STDERR "Error: Invalid usage!\n";
  usage();
  exit 1;
}

my %entry = get_module_list($modulesfile, $entryname);
build_objects(%entry);

