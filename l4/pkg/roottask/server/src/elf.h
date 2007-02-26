/* $Id$ */
/**
 * \file	exec/include/l4/exec/elf.h
 * \brief	ELF header definition
 *
 * \date	08/18/2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * Many structs from
 *   "Executable and Linkable Format (ELF)",
 *    Portable Formats Specification, Version 1.1
 * and
 *   "System V Application Binary Interface - DRAFT - April 29, 1998"
 *   The Santa Cruz Operation, Inc.
 *   (see http://www.sco.com/developer/gabi/contents.html) */

#ifndef _L4_EXEC_ELF_H
#define _L4_EXEC_ELF_H

#include <l4/sys/l4int.h>

/* map ELF types to L4 types */

typedef l4_uint32_t	Elf32_Addr;		/* size 4 align 4 */
typedef l4_uint32_t	Elf32_Off;		/* size 4 align 4 */
typedef l4_uint16_t	Elf32_Half;		/* size 2 align 2 */
typedef l4_uint32_t	Elf32_Word;		/* size 4 align 4 */
typedef l4_int32_t	Elf32_Sword;		/* size 4 align 4 */

typedef l4_uint64_t	Elf64_Addr;		/* size 8 align 8 */
typedef	l4_uint64_t	Elf64_Off;		/* size 8 align 8 */
typedef l4_uint16_t	Elf64_Half;		/* size 2 align 2 */
typedef	l4_uint32_t	Elf64_Word;		/* size 4 align 4 */
typedef	l4_int32_t	Elf64_Sword;		/* size 4 align 4 */
typedef	l4_uint64_t	Elf64_Xword;		/* size 8 align 8 */
typedef	l4_int64_t	Elf64_Sxword;		/* size 8 align 8 */


/*************************************/
/* ELF Header - figure 1-3, page 1-3 */
/*************************************/

#define EI_NIDENT 16
typedef struct {
    unsigned char	e_ident[EI_NIDENT];
    Elf32_Half		e_type;		/* type of ELF file */
    Elf32_Half		e_machine;	/* required architecture */
    Elf32_Word		e_version;	/* file version */
    Elf32_Addr		e_entry;	/* initial eip */
    Elf32_Off		e_phoff;	/* offset of program header table */
    Elf32_Off		e_shoff;	/* offset of file header table */
    Elf32_Word		e_flags;	/* processor-specific flags */
    Elf32_Half		e_ehsize;	/* size of ELF header */
    Elf32_Half		e_phentsize;	/* size of program header entry */
    Elf32_Half		e_phnum;	/* # of entries in progr. head. tab. */
    Elf32_Half		e_shentsize;	/* size of section header entry */
    Elf32_Half		e_shnum;	/* # of entries in sect. head. tab. */
    Elf32_Half		e_shstrndx;	/* sect.head.tab.idx of strtab */
} Elf32_Ehdr;

typedef struct {
    unsigned char	e_ident[EI_NIDENT];
    Elf64_Half		e_type;		/* type of ELF file */
    Elf64_Half		e_machine;	/* required architecture */
    Elf64_Word		e_version;	/* file version */
    Elf64_Addr		e_entry;	/* initial eip */
    Elf64_Off		e_phoff;	/* offset of program header table */
    Elf64_Off		e_shoff;	/* offset of file header table */
    Elf64_Word		e_flags;	/* processor-specific flags */
    Elf64_Half		e_ehsize;	/* size of ELF header */
    Elf64_Half		e_phentsize;	/* size of program header entry */
    Elf64_Half		e_phnum;	/* # of entries in progr. head. tab. */
    Elf64_Half		e_shentsize;	/* size of section header entry */
    Elf64_Half		e_shnum;	/* # of entries in sect. head. tab. */
    Elf64_Half		e_shstrndx;	/* sect.head.tab.idx of strtab */
} Elf64_Ehdr;

/* object file type - page 1-3 (e_type) */

