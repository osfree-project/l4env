/*
 * \brief   Conversion tool for fnt fonts to tff fonts
 * \date    2004-08-24
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/*** DOpE INCLUDES ***/
#include "dopestd.h"
#include "fontconv.h"

/*** PROTOTYPES FROM POOL.C ***/
extern long  pool_add(char *name,void *structure);
extern void *pool_get(char *name);

/*** PROTOTYPE FROM CONV_FNT.C ***/
extern int init_conv_fnt (struct dope_services *);


/*** CONFIGURATION ***/

#define MAX_FNTSIZE 16*1024


/*** DECLARATIONS ***/

struct dope_services dope = {
	pool_get,
	pool_add,
};

static struct fontconv_services *fontconv;

static char fnt_buf[MAX_FNTSIZE];  /* buffer for reading input file */
static int  fnt_size;              /* size of input file            */

static u32  wtab[256];       /* destination buffer for width table  */
static u32  otab[256];       /* destination buffer for offset table */
static u32  img_w, img_h;    /* width and height of font image      */
static u8  *img_buf;         /* pointer to font image               */


/*** MAIN PROGRAM OF THE CONVERTER ***/
int main(int argc, char **argv) {
	int fh;

	if (argc < 3) {
		printf("Convert *.fnt font to tff font\n");
		printf(" usage: fnt2tff <in.fnt> <out.tff>\n");
		exit(0);
	}

	/* init and resolve font conversion module */
	init_conv_fnt(&dope);
	fontconv = dope.get_module("ConvertFNT 1.0");

	/* read fnt input data */
	fh = open(argv[1], O_RDWR);
	if (fh < 0) {
		printf("Error: could not open input file %s\n", argv[1]);
		exit(1);
	}
	fnt_size = read(fh, &fnt_buf, MAX_FNTSIZE);
	close(fh);
	printf("read fnt file (%d bytes)\n", fnt_size);

	/* convert fnt data */
	if (!fontconv->probe(fnt_buf)) {
		printf("Error: input file is not a valid fnt font\n");
		exit(1);
	}
	printf("converting font '%s'\n", fontconv->get_name(fnt_buf));
	fontconv->gen_width_table (fnt_buf, wtab);
	fontconv->gen_offset_table(fnt_buf, otab);
	img_w = fontconv->get_image_width(fnt_buf);
	img_h = fontconv->get_image_height(fnt_buf);
	img_buf = malloc(img_w*img_h);

	if (!img_buf) {
		printf("Error: out of memory - unable to allocate font image\n");
		exit(1);
	}
	fontconv->gen_image(fnt_buf, img_buf);
	printf("font image is %d bytes\n", (int)(img_w*img_h));

	/* write output file */
	fh = creat(argv[2], S_IRWXU);
	if (fh < 0) {
		printf("Error %d: could not create output file %s\n", fh, argv[1]);
		exit(1);
	}
	if ((write(fh,  otab,    sizeof(otab))  < 0)
	 || (write(fh,  wtab,    sizeof(otab))  < 0)
	 || (write(fh, &img_w,   sizeof(img_w)) < 0)
	 || (write(fh, &img_h,   sizeof(img_h)) < 0)
	 || (write(fh,  img_buf, img_w*img_h)   < 0)) {
		printf("Error: could not write output file\n");
		exit(1);
	}
	close(fh);

	return 0;
}
