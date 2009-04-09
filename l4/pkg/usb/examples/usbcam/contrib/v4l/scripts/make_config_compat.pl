#!/usr/bin/perl
use strict;

my $kdir=shift or die "should specify a kernel dir";
my $infile=shift or die "should specify an input config file";
my $outfile=shift or die "should specify an output config file";

my $out;

sub check_spin_lock()
{
	my $file = "$kdir/include/linux/netdevice.h";
	my $old_syntax = 1;

	open INNET, "<$file" or die "File not found: $file";
	while (<INNET>) {
		if (m/netif_tx_lock_bh/) {
			$old_syntax = 0;
			last;
		}
	}

	if ($old_syntax) {
		$out.= "\n#define OLD_XMIT_LOCK 1\n";
	}
	close INNET;
}

sub check_sound_driver_h()
{
	my $file = "$kdir/include/sound/driver.h";
	my $old_syntax = 1;

	open INNET, "<$file" or return;
	while (<INNET>) {
		if (m/This file is deprecated/) {
			$old_syntax = 0;
			last;
		}
	}

	if ($old_syntax) {
		$out.= "\n#define NEED_SOUND_DRIVER_H 1\n";
	}
	close INNET;
}

sub check_snd_pcm_rate_to_rate_bit()
{
	my $file = "$kdir/include/sound/pcm.h";
	my $old_syntax = 1;

	open INNET, "<$file" or die "File not found: $file";
	while (<INNET>) {
		if (m/snd_pcm_rate_to_rate_bit/) {
			$old_syntax = 0;
			last;
		}
	}

	if ($old_syntax) {
		$out.= "\n#define COMPAT_PCM_TO_RATE_BIT 1\n";
	}
	close INNET;
}

sub check_snd_ctl_boolean_mono_info()
{
	my $file = "$kdir/include/sound/control.h";
	my $old_syntax = 1;

	open INNET, "<$file" or die "File not found: $file";
	while (<INNET>) {
		if (m/snd_ctl_boolean_mono_info/) {
			$old_syntax = 0;
			last;
		}
	}

	if ($old_syntax) {
		$out.= "\n#define COMPAT_SND_CTL_BOOLEAN_MONO 1\n";
	}
	close INNET;
}

sub check_bool()
{
	my $file = "$kdir/include/linux/types.h";
	my $old_syntax = 1;

	open INDEP, "<$file" or die "File not found: $file";
	while (<INDEP>) {
		if (m/^\s*typedef.*bool;/) {
			$old_syntax = 0;
			last;
		}
	}

	if ($old_syntax) {
		$out.= "\n#define NEED_BOOL_TYPE 1\n";
	}
	close INDEP;
}

sub check_is_singular()
{
	my $file = "$kdir/include/linux/list.h";
	my $need_compat = 1;

	open INNET, "<$file" or die "File not found: $file";
	while (<INNET>) {
		if (m/list_is_singular/) {
			$need_compat = 0;
			last;
		}
	}

	if ($need_compat) {
		$out.= "\n#define NEED_IS_SINGULAR 1\n";
	}
	close INNET;
}

sub check_clamp()
{
	my $file = "$kdir/include/linux/kernel.h";
	my $need_compat = 1;

	open INNET, "<$file" or die "File not found: $file";
	while (<INNET>) {
		if (m/define\s+clamp/) {
			$need_compat = 0;
			last;
		}
	}

	if ($need_compat) {
		$out.= "\n#define NEED_CLAMP 1\n";
	}
	close INNET;
}

sub check_proc_create()
{
	my $file = "$kdir/include/linux/proc_fs.h";
	my $need_compat = 1;

	open INNET, "<$file" or die "File not found: $file";
	while (<INNET>) {
		if (m/proc_create/) {
			$need_compat = 0;
			last;
		}
	}

	if ($need_compat) {
		$out.= "\n#define NEED_PROC_CREATE 1\n";
	}
	close INNET;
}

sub check_pcm_lock()
{
	my $file = "$kdir/include/sound/pcm.h";
	my $need_compat = 1;

	open INNET, "<$file" or die "File not found: $file";
	while (<INNET>) {
		if (m/pcm_stream_lock/) {
			$need_compat = 0;
			last;
		}
	}

	if ($need_compat) {
		$out.= "\n#define NO_PCM_LOCK 1\n";
	}
	close INNET;
}

sub check_algo_control()
{
	my $file = "$kdir/include/linux/i2c.h";
	my $need_compat = 0;

	open INNET, "<$file" or die "File not found: $file";
	while (<INNET>) {
		if (m/algo_control/) {
			$need_compat = 1;
			last;
		}
	}

	if ($need_compat) {
		$out.= "\n#define NEED_ALGO_CONTROL 1\n";
	}
	close INNET;
}

sub check_net_dev()
{
	my $file = "$kdir/include/linux/netdevice.h";
	my $need_compat = 1;

	open INNET, "<$file" or die "File not found: $file";
	while (<INNET>) {
		if (m/netdev_priv/) {
			$need_compat = 0;
			last;
		}
	}

	if ($need_compat) {
		$out.= "\n#define NEED_NETDEV_PRIV 1\n";
	}
	close INNET;
}

sub check_usb_endpoint_type()
{
	my $file = "$kdir/include/linux/usb.h";
	my $need_compat = 1;

	open INNET, "<$file" or die "File not found: $file";
	while (<INNET>) {
		if (m/usb_endpoint_type/) {
			$need_compat = 0;
			last;
		}
	}

	if ($need_compat) {
		$out.= "\n#define NEED_USB_ENDPOINT_TYPE 1\n";
	}
	close INNET;
}

sub check_pci_ioremap_bar()
{
	my $file = "$kdir/include/linux/pci.h";
	my $need_compat = 1;

	open INNET, "<$file" or die "File not found: $file";
	while (<INNET>) {
		if (m/pci_ioremap_bar/) {
			$need_compat = 0;
			last;
		}
	}

	if ($need_compat) {
		$out.= "\n#define NEED_PCI_IOREMAP_BAR 1\n";
	}
	close INNET;
}

sub check_other_dependencies()
{
	check_spin_lock();
	check_sound_driver_h();
	check_snd_ctl_boolean_mono_info();
	check_snd_pcm_rate_to_rate_bit();
	check_bool();
	check_is_singular();
	check_clamp();
	check_proc_create();
	check_pcm_lock();
	check_algo_control();
	check_net_dev();
	check_usb_endpoint_type();
	check_pci_ioremap_bar();
}

# Do the basic rules
open IN, "<$infile" or die "File not found: $infile";

$out.= "#ifndef __CONFIG_COMPAT_H__\n";
$out.= "#define __CONFIG_COMPAT_H__\n\n";
$out.= "#include <linux/autoconf.h>\n\n";

# mmdebug.h includes autoconf.h. So if this header exists,
# then include it before our config is set.
if (-f "$kdir/include/linux/mmdebug.h") {
	$out.= "#include <linux/mmdebug.h>\n\n";
}

while(<IN>) {
	next unless /^(\S+)\s*:= (\S+)$/;
	$out.= "#undef $1\n";
	$out.= "#undef $1_MODULE\n";
	if($2 eq "n") {
		next;
	} elsif($2 eq "m") {
		$out.= "#define $1_MODULE 1\n";
	} elsif($2 eq "y") {
		$out.= "#define $1 1\n";
	} else {
		$out.= "#define $1 $2\n";
	}
}
close IN;

check_other_dependencies();

open OUT, ">$outfile" or die 'Unable to write $outfile';
print OUT "$out\n#endif\n";
close OUT