#define ET_NONE		0	/* no file type */
#define ET_REL		1	/* relocatable file */
#define ET_EXEC		2	/* executable file */
#define ET_DYN		3	/* shared object file */
#define ET_CORE		4	/* core file */
#define ET_LOPROC	0xff00	/* processor-specific */
#define ET_HIPROC	0xffff	/* processor-specific */

/* required architecture - page 1-4 (e_machine) */

#define EM_NONE		0	/* no machine */
#define EM_M32		1	/* AT&T WE 32100 */
#define EM_SPARC	2	/* SPARC */
#define EM_386		3	/* Intel 80386 */
#define EM_68K		4	/* Motorola 68000 */
#define EM_88K		5	/* Motorola 88000 */
#define EM_860		7	/* Intel 80860 */
#define EM_MIPS		8	/* MIPS RS3000 big-endian */
#define EM_MIPS_RS4_BE	10	/* MIPS RS4000 big-endian */
#define EM_SPARC64	11	/* SPARC 64-bit */
#define EM_PARISC	15	/* HP PA-RISC */
#define EM_VPP500	17	/* Fujitsu VPP500 */
#define EM_SPARC32PLUS	18	/* Sun's V8plus */
#define EM_960		19	/* Intel 80960 */
#define EM_PPC		20	/* PowerPC */
#define EM_V800		36	/* NEC V800 */
#define EM_FR20		37	/* Fujitsu FR20 */
#define EM_RH32		38	/* TRW RH-32 */
#define RM_RCE		39	/* Motorola RCE */
#define EM_ARM		40	/* Advanced RISC Machines ARM */
#define EM_ALPHA	41	/* Digital Alpha */
#define EM_SH		42	/* Hitachi SuperH */
#define EM_SPARCV9	43	/* SPARC v9 64-bit */
#define EM_TRICORE	44	/* Siemens Tricore embedded processor */
#define EM_ARC		45	/* Argonaut RISC Core, Argonaut Techn. Inc. */
#define EM_H8_300	46	/* Hitachi H8/300 */
#define EM_H8_300H	47	/* Hitachi H8/300H */
#define EM_H8S		48	/* Hitachi H8/S */
#define EM_H8_500	49	/* Hitachi H8/500 */
#define EM_IA_64	50	/* HP/Intel IA-64 */
#define EM_MIPS_X	51	/* Stanford MIPS-X */
#define EM_COLDFIRE	52	/* Motorola Coldfire */
#define EM_68HC12	53	/* Motorola M68HC12 */

#if 0
#define EM_ALPHA	0x9026	/* interium value used by Linux until the
				   committee comes up with a final number */
#define EM_S390		0xA390	/* interium value used for IBM S390 */
#endif

/* object file version - page 1-4 (e_version) */

#define EV_NONE		0	/* Invalid version */
#define EV_CURRENT	1	/* Current version */

/* e_ident[] Identification Indexes - figure 1-4, page 1-5 */

#define EI_MAG0		0	/* file id */
#define EI_MAG1		1	/* file id */
#define EI_MAG2		2	/* file id */
#define EI_MAG3		3	/* file id */
#define EI_CLASS	4	/* file class */
#define EI_DATA		5	/* data encoding */
#define EI_VERSION	6	/* file version */
#define EI_OSABI	7	/* Operating system / ABI identification */
#define EI_ABIVERSION	8	/* ABI version */
#define EI_PAD		9	/* start of padding bytes */
#define EI_NIDENT	16	/* size of e_ident[] */

/* magic number - page 1-5 */

#define ELFMAG0		0x7f	/* e_ident[EI_MAG0] */
#define ELFMAG1		'E'	/* e_ident[EI_MAG1] */
#define ELFMAG2		'L'	/* e_ident[EI_MAG2] */
#define ELFMAG3		'F'	/* e_ident[EI_MAG3] */

/* file class or capacity - page 1-6 */

#define ELFCLASSNONE	0	/* Invalid class */
#define ELFCLASSS32	1	/* 32-bit object */
#define ELFCLASSS64	2	/* 64-bit object */

/* data encoding - page 1-6 */

