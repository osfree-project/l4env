/**
 * \file	roottask/server/src/lines.h
 * \brief	extract lines information from ELF binary
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#ifndef LINES_H
#define LINES_H

#include <l4/util/mb_info.h>

void extract_lines_from_image(l4_addr_t elf_image, unsigned task_no,
			      l4_addr_t *from_lin, l4_addr_t *to_lin);
void extract_lines_from_mbinfo(l4util_mb_info_t *mbi, unsigned task_no,
			       l4_addr_t *from_lin, l4_addr_t *to_lin);

#endif