#define ELFDATANONE	0	/* invalid data encoding */
#define ELFDATA2LSB	1	/* 0x01020304 => [ 0x04|0x03|0x02|0x01 ] */
#define ELFDATA2MSB	2	/* 0x01020304 => [ 0x01|0x02|0x03|0x04 ] */

/* Identify operating system and ABI to which the object is targeted */

#define ELFOSABI_SYSV	0	/* UNIX System V ABI (this specification) */
#define ELFOSABI_HPUX	1	/* HP-UX operating system */
#define ELFOSABI_STANDALONE 255  /* Standalone (embedded) application */


/***********************/
/* Sections - page 1-8 */
/***********************/

/* special section indexes */

#define SHN_UNDEF	0
#define SHN_LORESERVE	0xff00
#define SHN_LOPROC	0xff00
#define SHN_HIPROC	0xff1f
#define SHN_ABS		0xfff1
#define SHN_COMMON	0xfff2
#define SHN_HIRESERVE	0xffff

/* section header - figure 1-9, page 1-9 */

typedef struct {
    Elf32_Word		sh_name;	/* name of section (idx into strtab) */
    Elf32_Word		sh_type;	/* section's type */
    Elf32_Word		sh_flags;	/* section's flags */
    Elf32_Addr		sh_addr;	/* memory address of section */
    Elf32_Off		sh_offset;	/* file offset of section */
    Elf32_Word		sh_size;	/* file size of section */
    Elf32_Word		sh_link;	/* idx to associated header section */
    Elf32_Word		sh_info;	/* extra info of header section */
    Elf32_Word		sh_addralign;	/* address alignment constraints */
    Elf32_Word		sh_entsize;	/* size of entry if section is table */
} Elf32_Shdr;

typedef struct {
    Elf64_Word		sh_name;	/* name of section (idx into strtab) */
    Elf64_Word		sh_type;	/* section's type */
    Elf64_Xword		sh_flags;	/* section's flags */
    Elf64_Addr		sh_addr;	/* memory address of section */
    Elf64_Off		sh_offset;	/* file offset of section */
    Elf64_Xword		sh_size;	/* file size of section */
    Elf64_Word		sh_link;	/* idx to associated header section */
    Elf64_Word		sh_info;	/* extra info of header section */
    Elf64_Xword		sh_addralign;	/* address alignment constraints */
    Elf64_Xword		sh_entsize;	/* size of entry if section is table */
} Elf64_Shdr;

/* section type - figure 1-10, page 1-10 */

#define SHT_NULL	0
#define SHT_PROGBITS	1
#define SHT_SYMTAB	2
#define SHT_STRTAB	3
#define SHT_RELA	4
#define SHT_HASH	5
#define SHT_DYNAMIC	6
#define SHT_NOTE	7
#define SHT_NOBITS	8
#define SHT_REL		9
#define SHT_SHLIB	10
#define SHT_DYNSYM	11
#define SHT_LOOS	0x60000000
#define SHT_HIOS	0x6fffffff
#define SHT_LOPROC	0x70000000
#define SHT_HIPROC	0x7fffffff
#define SHT_LOUSER	0x80000000
#define SHT_HIUSER	0xffffffff

/* section attribute flags - page 1-12, figure 1-12 */

#define SHF_WRITE	0x1		/* writeable during execution */
#define SHF_ALLOC	0x2		/* section occupies virt. memory */
#define SHF_EXECINSTR	0x4		/* code section */
#define SHF_MASKPROC	0xf0000000	/* proc. spec. mask */


/*****************************************/
/* Program Header - figure 2-1, page 2-2 */
/*****************************************/

typedef struct {
    Elf32_Word		p_type;		/* type of program section */
    Elf32_Off		p_offset;	/* file offset of program section */
    Elf32_Addr		p_vaddr;	/* memory address of program section */
    Elf32_Addr		p_paddr;	/* physical address (ignored) */
    Elf32_Word		p_filesz;	/* file size of program section */
    Elf32_Word		p_memsz;	/* memory size of program section */
    Elf32_Word		p_flags;	/* flags */
    Elf32_Word		p_align;	/* alignment of section */
} Elf32_Phdr;

typedef struct {
    Elf64_Word		p_type;		/* type of program section */
    Elf64_Word		p_flags;	/* flags */
    Elf64_Off		p_offset;	/* file offset of program section */
    Elf64_Addr		p_vaddr;	/* memory address of program section */
    Elf64_Addr		p_paddr;	/* physical address (ignored) */
    Elf64_Xword		p_filesz;	/* file size of program section */
    Elf64_Xword		p_memsz;	/* memory size of program section */
    Elf64_Xword		p_align;	/* alignment of section */
} Elf64_Phdr;

/* segment types - figure 2-2, page 2-3 */

#define PT_NULL		0	/* array is unused */
#define PT_LOAD		1	/* loadable */
#define PT_DYNAMIC	2	/* dynamic linking information */
#define PT_INTERP	3	/* path to interpreter */
#define PT_NOTE		4	/* auxiliary information */
#define PT_SHLIB	5	/* reserved */
#define PT_PHDR		6	/* location of the pht itself */
#define PT_LOOS		0x60000000 /* os spec. */
#define PT_HIOS		0x6fffffff /* os spec. */
#define PT_LOPROC	0x70000000 /* processor spec. */
#define PT_HIPROC	0x7fffffff /* processor spec. */

/* segment permissions - page 2-3 */

#define PF_X		0x1
#define PF_W		0x2
#define PF_R		0x4
#define PF_MASKOS	0x0ff00000
#define PF_MASKPROC	0x7fffffff

/* Dynamic structure - figure 2-9, page 2-12 */

typedef struct {
    Elf32_Sword		d_tag;
    union {
	Elf32_Word	d_val;	/* integer values with various interpret. */
	Elf32_Addr	d_ptr;	/* program virtual addresses */
    } d_un;
} Elf32_Dyn;

typedef struct {
    Elf64_Sxword	d_tag;
    union {
	Elf64_Xword	d_val;	/* integer values with various interpret. */
	Elf64_Addr	d_ptr;	/* program virtual addresses */
    } d_un;
} Elf64_Dyn;

/* Dynamic Array Tags, d_tag - figure 2-10, page 2-12 */

#define DT_NULL		0	/* end of _DYNAMIC array */
#define DT_NEEDED	1	/* name of a needed library */
#define DT_PLTRELSZ	2	/* total size of relocation entry */
#define DT_PLTGOT	3	/* address assoc. with pro. link. table */
#define DT_HASH		4	/* address of symbol hash table */
#define DT_STRTAB	5	/* address of string table */
#define DT_SYMTAB	6	/* address of symbol table */
#define DT_RELA		7	/* address of relocation table */
#define DT_RELASZ	8	/* total size of relocation table */
#define DT_RELAENT	9	/* size of DT_RELA relocation entry */
#define DT_STRSZ	10	/* size of the string table */
#define DT_SYMENT	11	/* size of a symbol table entry */
#define DT_INIT		12	/* address of initialization function */
#define DT_FINI		13	/* address of termination function */
#define DT_SONAME	14	/* name of the shared object */
#define DT_RPATH	15	/* search library path */
#define DT_SYMBOLIC	16	/* alter symbol resolution algorithm */
#define DT_REL		17	/* address of relocation table */
#define DT_RELSZ	18	/* total size of DT_REL relocation table */
#define DT_RELENT	19	/* size of the DT_REL relocation entry */
#define DT_PTRREL	20	/* type of relocation entry */
#define DT_DEBUG	21	/* for debugging purposes */
#define DT_TEXTREL	22	/* at least on entry changes r/o section */
#define DT_JMPREL	23	/* address of relocation entries */
#define DT_LOPROC	0x70000000 /* processor spec. */
#define DT_HIPROC	0x7fffffff /* processor spec. */

/* Relocation - page 1-21, figure 1-20 */

typedef struct {
    Elf32_Addr		r_offset;
    Elf32_Word		r_info;
} Elf32_Rel;

typedef struct {
    Elf32_Addr		r_offset;
    Elf32_Word		r_info;
    Elf32_Sword		r_addend;
} Elf32_Rela;

typedef struct {
    Elf64_Addr		r_offset;
    Elf64_Xword		r_info;
} Elf64_Rel;

typedef struct {
    Elf64_Addr		r_offset;
    Elf64_Xword		r_info;
    Elf64_Sxword	r_addend;
} Elf64_Rela;

#define ELF32_R_SYM(i)	  ((i)>>8)
#define ELF32_R_TYPE(i)	  ((unsigned char)(i))
#define ELF32_R_INFO(s,t) (((s)<<8)+(unsigned char)(t))

#define ELF64_R_SYM(i)	  ((i)>>32)
#define ELF64_R_TYPE(i)	  ((i)&0xffffffffL)
#define ELF64_R_INFO(s,t) (((s)<<32)+(t)&0xffffffffL)

/* Relocation types (processor specific) - page 1-23, figure 1-22 */

#define R_386_NONE	0	/* none */
#define R_386_32	1	/* S + A */
#define R_386_PC32	2	/* S + A - P */
#define R_386_GOT32	3	/* G + A - P */
#define R_386_PLT32	4	/* L + A - P */
#define R_386_COPY	5	/* none */
#define R_386_GLOB_DAT	6	/* S */
#define R_386_JMP_SLOT	7	/* S */
#define R_386_RELATIVE	8	/* B + A */
#define R_386_GOTOFF	9	/* S + A - GOT */
#define R_386_GOTPC	10	/* GOT + A - P */

/* Symbol Table Entry - page 1-17, figure 1-16 */

#define STN_UNDEF	0

typedef struct {
    Elf32_Word		st_name;	/* name of symbol (idx symstrtab) */
    Elf32_Addr		st_value;	/* value of associated symbol */
    Elf32_Word		st_size;	/* size of associated symbol */
    unsigned char	st_info;	/* type and binding info */
    unsigned char	st_other;	/* undefined */
    Elf32_Half		st_shndx;	/* associated section header */
} Elf32_Sym;

typedef struct {
    Elf64_Word		st_name;	/* name of symbol (idx symstrtab) */
    unsigned char	st_info;	/* type and binding info */
    unsigned char	st_other;	/* undefined */
    Elf64_Half		st_shndx;	/* associated section header */
    Elf64_Addr		st_value;	/* value of associated symbol */
    Elf64_Xword		st_size;	/* size of associated symbol */
} Elf64_Sym;

#define ELF32_ST_BIND(i)    ((i)>>4)
#define ELF32_ST_TYPE(i)    ((i)&0xf)
#define ELF32_ST_INFO(b,t)  (((b)<<4)+((t)&0xf))

#define ELF64_ST_BIND(i)    ((i)>>4)
#define ELF64_ST_TYPE(i)    ((i)&0xf)
#define ELF64_ST_INFO(b,t)  (((b)<<4)+((t)&0xf))

/* Symbol Binding - page 1-18, figure 1-17 */

#define STB_LOCAL	0	/* not visible outside object file */
#define STB_GLOBAL	1	/* visible to all objects beeing combined */
#define STB_WEAK	2	/* resemble global symbols */
#define STB_LOOS	10	/* os specific */
#define STB_HIOS	12	/* os specific */
#define STB_LOPROC	13	/* proc. specific */
#define STB_HIPROC	15	/* proc. specific */

/* Symbol Types - page 1-19, figure 1-18 */

#define STT_NOTYPE	0	/* symbol's type not specified */
#define STT_OBJECT	1	/* associated with a data object */
#define STT_FUNC	2	/* associated with a function or other code */
#define STT_SECTION	3	/* associated with a section */
#define STT_FILE	4	/* source file name associated with object */
#define STT_LOOS	10	/* os specific */
#define STT_HIOS	12	/* os specific */
#define STT_LOPROC	13	/* proc. specific */
#define STT_HIPROC	15	/* proc. specific */

#endif /* _L4_EXEC_ELF_H */

