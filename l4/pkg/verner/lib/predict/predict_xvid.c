#define COMPILE_XVID \
  gcc -O3 -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wno-long-long -ansi -pedantic -DTESTING -DTEST_XVID_MAIN -lm -o predict_xvid $0 ; exit
/* ^^^ This file can compile itself on Linux for testing. ^^^ */

/*
 * Copyright (C) 2005 Michael Roitzsch <mroi@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifdef TESTING
/* we want strdup() */
#  define _XOPEN_SOURCE
#  define _XOPEN_SOURCE_EXTENDED
#else
#  include <l4/log/l4log.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "predict.h"

/* the XviD code needs to be parsed first, but I prefer to keep it at the end */
#ifndef XVID_CODE_INCLUDED
#define XVID_CODE_INCLUDED
#include "predict_xvid.c"

#define XVID_METRICS_COUNT 7
#define XVID_CONTEXT_COUNT 5


/* internal structures */
struct predictor_s {
  char *learn_file;
  char *predict_file;

  /* XviD library context */
  void *xvid;

  /* the XviD metrics */
  double metrics[XVID_METRICS_COUNT];
  int frame_type;
  char valid;

  /* the llsp contexts for the frametypes */
  llsp_t *llsp[XVID_CONTEXT_COUNT];
};

enum {
  METRIC_XVID_FRAME_BYTES, METRIC_XVID_FRAME_PIXELS, METRIC_XVID_FRAME_PIXELS_SQRT,
  METRIC_XVID_MACRO_INTRA, METRIC_XVID_MACRO_INTER,
  METRIC_XVID_MOTION, METRIC_XVID_MOTION_GLOBAL
};

/* helper functions */
#ifdef TESTING
#  include "llsp_solver.c"
#  undef LOG
#  define LOG(format, arg) printf(format "\n", arg)
#endif


predictor_t *predict_xvid_new(const char *learn_file, const char *predict_file)
{
  predictor_t *predictor;
  xvid_gbl_init_t   xvid_init;
  xvid_dec_create_t xvid_create;
  int i;

  predictor = (predictor_t *)malloc(sizeof(predictor_t));
  if (!predictor) return NULL;
  memset(predictor, 0, sizeof(predictor));

  predictor->learn_file   = (learn_file   && learn_file[0])   ? strdup(learn_file)   : NULL;
  predictor->predict_file = (predict_file && predict_file[0]) ? strdup(predict_file) : NULL;
  
  if (!predictor->learn_file && !predictor->predict_file) goto fail;

  /* initialize our stripped down XviD library */
  memset(&xvid_init, 0, sizeof(xvid_init));
  xvid_init.version = XVID_VERSION;
  xvid_global(NULL, 0, &xvid_init, NULL);
  memset(&xvid_create, 0, sizeof(xvid_create));
  xvid_create.version = XVID_VERSION;
  if (xvid_decore(NULL, XVID_DEC_CREATE, &xvid_create, NULL) == 0)
    predictor->xvid = xvid_create.handle;
  else
    goto fail;

  for (i = 0; i < sizeof(predictor->llsp) / sizeof(predictor->llsp[0]); i++) {
    predictor->llsp[i] = llsp_new(PREDICT_XVID_BASE + i, XVID_METRICS_COUNT);
    if (!predictor->llsp[i]) {
      for (i--; i >= 0; i--) llsp_dispose(predictor->llsp[i]);
      goto fail;
    }
    if (predictor->predict_file)
      llsp_load(predictor->llsp[i], predictor->predict_file);
  }

  return predictor;

fail:
  free(predictor->learn_file);
  free(predictor->predict_file);
  free(predictor);
  return NULL;
}

double predict_xvid(predictor_t *predictor, unsigned char **data, unsigned *length)
{
  xvid_dec_frame_t xvid_frame;
  xvid_dec_stats_t xvid_stats;
  int used_bytes, frame_count;
  double time = 0.0;

  if (!predictor->learn_file && !predictor->predict_file) return -1.0;
  if (!data || !*data || !length) return -1.0;

  predictor->valid = 1;
  frame_count = 0;

  do {
    memset(&xvid_frame, 0, sizeof(xvid_frame));
    xvid_frame.version = XVID_VERSION;
    xvid_frame.bitstream = *data;
    xvid_frame.length = *length;
    xvid_stats.version = XVID_VERSION;

    used_bytes = xvid_decore(predictor->xvid, XVID_DEC_DECODE, &xvid_frame, &xvid_stats);

    if (xvid_stats.coding_type > 0) {
      /* we "decoded" one frame */
      frame_count++;

      predictor->metrics[METRIC_XVID_FRAME_BYTES]       = used_bytes;
      predictor->metrics[METRIC_XVID_MACRO_INTRA]       = xvid_stats.data.vop.macro_intra;
      predictor->metrics[METRIC_XVID_MACRO_INTER]       = xvid_stats.data.vop.macro_inter;
      predictor->metrics[METRIC_XVID_MOTION]            = xvid_stats.data.vop.motion;
      predictor->metrics[METRIC_XVID_MOTION_GLOBAL]     = xvid_stats.data.vop.motion_global;

      switch (xvid_stats.coding_type) {
      case XVID_TYPE_IVOP:
	predictor->frame_type = PREDICT_XVID_I;
	break;
      case XVID_TYPE_PVOP:
	predictor->frame_type = PREDICT_XVID_P;
	break;
      case XVID_TYPE_BVOP:
	predictor->frame_type = PREDICT_XVID_B;
	break;
      case XVID_TYPE_SVOP:
	predictor->frame_type = PREDICT_XVID_S;
	break;
      case XVID_TYPE_SVOP + 1:  /* why is there no define for N_VOPs (uncoded VOPs)? */
	predictor->frame_type = PREDICT_XVID_N;
	break;
      default:
	predictor->valid = 0;
      }

      if (predictor->predict_file && predictor->valid)
	time += llsp_predict(predictor->llsp[predictor->frame_type - PREDICT_XVID_BASE], predictor->metrics);
      else
	time = -1.0;

    } else if (xvid_stats.type == XVID_TYPE_VOL) {
      /* we got a resize hint */
      predictor->metrics[METRIC_XVID_FRAME_PIXELS] = xvid_stats.data.vol.width * xvid_stats.data.vol.height;
      predictor->metrics[METRIC_XVID_FRAME_PIXELS_SQRT] = sqrt(predictor->metrics[METRIC_XVID_FRAME_PIXELS]);
    }

    if (used_bytes < 0 || used_bytes > *length) {
      /* fatal error during stream parsing */
      predictor->valid = 0;
      return -1.0;
    }
    if (used_bytes > 0) {
      *data += used_bytes / sizeof((*data)[0]);
      *length -= used_bytes;
    }
  } while (xvid_stats.type == XVID_TYPE_VOL && *length > 0);
  
  /* we can only use this predictor for further learning if only one frame was handled */
  predictor->valid = (frame_count == 1);
  
  return time;
}

void predict_xvid_learn(predictor_t *predictor, double time)
{
  if (predictor->learn_file && predictor->valid)
    llsp_accumulate(predictor->llsp[predictor->frame_type - PREDICT_XVID_BASE], predictor->metrics, time);
}

void predict_xvid_eval(predictor_t *predictor)
{
  int i;
  
  if (predictor->learn_file) {
    for (i = 0; i < sizeof(predictor->llsp) / sizeof(predictor->llsp[0]); i++) {
      if (!llsp_finalize(predictor->llsp[i]))
	LOG("finalization error on llsp %d", i);
      else
	llsp_store(predictor->llsp[i], predictor->learn_file);
    }
  }
}

void predict_xvid_discontinue(predictor_t *predictor)
{
  xvid_dec_create_t xvid_create;
  
  /* destroy ... */
  xvid_decore(predictor->xvid, XVID_DEC_DESTROY, NULL, NULL);
  /* ... and recreate our internal XviD decoder */
  memset(&xvid_create, 0, sizeof(xvid_create));
  xvid_create.version = XVID_VERSION;
  if (xvid_decore(NULL, XVID_DEC_CREATE, &xvid_create, NULL) == 0)
    predictor->xvid = xvid_create.handle;
  else {
    /* make the predictor unusable */
    free(predictor->learn_file);
    free(predictor->predict_file);
    predictor->learn_file   = NULL;
    predictor->predict_file = NULL;
    predictor->xvid         = NULL;
    predictor->valid        = 0;
  }
}

void predict_xvid_dispose(predictor_t *predictor)
{
  int i;
  
  for (i = 0; i < sizeof(predictor->llsp) / sizeof(predictor->llsp[0]); i++)
    llsp_dispose(predictor->llsp[i]);
  xvid_decore(predictor->xvid, XVID_DEC_DESTROY, NULL, NULL);
  free(predictor->learn_file);
  free(predictor->predict_file);
  free(predictor);
}


#ifdef TEST_XVID_MAIN
int main(void)
{
  predictor_t *predictor;
  unsigned char mem[2 * 1024 * 1024];
  unsigned char *buf = mem;
  unsigned length = 0;
  
  predictor = predict_xvid_new("/dev/null", NULL);
  while (!feof(stdin)) {
    length += fread(buf, sizeof(mem[0]), mem + sizeof(mem)/sizeof(mem[0]) - buf, stdin);
    buf = mem;
    do {
      predict_xvid(predictor, &buf, &length);
      if (predictor->valid)
	printf("%d, %G, %G, %G, %G, %G, %G, %G\n", predictor->frame_type,
	  predictor->metrics[METRIC_XVID_FRAME_BYTES],
	  predictor->metrics[METRIC_XVID_FRAME_PIXELS],
	  predictor->metrics[METRIC_XVID_FRAME_PIXELS_SQRT],
	  predictor->metrics[METRIC_XVID_MACRO_INTRA],
	  predictor->metrics[METRIC_XVID_MACRO_INTER],
	  predictor->metrics[METRIC_XVID_MOTION],
	  predictor->metrics[METRIC_XVID_MOTION_GLOBAL]);
      else
	printf("error extracting metrics\n");
    } while (length && (buf < mem + sizeof(mem)/sizeof(mem[0])/2 || feof(stdin)));
    memmove(mem, buf, length);
    buf = mem + length;
  }
  predict_xvid_dispose(predictor);

  return 0;
}
#endif


#else /* XVID_CODE_INCLUDED */


/*
 * The following code is taken from various files of the original XviD
 * library. Credits go to the members of the XviD team.
 *
 * Although the code has been heavily reduced to skip all the decoding
 * and only keep the bitstream parsing for metrics extraction, you should
 * still get something useful, when diffing this file against the
 * respective files of the XviD distribution, because indentation style
 * has been preserved. Use something like this to get a diff:
 *
 * sed -n '/FILE: xvidcore-1.0.3\/src\/xvid.c/,/FILE:/p' predict_xvid.c | diff -u xvidcore-1.0.3/src/xvid.c -
 */


/* FILE: xvidcore-1.0.3/src/xvid.h */

#define XVID_MAKE_VERSION(a,b,c) ((((a)&0xff)<<16) | (((b)&0xff)<<8) | ((c)&0xff))
#define XVID_VERSION_MAJOR(a)    ((char)(((a)>>16) & 0xff))
#define XVID_VERSION_MINOR(a)    ((char)(((a)>> 8) & 0xff))
#define XVID_VERSION_PATCH(a)    ((char)(((a)>> 0) & 0xff))

#define XVID_MAKE_API(a,b)       ((((a)&0xff)<<16) | (((b)&0xff)<<0))
#define XVID_API_MAJOR(a)        (((a)>>16) & 0xff)
#define XVID_API_MINOR(a)        (((a)>> 0) & 0xff)

#define XVID_VERSION             XVID_MAKE_VERSION(1,0,3)
#define XVID_API                 XVID_MAKE_API(4, 0)

/*****************************************************************************
 * error codes
 ****************************************************************************/

	/*	all functions return values <0 indicate error */

#define XVID_ERR_FAIL		-1		/* general fault */
#define XVID_ERR_MEMORY		-2		/* memory allocation error */
#define XVID_ERR_FORMAT		-3		/* file format error */
#define XVID_ERR_VERSION	-4		/* structure version not supported */
#define XVID_ERR_END		-5		/* encoder only; end of stream reached */

/*****************************************************************************
 * xvid_image_t
 ****************************************************************************/

/* frame type flags */
#define XVID_TYPE_VOL     -1 /* decoder only: vol was decoded */
#define XVID_TYPE_NOTHING  0 /* decoder only (encoder stats): nothing was decoded/encoded */
#define XVID_TYPE_AUTO     0 /* encoder: automatically determine coding type */
#define XVID_TYPE_IVOP     1 /* intra frame */
#define XVID_TYPE_PVOP     2 /* predicted frame */
#define XVID_TYPE_BVOP     3 /* bidirectionally encoded */
#define XVID_TYPE_SVOP     4 /* predicted+sprite frame */

/*****************************************************************************
 * xvid_global()
 ****************************************************************************/

#define XVID_DEBUG_ERROR     (1<< 0)
#define XVID_DEBUG_STARTCODE (1<< 1)
#define XVID_DEBUG_HEADER    (1<< 2)
#define XVID_DEBUG_TIMECODE  (1<< 3)
#define XVID_DEBUG_MB        (1<< 4)
#define XVID_DEBUG_COEFF     (1<< 5)
#define XVID_DEBUG_MV        (1<< 6)
#define XVID_DEBUG_RC        (1<< 7)
#define XVID_DEBUG_DEBUG     (1<<31)

/* XVID_GBL_INIT param1 */
typedef struct {
	int version;
} xvid_gbl_init_t;

#define XVID_GBL_INIT    0 /* initialize xvidcore; must be called before using xvid_decore, or xvid_encore) */

static int xvid_global(void *handle, int opt, void *param1, void *param2);

/*****************************************************************************
 * xvid_decore()
 ****************************************************************************/

#define XVID_DEC_CREATE  0 /* create decore instance; return 0 on success */
#define XVID_DEC_DESTROY 1 /* destroy decore instance: return 0 on success */
#define XVID_DEC_DECODE  2 /* decode a frame: returns number of bytes consumed >= 0 */

static int xvid_decore(void *handle, int opt, void *param1, void *param2);

/* XVID_DEC_CREATE param 1 */
typedef struct {
	int version;
	void * handle; /* [out]	   decore context handle */
} xvid_dec_create_t;

/* XVID_DEC_DECODE param1 */
/* general flags */
#define XVID_LOWDELAY      (1<<0) /* lowdelay mode  */
#define XVID_DISCONTINUITY (1<<1) /* indicates break in stream */
#define XVID_DEBLOCKY      (1<<2) /* perform luma deblocking */
#define XVID_DEBLOCKUV     (1<<3) /* perform chroma deblocking */
#define XVID_FILMEFFECT    (1<<4) /* adds film grain */

typedef struct {
	int version;
	int general;         /* [in:opt] general flags */
	void *bitstream;     /* [in]     bitstream (read from)*/
	int length;          /* [in]     bitstream length */
} xvid_dec_frame_t;

/* XVID_DEC_DECODE param2 :: optional */
typedef struct
{
	int version;

	int type;                   /* [out] output data type */
	int coding_type;
	union {
		struct { /* type>0 {XVID_TYPE_IVOP,XVID_TYPE_PVOP,XVID_TYPE_BVOP,XVID_TYPE_SVOP} */
			unsigned macro_intra;
			unsigned macro_inter;
			unsigned motion;
			unsigned motion_global;
		} vop;
		struct {	/* XVID_TYPE_VOL */
			int width;          /* [out] width */
			int height;         /* [out] height */
		} vol;
	} data;
} xvid_dec_stats_t;


/* FILE: xvidcore-1.0.3/src/portab.h */

#define int8_t   char
#define uint8_t  unsigned char
#define int16_t  short
#define uint16_t unsigned short
#define int32_t  int
#define uint32_t unsigned int
#define int64_t  long long
#define uint64_t unsigned long long

/*****************************************************************************
 *  Some things that are only architecture dependant
 ****************************************************************************/

#    define CACHE_LINE 64
#    define ptr_t uint32_t
#    define intptr_t int32_t
#        define uintptr_t uint32_t

/*----------------------------------------------------------------------------
  | Common gcc stuff
 *---------------------------------------------------------------------------*/

static __inline void DPRINTF(int level, char *format, ...) {}

#    define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) \
	type name##_storage[(sizex)*(sizey)+(alignment)-1]; \
type * name = (type *) (((ptr_t) name##_storage+(alignment - 1)) & ~((ptr_t)(alignment)-1))

/*----------------------------------------------------------------------------
  | gcc GENERIC (plain C only) specific macros/functions
 *---------------------------------------------------------------------------*/
#        define BSWAP(a) \
	((a) = (((a) & 0xff) << 24)  | (((a) & 0xff00) << 8) | \
	 (((a) >> 8) & 0xff00) | (((a) >> 24) & 0xff))


/* FILE: xvidcore-1.0.3/src/global.h */

/* --- macroblock modes --- */

#define MODE_INTER		0
#define MODE_INTER_Q	1
#define MODE_INTER4V	2
#define	MODE_INTRA		3
#define MODE_INTRA_Q	4
#define MODE_NOT_CODED	16
#define MODE_NOT_CODED_GMC	17

/* --- bframe specific --- */

#define MODE_DIRECT			0
#define MODE_INTERPOLATE	1
#define MODE_BACKWARD		2
#define MODE_FORWARD		3
#define MODE_DIRECT_NONE_MV	4
#define MODE_DIRECT_NO4V	5

/*
 * vop coding types
 * intra, prediction, backward, sprite, not_coded
 */
#define I_VOP	0
#define P_VOP	1
#define B_VOP	2
#define S_VOP	3
#define N_VOP	4

/* convert mpeg-4 coding type i/p/b/s_VOP to XVID_TYPE_xxx */
static __inline int
coding2type(int coding_type)
{
	return coding_type + 1;
}

typedef struct
{
	int x;
	int y;
}
VECTOR;

typedef struct
{
	VECTOR duv[3];
}
WARPPOINTS;

/* save all warping parameters for GMC once and for all, instead of
   recalculating for every block. This is needed for encoding&decoding
   When switching to incremental calculations, this will get much shorter
*/

/*	we don't include WARPPOINTS wp	here, but in FRAMEINFO itself */

typedef struct
{
	int num_wp;		/* [input]: 0=none, 1=translation, 2,3 = warping */
							/* a value of -1 means: "structure not initialized!" */
	int s;					/* [input]: calc is done with 1/s pel resolution */

	int W;
	int H;

	int ss;
	int smask;
	int sigma;

	int r;
	int rho;

	int i0s;
	int j0s;
	int i1s;
	int j1s;
	int i2s;
	int j2s;

	int i1ss;
	int j1ss;
	int i2ss;
	int j2ss;

	int alpha;
	int beta;
	int Ws;
	int Hs;

	int dxF, dyF, dxG, dyG;
	int Fo, Go;
	int cFo, cGo;
} GMC_DATA;

typedef struct _NEW_GMC_DATA
{
   /*  0=none, 1=translation, 2,3 = warping
	*  a value of -1 means: "structure not initialized!" */
	int num_wp;

	/* {0,1,2,3}  =>   {1/2,1/4,1/8,1/16} pel */
	int accuracy;

	/* sprite size * 16 */
	int sW, sH;

	/* gradient, calculated from warp points */
	int dU[2], dV[2], Uo, Vo, Uco, Vco;

	void (*predict_16x16)(const struct _NEW_GMC_DATA * const This,
						  uint8_t *dst, const uint8_t *src,
						  int dststride, int srcstride, int x, int y, int rounding);
} NEW_GMC_DATA;

typedef struct
{
	uint32_t bufa;
	uint32_t bufb;
	uint32_t buf;
	uint32_t pos;
	uint32_t *tail;
	uint32_t *start;
	uint32_t length;
	uint32_t initpos;
}
Bitstream;

#define MBPRED_SIZE  15

typedef struct
{
	/* decoder/encoder */
	VECTOR mvs[4];

	int mode;

	int field_dct;
	int field_pred;
	int field_for_top;
	int field_for_bot;

	int cbp;

	/* bframe stuff */
	VECTOR b_mvs[4];
}
MACROBLOCK;

/* useful macros */

#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))
/* #define ABS(X)    (((X)>0)?(X):-(X)) */
#define SIGN(X)   (((X)>0)?1:-1)
#define CLIP(X,AMIN,AMAX)   (((X)<(AMIN)) ? (AMIN) : ((X)>(AMAX)) ? (AMAX) : (X))
#define DIV_DIV(a,b)    (((a)>0) ? ((a)+((b)>>1))/(b) : ((a)-((b)>>1))/(b))
#define SWAP(_T_,A,B)    { _T_ tmp = A; A = B; B = tmp; }


/* FILE: xvidcore-1.0.3/src/decoder.h */

/*****************************************************************************
 * Structures
 ****************************************************************************/

/* complexity estimation toggles */
typedef struct
{
	int method;

	int opaque;
	int transparent;
	int intra_cae;
	int inter_cae;
	int no_update;
	int upsampling;

	int intra_blocks;
	int inter_blocks;
	int inter4v_blocks;
	int gmc_blocks;
	int not_coded_blocks;

	int dct_coefs;
	int dct_lines;
	int vlc_symbols;
	int vlc_bits;

	int apm;
	int npm;
	int interpolate_mc_q;
	int forw_back_mc_q;
	int halfpel2;
	int halfpel4;

	int sadct;
	int quarterpel;
} ESTIMATION;

typedef struct
{
	/* vol bitstream */

	int time_inc_resolution;
	int fixed_time_inc;
	uint32_t time_inc_bits;

	uint32_t shape;
	uint32_t quant_bits;
	uint32_t quant_type;
	int32_t quarterpel;
	int32_t cartoon_mode;
	int complexity_estimation_disable;
	ESTIMATION estimation;

	int interlacing;
	uint32_t top_field_first;
	uint32_t alternate_vertical_scan;

	int aspect_ratio;
	int par_width;
	int par_height;

	int sprite_enable;
	int sprite_warping_points;
	int sprite_warping_accuracy;
	int sprite_brightness_change;

	int newpred_enable;
	int reduced_resolution_enable;

	/* The bitstream version if it's a XviD stream */
	int bs_version;

	/* image */

	int fixed_dimensions;
	uint32_t width;
	uint32_t height;
	uint32_t edged_width;
	uint32_t edged_height;

	/* macroblock */

	uint32_t mb_width;
	uint32_t mb_height;
	MACROBLOCK *mbs;

	/*
	 * for B-frame & low_delay==0
	 * XXX: should move frame based stuff into a DECODER_FRAMEINFO struct
	 */
	MACROBLOCK *last_mbs;			/* last MB */
    int last_coding_type;           /* last coding type value */
	int last_reduced_resolution;	/* last reduced_resolution value */
	int32_t frames;				/* total frame number */
	int32_t packed_mode;		/* bframes packed bitstream? (1 = yes) */
	int8_t scalability;
	VECTOR p_fmv, p_bmv;		/* pred forward & backward motion vector */
	int64_t time;				/* for record time */
	int64_t time_base;
	int64_t last_time_base;
	int64_t last_non_b_time;
	int32_t time_pp;
	int32_t time_bp;
	uint32_t low_delay;			/* low_delay flage (1 means no B_VOP) */
	uint32_t low_delay_default;	/* default value for low_delay flag */

	/* for GMC: central place for all parameters */

	GMC_DATA gmc_data;
	NEW_GMC_DATA new_gmc_data;

	/* metrics for decoding time prediction */
	unsigned macro_intra;
	unsigned macro_inter;
	unsigned motion;
	unsigned motion_global;
}
DECODER;

/*****************************************************************************
 * Decoder prototypes
 ****************************************************************************/

static int decoder_create(xvid_dec_create_t * param);
static int decoder_destroy(DECODER * dec);
static int decoder_decode(DECODER * dec,
				   xvid_dec_frame_t * frame, xvid_dec_stats_t * stats);


/* FILE: xvidcore-1.0.3/src/bitstream/bitstream.h */

/*****************************************************************************
 * Constants
 ****************************************************************************/

/* comment any #defs we dont use */

#define VIDOBJ_START_CODE		0x00000100	/* ..0x0000011f  */
#define VIDOBJLAY_START_CODE	0x00000120	/* ..0x0000012f */
#define VISOBJSEQ_START_CODE	0x000001b0
#define VISOBJSEQ_STOP_CODE		0x000001b1	/* ??? */
#define USERDATA_START_CODE		0x000001b2
#define GRPOFVOP_START_CODE		0x000001b3
/*#define VIDSESERR_ERROR_CODE  0x000001b4 */
#define VISOBJ_START_CODE		0x000001b5
#define VOP_START_CODE			0x000001b6
/*#define STUFFING_START_CODE	0x000001c3 */

#define VISOBJ_TYPE_VIDEO				1
/*#define VISOBJ_TYPE_STILLTEXTURE      2 */
/*#define VISOBJ_TYPE_MESH              3 */
/*#define VISOBJ_TYPE_FBA               4 */
/*#define VISOBJ_TYPE_3DMESH            5 */

#define VIDOBJLAY_TYPE_SIMPLE			1
/*#define VIDOBJLAY_TYPE_SIMPLE_SCALABLE    2 */
/*#define VIDOBJLAY_TYPE_CORE				3 */
/*#define VIDOBJLAY_TYPE_MAIN				4 */
/*#define VIDOBJLAY_TYPE_NBIT				5 */
/*#define VIDOBJLAY_TYPE_ANIM_TEXT			6 */
/*#define VIDOBJLAY_TYPE_ANIM_MESH			7 */
/*#define VIDOBJLAY_TYPE_SIMPLE_FACE		8 */
/*#define VIDOBJLAY_TYPE_STILL_SCALABLE		9 */
#define VIDOBJLAY_TYPE_ART_SIMPLE		10
/*#define VIDOBJLAY_TYPE_CORE_SCALABLE		11 */
/*#define VIDOBJLAY_TYPE_ACE				12 */
/*#define VIDOBJLAY_TYPE_ADVANCED_SCALABLE_TEXTURE 13 */
/*#define VIDOBJLAY_TYPE_SIMPLE_FBA			14 */
/*#define VIDEOJLAY_TYPE_SIMPLE_STUDIO    15*/
/*#define VIDEOJLAY_TYPE_CORE_STUDIO      16*/
#define VIDOBJLAY_TYPE_ASP              17
/*#define VIDOBJLAY_TYPE_FGS              18*/

/*#define VIDOBJLAY_AR_SQUARE           1 */
/*#define VIDOBJLAY_AR_625TYPE_43       2 */
/*#define VIDOBJLAY_AR_525TYPE_43       3 */
/*#define VIDOBJLAY_AR_625TYPE_169      8 */
/*#define VIDOBJLAY_AR_525TYPE_169      9 */
#define VIDOBJLAY_AR_EXTPAR				15

#define VIDOBJLAY_SHAPE_RECTANGULAR		0
#define VIDOBJLAY_SHAPE_BINARY			1
#define VIDOBJLAY_SHAPE_BINARY_ONLY		2
#define VIDOBJLAY_SHAPE_GRAYSCALE		3

#define SPRITE_NONE		0
#define SPRITE_STATIC	1
#define SPRITE_GMC		2

#define READ_MARKER()	BitstreamSkip(bs, 1)
#define WRITE_MARKER()	BitstreamPutBit(bs, 1)

/* vop coding types  */
/* intra, prediction, backward, sprite, not_coded */
#define I_VOP	0
#define P_VOP	1
#define B_VOP	2
#define S_VOP	3
#define N_VOP	4

/* resync-specific */
#define NUMBITS_VP_RESYNC_MARKER  17
#define RESYNC_MARKER 1

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

static
int read_video_packet_header(Bitstream *bs,
							 DECODER * dec,
							 const int addbits,
							 int *quant,
							 int *fcode_forward,
							 int *fcode_backward,
							 int *intra_dc_threshold);

/* header stuff */
static
int BitstreamReadHeaders(Bitstream * bs,
						 DECODER * dec,
						 uint32_t * rounding,
						 uint32_t * reduced_resolution,
						 uint32_t * quant,
						 uint32_t * fcode_forward,
						 uint32_t * fcode_backward,
						 uint32_t * intra_dc_threshold,
						 WARPPOINTS * gmc_warp);

/* initialise bitstream structure */

static void __inline
BitstreamInit(Bitstream * const bs,
			  void *const bitstream,
			  uint32_t length)
{
	uint32_t tmp;
	size_t bitpos;
	ptr_t adjbitstream = (ptr_t)bitstream;

	/*
	 * Start the stream on a uint32_t boundary, by rounding down to the
	 * previous uint32_t and skipping the intervening bytes.
	 */
	bitpos = ((sizeof(uint32_t)-1) & (size_t)bitstream);
	adjbitstream = adjbitstream - bitpos;
	bs->start = bs->tail = (uint32_t *) adjbitstream;

	tmp = *bs->start;
#ifndef ARCH_IS_BIG_ENDIAN
	BSWAP(tmp);
#endif
	bs->bufa = tmp;

	tmp = *(bs->start + 1);
#ifndef ARCH_IS_BIG_ENDIAN
	BSWAP(tmp);
#endif
	bs->bufb = tmp;

	bs->buf = 0;
	bs->pos = bs->initpos = bitpos*8;
	bs->length = length;
}

/* reads n bits from bitstream without changing the stream pos */

static uint32_t __inline
BitstreamShowBits(Bitstream * const bs,
				  const uint32_t bits)
{
	int nbit = (bits + bs->pos) - 32;

	if (nbit > 0) {
		return ((bs->bufa & (0xffffffff >> bs->pos)) << nbit) | (bs->
																 bufb >> (32 -
																		  nbit));
	} else {
		return (bs->bufa & (0xffffffff >> bs->pos)) >> (32 - bs->pos - bits);
	}
}

/* skip n bits forward in bitstream */

static __inline void
BitstreamSkip(Bitstream * const bs,
			  const uint32_t bits)
{
	bs->pos += bits;

	if (bs->pos >= 32) {
		uint32_t tmp;

		bs->bufa = bs->bufb;
		tmp = *((uint32_t *) bs->tail + 2);
#ifndef ARCH_IS_BIG_ENDIAN
		BSWAP(tmp);
#endif
		bs->bufb = tmp;
		bs->tail++;
		bs->pos -= 32;
	}
}

/* number of bits to next byte alignment */
static __inline uint32_t
BitstreamNumBitsToByteAlign(Bitstream *bs)
{
	uint32_t n = (32 - bs->pos) % 8;
	return n == 0 ? 8 : n;
}

/* show nbits from next byte alignment */
static __inline uint32_t
BitstreamShowBitsFromByteAlign(Bitstream *bs, int bits)
{
	int bspos = bs->pos + BitstreamNumBitsToByteAlign(bs);
	int nbit = (bits + bspos) - 32;

	if (bspos >= 32) {
		return bs->bufb >> (32 - nbit);
	} else	if (nbit > 0) {
		return ((bs->bufa & (0xffffffff >> bspos)) << nbit) | (bs->
																 bufb >> (32 -
																		  nbit));
	} else {
		return (bs->bufa & (0xffffffff >> bspos)) >> (32 - bspos - bits);
	}

}

/* move forward to the next byte boundary */

static __inline void
BitstreamByteAlign(Bitstream * const bs)
{
	uint32_t remainder = bs->pos % 8;

	if (remainder) {
		BitstreamSkip(bs, 8 - remainder);
	}
}

/* bitstream length (unit bits) */

static uint32_t __inline
BitstreamPos(const Bitstream * const bs)
{
	return((uint32_t)(8*((ptr_t)bs->tail - (ptr_t)bs->start) + bs->pos - bs->initpos));
}

/*
 * flush the bitstream & return length (unit bytes)
 * NOTE: assumes no futher bitstream functions will be called.
 */

static uint32_t __inline
BitstreamLength(Bitstream * const bs)
{
	uint32_t len = (uint32_t)((ptr_t)bs->tail - (ptr_t)bs->start);

	if (bs->pos) {
		uint32_t b = bs->buf;

#ifndef ARCH_IS_BIG_ENDIAN
		BSWAP(b);
#endif
		*bs->tail = b;

		len += (bs->pos + 7) / 8;
	}

	/* initpos is always on a byte boundary */
	if (bs->initpos)
		len -= bs->initpos/8;

	return len;
}

/* read n bits from bitstream */

static uint32_t __inline
BitstreamGetBits(Bitstream * const bs,
				 const uint32_t n)
{
	uint32_t ret = BitstreamShowBits(bs, n);

	BitstreamSkip(bs, n);
	return ret;
}

/* read single bit from bitstream */

static uint32_t __inline
BitstreamGetBit(Bitstream * const bs)
{
	return BitstreamGetBits(bs, 1);
}


/* FILE: xvidcore-1.0.3/src/bitstream/mbcoding.h */

static void init_vlc_tables(void);

static int check_resync_marker(Bitstream * bs, int addbits);

static int bs_get_spritetrajectory(Bitstream * bs);

static int get_mcbpc_intra(Bitstream * bs);
static int get_mcbpc_inter(Bitstream * bs);
static int get_cbpy(Bitstream * bs,
			 int intra);
static int get_mv(Bitstream * bs,
		   int fcode);

static int get_dc_dif(Bitstream * bs,
			   uint32_t dc_size);
static int get_dc_size_lum(Bitstream * bs);
static int get_dc_size_chrom(Bitstream * bs);

static
void get_intra_block(Bitstream * bs);
static
void get_inter_block(Bitstream * bs);


/* FILE: xvidcore-1.0.3/src/bitstream/vlc_codes.h */

#define VLC_ERROR	(-1)

#define ESCAPE  3
#define ESCAPE1 6
#define ESCAPE2 14
#define ESCAPE3 15

typedef struct
{
	uint32_t code;
	uint8_t len;
}
VLC;

typedef struct
{
	uint8_t last;
	uint8_t run;
	int8_t level;
}
EVENT;

typedef struct
{
	uint8_t len;
	EVENT event;
}
REVERSE_EVENT;

typedef struct
{
	VLC vlc;
	EVENT event;
}
VLC_TABLE;


/* FILE: xvidcore-1.0.3/src/motion/gmc.h */

/* And this is borrowed from   bitstream.c  until we find a common solution */
static uint32_t __inline
log2bin(uint32_t value)
{
/* Changed by Chenm001 */
#if !defined(_MSC_VER)
  int n = 0;

  while (value) {
	value >>= 1;
	n++;
  }
  return n;
#else
  __asm {
	bsr eax, value
	inc eax
  }
#endif
}
/* 16*sizeof(int) -> 1 or 2 cachelines */
/* table lookup might be faster!  (still to be benchmarked) */

/*
static int log2bin_table[16] =
	{ 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4};
*/
/*	1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 */

#define RDIV(a,b) (((a)>0 ? (a) + ((b)>>1) : (a) - ((b)>>1))/(b))
#define RSHIFT(a,b) ( (a)>0 ? ((a) + (1<<((b)-1)))>>(b) : ((a) + (1<<((b)-1))-1)>>(b))

#define MLT(i)  (((16-(i))<<16) + (i))
static const uint32_t MTab[16] = {
  MLT( 0), MLT( 1), MLT( 2), MLT( 3), MLT( 4), MLT( 5), MLT( 6), MLT( 7),
  MLT( 8), MLT( 9), MLT(10), MLT(11), MLT(12), MLT(13), MLT(14), MLT(15)
};
#undef MLT

/* ************************************************************
 * Pts = 2 or 3
 *
 * Warning! *src is the global frame pointer (that is: adress
 * of pixel 0,0), not the macroblock one.
 * Conversely, *dst is the macroblock top-left adress.
 */

static
void Predict_16x16_C(const NEW_GMC_DATA * const This,
					 uint8_t *dst, const uint8_t *src,
					 int dststride, int srcstride, int x, int y, int rounding);

/* ************************************************************
 * simplified version for 1 warp point
 */

static
void Predict_1pt_16x16_C(const NEW_GMC_DATA * const This,
						 uint8_t *Dst, const uint8_t *Src,
						 int dststride, int srcstride, int x, int y, int rounding);

/* *************************************************************
 * Warning! It's Accuracy being passed, not 'resolution'!
 */

static
void generate_GMCparameters(int nb_pts, const int accuracy,
							const WARPPOINTS *const pts,
							const int width, const int height,
							NEW_GMC_DATA *const gmc);


/* FILE: xvidcore-1.0.3/src/utils/mem_align.h */

static
void *xvid_malloc(size_t size,
				  uint8_t alignment);
static
void xvid_free(void *mem_ptr);


/* FILE: xvidcore-1.0.3/src/xvid.c */

static
int xvid_gbl_init(xvid_gbl_init_t * init)
{
	if (XVID_VERSION_MAJOR(init->version) != 1) /* v1.x.x */
		return XVID_ERR_VERSION;

	init_vlc_tables();

    return 0;
}

static int
xvid_global(void *handle,
		  int opt,
		  void *param1,
		  void *param2)
{
	switch(opt)
	{
		case XVID_GBL_INIT :
			return xvid_gbl_init((xvid_gbl_init_t*)param1);

		default :
			return XVID_ERR_FAIL;
	}
}

static int
xvid_decore(void *handle,
			int opt,
			void *param1,
			void *param2)
{
	switch (opt) {
	case XVID_DEC_CREATE:
		return decoder_create((xvid_dec_create_t *) param1);

	case XVID_DEC_DESTROY:
		return decoder_destroy((DECODER *) handle);

	case XVID_DEC_DECODE:
		return decoder_decode((DECODER *) handle, (xvid_dec_frame_t *) param1, (xvid_dec_stats_t*) param2);

	default:
		return XVID_ERR_FAIL;
	}
}


/* FILE: xvidcore-1.0.3/src/image/reduced.h */

/* rrv motion vector scale-up */
#define RRV_MV_SCALEUP(a)	( (a)>0 ? 2*(a)-1 : (a)<0 ? 2*(a)+1 : (a) )


/* FILE: xvidcore-1.0.3/src/motion/estimation_common.c */

/* K = 4 */
static
const uint32_t roundtab_76[16] =
{ 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1 };

/* K = 1 */
static
const uint32_t roundtab_79[4] =
{ 0, 1, 0, 0 };


/* FILE: xvidcore-1.0.3/src/prediction/mbprediction.c */

static const VECTOR zeroMV = { 0, 0 };

static VECTOR
get_pmv2(const MACROBLOCK * const mbs,
		const int mb_width,
		const int bound,
		const int x,
		const int y,
		const int block)
{
	int lx, ly, lz;		/* left */
	int tx, ty, tz;		/* top */
	int rx, ry, rz;		/* top-right */
	int lpos, tpos, rpos;
	int num_cand = 0, last_cand = 1;

	VECTOR pmv[4];	/* left neighbour, top neighbour, top-right neighbour */

	switch (block) {
	case 0:
		lx = x - 1;	ly = y;		lz = 1;
		tx = x;		ty = y - 1;	tz = 2;
		rx = x + 1;	ry = y - 1;	rz = 2;
		break;
	case 1:
		lx = x;		ly = y;		lz = 0;
		tx = x;		ty = y - 1;	tz = 3;
		rx = x + 1;	ry = y - 1;	rz = 2;
		break;
	case 2:
		lx = x - 1;	ly = y;		lz = 3;
		tx = x;		ty = y;		tz = 0;
		rx = x;		ry = y;		rz = 1;
		break;
	default:
		lx = x;		ly = y;		lz = 2;
		tx = x;		ty = y;		tz = 0;
		rx = x;		ry = y;		rz = 1;
	}

	lpos = lx + ly * mb_width;
	rpos = rx + ry * mb_width;
	tpos = tx + ty * mb_width;

	if (lpos >= bound && lx >= 0) {
		num_cand++;
		pmv[1] = mbs[lpos].mvs[lz];
	} else pmv[1] = zeroMV;

	if (tpos >= bound) {
		num_cand++;
		last_cand = 2;
		pmv[2] = mbs[tpos].mvs[tz];
	} else pmv[2] = zeroMV;

	if (rpos >= bound && rx < mb_width) {
		num_cand++;
		last_cand = 3;
		pmv[3] = mbs[rpos].mvs[rz];
	} else pmv[3] = zeroMV;

	/* If there're more than one candidate, we return the median vector */

	if (num_cand > 1) {
		/* set median */
		pmv[0].x =
			MIN(MAX(pmv[1].x, pmv[2].x),
				MIN(MAX(pmv[2].x, pmv[3].x), MAX(pmv[1].x, pmv[3].x)));
		pmv[0].y =
			MIN(MAX(pmv[1].y, pmv[2].y),
				MIN(MAX(pmv[2].y, pmv[3].y), MAX(pmv[1].y, pmv[3].y)));
		return pmv[0];
	}

	return pmv[last_cand];	/* no point calculating median mv */
}


/* FILE: xvidcore-1.0.3/src/decoder.c */

#define HPEL(X,Y) (hpel_cost[((X)&1)|(((Y)&1)<<1)])
#define QPEL(X,Y) (qpel_cost[((X)&3)|(((Y)&3)<<2)])
static const int hpel_cost[] = {
  1, 2, 2, 4
};
static const int qpel_cost[] = {
  1, 10, 8, 10, 10, 20, 18, 20, 8, 18, 16, 18, 10, 20, 18, 20
};

static int
decoder_resize(DECODER * dec)
{
	if (dec->last_mbs)
		xvid_free(dec->last_mbs);
	if (dec->mbs)
		xvid_free(dec->mbs);

	/* realloc */
	dec->mb_width = (dec->width + 15) / 16;
	dec->mb_height = (dec->height + 15) / 16;

	dec->mbs =
		xvid_malloc(sizeof(MACROBLOCK) * dec->mb_width * dec->mb_height,
					CACHE_LINE);
	if (dec->mbs == NULL) {
		xvid_free(dec);
		return XVID_ERR_MEMORY;
	}
	memset(dec->mbs, 0, sizeof(MACROBLOCK) * dec->mb_width * dec->mb_height);

	/* For skip MB flag */
	dec->last_mbs =
		xvid_malloc(sizeof(MACROBLOCK) * dec->mb_width * dec->mb_height,
					CACHE_LINE);
	if (dec->last_mbs == NULL) {
		xvid_free(dec->mbs);
		xvid_free(dec);
		return XVID_ERR_MEMORY;
	}

	memset(dec->last_mbs, 0, sizeof(MACROBLOCK) * dec->mb_width * dec->mb_height);

	return 0;
}


static int
decoder_create(xvid_dec_create_t * create)
{
	DECODER *dec;

	if (XVID_VERSION_MAJOR(create->version) != 1)	/* v1.x.x */
		return XVID_ERR_VERSION;

	dec = xvid_malloc(sizeof(DECODER), CACHE_LINE);
	if (dec == NULL) {
		return XVID_ERR_MEMORY;
	}

	memset(dec, 0, sizeof(DECODER));

	create->handle = dec;

	dec->mbs = NULL;
	dec->last_mbs = NULL;

	/* For B-frame support (used to save reference frame's time */
	dec->frames = 0;
	dec->time = dec->time_base = dec->last_time_base = 0;
	dec->low_delay = 0;
	dec->packed_mode = 0;
	dec->time_inc_resolution = 1; /* until VOL header says otherwise */

	dec->fixed_dimensions = (dec->width > 0 && dec->height > 0);

	if (dec->fixed_dimensions)
		return decoder_resize(dec);
	else
		return 0;
}


static int
decoder_destroy(DECODER * dec)
{
	xvid_free(dec->last_mbs);
	xvid_free(dec->mbs);
	xvid_free(dec);

	return 0;
}

static const int32_t dquant_table[4] = {
	-1, -2, 1, 2
};

/* decode an intra macroblock */
static void
decoder_mbintra(DECODER * dec,
				const uint32_t cbp,
				Bitstream * bs,
				const uint32_t quant,
				const uint32_t intra_dc_threshold)
{
	uint32_t i;

	for (i = 0; i < 6; i++) {
		dec->macro_intra++;

		if (quant < intra_dc_threshold) {
			int dc_size;
			int dc_dif;

			dc_size = i < 4 ? get_dc_size_lum(bs) : get_dc_size_chrom(bs);
			dc_dif = dc_size ? get_dc_dif(bs, dc_size) : 0;

			if (dc_size > 8) {
				BitstreamSkip(bs, 1);	/* marker */
			}
		}

		if (cbp & (1 << (5 - i)))	/* coded */
		{
			get_intra_block(bs);
		}
	}
}

static void
decoder_mb_decode(DECODER * dec,
				const uint32_t cbp,
				Bitstream * bs)
{
	int i;

	for (i = 0; i < 6; i++) {

		if (cbp & (1 << (5 - i))) {	/* coded */
			dec->macro_inter++;
			get_inter_block(bs);
		}
	}
}

/* decode an inter macroblock */
static void
decoder_mbinter(DECODER * dec,
				const MACROBLOCK * pMB,
				const uint32_t cbp,
				Bitstream * bs,
				const int reduced_resolution)
{
	uint32_t i;

	int uv_dx, uv_dy;
	VECTOR mv[4];	/* local copy of mvs */

	if (reduced_resolution) {
		for (i = 0; i < 4; i++)	{
			mv[i].x = RRV_MV_SCALEUP(pMB->mvs[i].x);
			mv[i].y = RRV_MV_SCALEUP(pMB->mvs[i].y);
		}
	} else {
		for (i = 0; i < 4; i++)
			mv[i] = pMB->mvs[i];
	}

	if (pMB->mode != MODE_INTER4V) { /* INTER, INTER_Q, NOT_CODED, FORWARD, BACKWARD */

		uv_dx = mv[0].x;
		uv_dy = mv[0].y;
		if (dec->quarterpel) {
			uv_dx /= 2;
			uv_dy /= 2;
		}
		uv_dx = (uv_dx >> 1) + roundtab_79[uv_dx & 0x3];
		uv_dy = (uv_dy >> 1) + roundtab_79[uv_dy & 0x3];

		if (reduced_resolution)
			dec->motion += HPEL(mv[0].x,mv[0].y)*32*32;
		else if (dec->quarterpel)
			dec->motion += QPEL(mv[0].x,mv[0].y)*16*16;
		else
			dec->motion += HPEL(mv[0].x,mv[0].y)*16*16;

	} else {	/* MODE_INTER4V */

		if(dec->quarterpel) {
			uv_dx = (mv[0].x / 2) + (mv[1].x / 2) + (mv[2].x / 2) + (mv[3].x / 2);
			uv_dy = (mv[0].y / 2) + (mv[1].y / 2) + (mv[2].y / 2) + (mv[3].y / 2);
		} else {
			uv_dx = mv[0].x + mv[1].x + mv[2].x + mv[3].x;
			uv_dy = mv[0].y + mv[1].y + mv[2].y + mv[3].y;
		}

		uv_dx = (uv_dx >> 3) + roundtab_76[uv_dx & 0xf];
		uv_dy = (uv_dy >> 3) + roundtab_76[uv_dy & 0xf];

		if (reduced_resolution) {
			dec->motion += HPEL(mv[0].x,mv[0].y)*16*16;
			dec->motion += HPEL(mv[1].x,mv[1].y)*16*16;
			dec->motion += HPEL(mv[2].x,mv[2].y)*16*16;
			dec->motion += HPEL(mv[3].x,mv[3].y)*16*16;
			dec->motion += 2*HPEL(uv_dx,uv_dy)*16*16;
		} else if (dec->quarterpel) {
			dec->motion += QPEL(mv[0].x,mv[0].y)*8*8;
			dec->motion += QPEL(mv[1].x,mv[1].y)*8*8;
			dec->motion += QPEL(mv[2].x,mv[2].y)*8*8;
			dec->motion += QPEL(mv[3].x,mv[3].y)*8*8;
		} else {
			dec->motion += HPEL(mv[0].x,mv[0].y)*8*8;
			dec->motion += HPEL(mv[1].x,mv[1].y)*8*8;
			dec->motion += HPEL(mv[2].x,mv[2].y)*8*8;
			dec->motion += HPEL(mv[3].x,mv[3].y)*8*8;
		}
	}

	/* chroma */
	if (reduced_resolution) {
		dec->motion += 2*HPEL(uv_dx,uv_dy)*16*16;
	} else {
		dec->motion += 2*HPEL(uv_dx,uv_dy)*8*8;
	}

	if (cbp)
		decoder_mb_decode(dec, cbp, bs);
}

static void
decoder_mbgmc(DECODER * dec,
				const uint32_t cbp,
				Bitstream * bs)
{
	dec->motion_global++;

	if (cbp)
		decoder_mb_decode(dec, cbp, bs);

}

static void
decoder_iframe(DECODER * dec,
				Bitstream * bs,
				int reduced_resolution,
				int quant,
				int intra_dc_threshold)
{
	uint32_t bound;
	uint32_t x, y;
	uint32_t mb_width = dec->mb_width;
	uint32_t mb_height = dec->mb_height;

	if (reduced_resolution) {
		mb_width = (dec->width + 31) / 32;
		mb_height = (dec->height + 31) / 32;
	}

	bound = 0;

	for (y = 0; y < mb_height; y++) {
		for (x = 0; x < mb_width; x++) {
			MACROBLOCK *mb;
			uint32_t mcbpc;
			uint32_t cbpc;
			uint32_t acpred_flag;
			uint32_t cbpy;
			uint32_t cbp;

			while (BitstreamShowBits(bs, 9) == 1)
				BitstreamSkip(bs, 9);

			if (check_resync_marker(bs, 0))
			{
				bound = read_video_packet_header(bs, dec, 0,
							&quant, NULL, NULL, &intra_dc_threshold);
				x = bound % mb_width;
				y = bound / mb_width;
			}
			mb = &dec->mbs[y * dec->mb_width + x];

			DPRINTF(XVID_DEBUG_MB, "macroblock (%i,%i) %08x\n", x, y, BitstreamShowBits(bs, 32));

			mcbpc = get_mcbpc_intra(bs);
			mb->mode = mcbpc & 7;
			cbpc = (mcbpc >> 4);

			acpred_flag = BitstreamGetBit(bs);

			cbpy = get_cbpy(bs, 1);
			cbp = (cbpy << 2) | cbpc;

			if (mb->mode == MODE_INTRA_Q) {
				quant += dquant_table[BitstreamGetBits(bs, 2)];
				if (quant > 31) {
					quant = 31;
				} else if (quant < 1) {
					quant = 1;
				}
			}
			mb->mvs[0].x = mb->mvs[0].y =
			mb->mvs[1].x = mb->mvs[1].y =
			mb->mvs[2].x = mb->mvs[2].y =
			mb->mvs[3].x = mb->mvs[3].y =0;

			if (dec->interlacing) {
				mb->field_dct = BitstreamGetBit(bs);
				DPRINTF(XVID_DEBUG_MB,"deci: field_dct: %i\n", mb->field_dct);
			}

			decoder_mbintra(dec, cbp, bs, quant, intra_dc_threshold);

		}
	}

}

static void
get_motion_vector(DECODER * dec,
				Bitstream * bs,
				int x,
				int y,
				int k,
				VECTOR * ret_mv,
				int fcode,
				const int bound)
{

	const int scale_fac = 1 << (fcode - 1);
	const int high = (32 * scale_fac) - 1;
	const int low = ((-32) * scale_fac);
	const int range = (64 * scale_fac);

	const VECTOR pmv = get_pmv2(dec->mbs, dec->mb_width, bound, x, y, k);
	VECTOR mv;

	mv.x = get_mv(bs, fcode);
	mv.y = get_mv(bs, fcode);

	DPRINTF(XVID_DEBUG_MV,"mv_diff (%i,%i) pred (%i,%i) result (%i,%i)\n", mv.x, mv.y, pmv.x, pmv.y, mv.x+pmv.x, mv.y+pmv.y);

	mv.x += pmv.x;
	mv.y += pmv.y;

	if (mv.x < low) {
		mv.x += range;
	} else if (mv.x > high) {
		mv.x -= range;
	}

	if (mv.y < low) {
		mv.y += range;
	} else if (mv.y > high) {
		mv.y -= range;
	}

	ret_mv->x = mv.x;
	ret_mv->y = mv.y;
}

/* for P_VOP set gmc_warp to NULL */
static void
decoder_pframe(DECODER * dec,
				Bitstream * bs,
				int rounding,
				int reduced_resolution,
				int quant,
				int fcode,
				int intra_dc_threshold,
				const WARPPOINTS *const gmc_warp)
{
	uint32_t x, y;
	uint32_t bound;
	int cp_mb, st_mb;
	uint32_t mb_width = dec->mb_width;
	uint32_t mb_height = dec->mb_height;

	if (reduced_resolution) {
		mb_width = (dec->width + 31) / 32;
		mb_height = (dec->height + 31) / 32;
	}

	if (gmc_warp) {
		/* accuracy: 0==1/2, 1=1/4, 2=1/8, 3=1/16 */
		generate_GMCparameters(	dec->sprite_warping_points,
				dec->sprite_warping_accuracy, gmc_warp,
				dec->width, dec->height, &dec->new_gmc_data);

		/* image warping is done block-based in decoder_mbgmc(), now */
	}

	bound = 0;

	for (y = 0; y < mb_height; y++) {
		cp_mb = st_mb = 0;
		for (x = 0; x < mb_width; x++) {
			MACROBLOCK *mb;

			/* skip stuffing */
			while (BitstreamShowBits(bs, 10) == 1)
				BitstreamSkip(bs, 10);

			if (check_resync_marker(bs, fcode - 1)) {
				bound = read_video_packet_header(bs, dec, fcode - 1,
					&quant, &fcode, NULL, &intra_dc_threshold);
				x = bound % mb_width;
				y = bound / mb_width;
			}
			mb = &dec->mbs[y * dec->mb_width + x];

			DPRINTF(XVID_DEBUG_MB, "macroblock (%i,%i) %08x\n", x, y, BitstreamShowBits(bs, 32));

			if (!(BitstreamGetBit(bs)))	{ /* block _is_ coded */
				uint32_t mcbpc, cbpc, cbpy, cbp;
				uint32_t intra, acpred_flag = 0;
				int mcsel = 0;		/* mcsel: '0'=local motion, '1'=GMC */

				cp_mb++;
				mcbpc = get_mcbpc_inter(bs);
				mb->mode = mcbpc & 7;
				cbpc = (mcbpc >> 4);

				DPRINTF(XVID_DEBUG_MB, "mode %i\n", mb->mode);
				DPRINTF(XVID_DEBUG_MB, "cbpc %i\n", cbpc);

				intra = (mb->mode == MODE_INTRA || mb->mode == MODE_INTRA_Q);

				if (gmc_warp && (mb->mode == MODE_INTER || mb->mode == MODE_INTER_Q))
					mcsel = BitstreamGetBit(bs);
				else if (intra)
					acpred_flag = BitstreamGetBit(bs);

				cbpy = get_cbpy(bs, intra);
				DPRINTF(XVID_DEBUG_MB, "cbpy %i mcsel %i \n", cbpy,mcsel);

				cbp = (cbpy << 2) | cbpc;

				if (mb->mode == MODE_INTER_Q || mb->mode == MODE_INTRA_Q) {
					int dquant = dquant_table[BitstreamGetBits(bs, 2)];
					DPRINTF(XVID_DEBUG_MB, "dquant %i\n", dquant);
					quant += dquant;
					if (quant > 31) {
						quant = 31;
					} else if (quant < 1) {
						quant = 1;
					}
					DPRINTF(XVID_DEBUG_MB, "quant %i\n", quant);
				}

				if (dec->interlacing) {
					if (cbp || intra) {
						mb->field_dct = BitstreamGetBit(bs);
						DPRINTF(XVID_DEBUG_MB,"decp: field_dct: %i\n", mb->field_dct);
					}

					if ((mb->mode == MODE_INTER || mb->mode == MODE_INTER_Q) && !mcsel) {
						mb->field_pred = BitstreamGetBit(bs);
						DPRINTF(XVID_DEBUG_MB, "decp: field_pred: %i\n", mb->field_pred);

						if (mb->field_pred) {
							mb->field_for_top = BitstreamGetBit(bs);
							DPRINTF(XVID_DEBUG_MB,"decp: field_for_top: %i\n", mb->field_for_top);
							mb->field_for_bot = BitstreamGetBit(bs);
							DPRINTF(XVID_DEBUG_MB,"decp: field_for_bot: %i\n", mb->field_for_bot);
						}
					}
				}

				if (mcsel) {
					decoder_mbgmc(dec, cbp, bs);
					continue;

				} else if (mb->mode == MODE_INTER || mb->mode == MODE_INTER_Q) {

					if (dec->interlacing && mb->field_pred) {
						get_motion_vector(dec, bs, x, y, 0, &mb->mvs[0], fcode, bound);
						get_motion_vector(dec, bs, x, y, 0, &mb->mvs[1], fcode, bound);
					} else {
						get_motion_vector(dec, bs, x, y, 0, &mb->mvs[0], fcode, bound);
						mb->mvs[1] = mb->mvs[2] = mb->mvs[3] = mb->mvs[0];
					}
				} else if (mb->mode == MODE_INTER4V ) {
					get_motion_vector(dec, bs, x, y, 0, &mb->mvs[0], fcode, bound);
					get_motion_vector(dec, bs, x, y, 1, &mb->mvs[1], fcode, bound);
					get_motion_vector(dec, bs, x, y, 2, &mb->mvs[2], fcode, bound);
					get_motion_vector(dec, bs, x, y, 3, &mb->mvs[3], fcode, bound);
				} else {		/* MODE_INTRA, MODE_INTRA_Q */
					mb->mvs[0].x = mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x = 0;
					mb->mvs[0].y = mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y =	0;
					decoder_mbintra(dec, cbp, bs, quant, intra_dc_threshold);
					continue;
				}

				decoder_mbinter(dec, mb, cbp, bs, reduced_resolution);

			} else if (gmc_warp) {	/* a not coded S(GMC)-VOP macroblock */
				mb->mode = MODE_NOT_CODED_GMC;
				decoder_mbgmc(dec, 0x00, bs);
				st_mb = x+1;
			} else {	/* not coded P_VOP macroblock */
				mb->mode = MODE_NOT_CODED;

				mb->mvs[0].x = mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x = 0;
				mb->mvs[0].y = mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y = 0;

				decoder_mbinter(dec, mb, 0, bs, reduced_resolution);
				st_mb = x+1;
			}
		}
	}
}

/* decode B-frame motion vector */
static void
get_b_motion_vector(Bitstream * bs,
					VECTOR * mv,
					int fcode,
					const VECTOR pmv,
					const DECODER * const dec,
					const int x, const int y)
{
	const int scale_fac = 1 << (fcode - 1);
	const int high = (32 * scale_fac) - 1;
	const int low = ((-32) * scale_fac);
	const int range = (64 * scale_fac);

	int mv_x = get_mv(bs, fcode);
	int mv_y = get_mv(bs, fcode);

	mv_x += pmv.x;
	mv_y += pmv.y;

	if (mv_x < low)
		mv_x += range;
	else if (mv_x > high)
		mv_x -= range;

	if (mv_y < low)
		mv_y += range;
	else if (mv_y > high)
		mv_y -= range;

	mv->x = mv_x;
	mv->y = mv_y;
}

/* decode an B-frame direct & interpolate macroblock */
static void
decoder_bf_interpolate_mbinter(DECODER * dec,
								MACROBLOCK * pMB,
								Bitstream * bs,
								const int direct)
{
	int uv_dx, uv_dy;
	int b_uv_dx, b_uv_dy;
	const uint32_t cbp = pMB->cbp;

	if (!direct) {
		uv_dx = pMB->mvs[0].x;
		uv_dy = pMB->mvs[0].y;

		b_uv_dx = pMB->b_mvs[0].x;
		b_uv_dy = pMB->b_mvs[0].y;

		if (dec->quarterpel) {
			uv_dx /= 2;
			uv_dy /= 2;
			b_uv_dx /= 2;
			b_uv_dy /= 2;
		}

		uv_dx = (uv_dx >> 1) + roundtab_79[uv_dx & 0x3];
		uv_dy = (uv_dy >> 1) + roundtab_79[uv_dy & 0x3];

		b_uv_dx = (b_uv_dx >> 1) + roundtab_79[b_uv_dx & 0x3];
		b_uv_dy = (b_uv_dy >> 1) + roundtab_79[b_uv_dy & 0x3];

	} else {
		if(dec->quarterpel) {
			uv_dx = (pMB->mvs[0].x / 2) + (pMB->mvs[1].x / 2) + (pMB->mvs[2].x / 2) + (pMB->mvs[3].x / 2);
			uv_dy = (pMB->mvs[0].y / 2) + (pMB->mvs[1].y / 2) + (pMB->mvs[2].y / 2) + (pMB->mvs[3].y / 2);
			b_uv_dx = (pMB->b_mvs[0].x / 2) + (pMB->b_mvs[1].x / 2) + (pMB->b_mvs[2].x / 2) + (pMB->b_mvs[3].x / 2);
			b_uv_dy = (pMB->b_mvs[0].y / 2) + (pMB->b_mvs[1].y / 2) + (pMB->b_mvs[2].y / 2) + (pMB->b_mvs[3].y / 2);
		} else {
			uv_dx = pMB->mvs[0].x + pMB->mvs[1].x + pMB->mvs[2].x + pMB->mvs[3].x;
			uv_dy = pMB->mvs[0].y + pMB->mvs[1].y + pMB->mvs[2].y + pMB->mvs[3].y;
			b_uv_dx = pMB->b_mvs[0].x + pMB->b_mvs[1].x + pMB->b_mvs[2].x + pMB->b_mvs[3].x;
			b_uv_dy = pMB->b_mvs[0].y + pMB->b_mvs[1].y + pMB->b_mvs[2].y + pMB->b_mvs[3].y;
		}

		uv_dx = (uv_dx >> 3) + roundtab_76[uv_dx & 0xf];
		uv_dy = (uv_dy >> 3) + roundtab_76[uv_dy & 0xf];
		b_uv_dx = (b_uv_dx >> 3) + roundtab_76[b_uv_dx & 0xf];
		b_uv_dy = (b_uv_dy >> 3) + roundtab_76[b_uv_dy & 0xf];
	}

	if(dec->quarterpel) {
		if(!direct) {
			dec->motion += QPEL(pMB->mvs[0].x,pMB->mvs[0].y)*16*16;
		} else {
			dec->motion += QPEL(pMB->mvs[0].x,pMB->mvs[0].y)*8*8;
			dec->motion += QPEL(pMB->mvs[1].x,pMB->mvs[1].y)*8*8;
			dec->motion += QPEL(pMB->mvs[2].x,pMB->mvs[2].y)*8*8;
			dec->motion += QPEL(pMB->mvs[3].x,pMB->mvs[3].y)*8*8;
		}
	} else {
		dec->motion += HPEL(pMB->mvs[0].x,pMB->mvs[0].y)*8*8;
		dec->motion += HPEL(pMB->mvs[1].x,pMB->mvs[1].y)*8*8;
		dec->motion += HPEL(pMB->mvs[2].x,pMB->mvs[2].y)*8*8;
		dec->motion += HPEL(pMB->mvs[3].x,pMB->mvs[3].y)*8*8;
	}

	dec->motion += 2*HPEL(uv_dx,uv_dy)*8*8;

	if(dec->quarterpel) {
		if(!direct) {
			dec->motion += QPEL(pMB->b_mvs[0].x,pMB->b_mvs[0].y)*16*16;
		} else {
			dec->motion += QPEL(pMB->b_mvs[0].x,pMB->b_mvs[0].y)*8*8;
			dec->motion += QPEL(pMB->b_mvs[1].x,pMB->b_mvs[1].y)*8*8;
			dec->motion += QPEL(pMB->b_mvs[2].x,pMB->b_mvs[2].y)*8*8;
			dec->motion += QPEL(pMB->b_mvs[3].x,pMB->b_mvs[3].y)*8*8;
		}
	} else {
		dec->motion += HPEL(pMB->b_mvs[0].x,pMB->b_mvs[0].y)*8*8;
		dec->motion += HPEL(pMB->b_mvs[1].x,pMB->b_mvs[1].y)*8*8;
		dec->motion += HPEL(pMB->b_mvs[2].x,pMB->b_mvs[2].y)*8*8;
		dec->motion += HPEL(pMB->b_mvs[3].x,pMB->b_mvs[3].y)*8*8;
	}

	dec->motion += 2*HPEL(b_uv_dx,b_uv_dy)*8*8;
	dec->motion += 6*2*8*8;

	if (cbp)
		decoder_mb_decode(dec, cbp, bs);
}

/* for decode B-frame dbquant */
static __inline int32_t
get_dbquant(Bitstream * bs)
{
	if (!BitstreamGetBit(bs))		/*  '0' */
		return (0);
	else if (!BitstreamGetBit(bs))	/* '10' */
		return (-2);
	else							/* '11' */
		return (2);
}

/*
 * decode B-frame mb_type
 * bit		ret_value
 * 1		0
 * 01		1
 * 001		2
 * 0001		3
 */
static int32_t __inline
get_mbtype(Bitstream * bs)
{
	int32_t mb_type;

	for (mb_type = 0; mb_type <= 3; mb_type++)
		if (BitstreamGetBit(bs))
			return (mb_type);

	return -1;
}

static void
decoder_bframe(DECODER * dec,
				Bitstream * bs,
				int quant,
				int fcode_forward,
				int fcode_backward)
{
	uint32_t x, y;
	VECTOR mv;
	const VECTOR zeromv = {0,0};
	int i;

	for (y = 0; y < dec->mb_height; y++) {
		/* Initialize Pred Motion Vector */
		dec->p_fmv = dec->p_bmv = zeromv;
		for (x = 0; x < dec->mb_width; x++) {
			MACROBLOCK *mb = &dec->mbs[y * dec->mb_width + x];
			MACROBLOCK *last_mb = &dec->last_mbs[y * dec->mb_width + x];
			const int fcode_max = (fcode_forward>fcode_backward) ? fcode_forward : fcode_backward;
			int32_t intra_dc_threshold; /* fake variable */

			if (check_resync_marker(bs, fcode_max  - 1)) {
				int bound = read_video_packet_header(bs, dec, fcode_max - 1, &quant,
													 &fcode_forward, &fcode_backward, &intra_dc_threshold);
				x = bound % dec->mb_width;
				y = bound / dec->mb_width;
				/* reset predicted macroblocks */
				dec->p_fmv = dec->p_bmv = zeromv;
			}

			mv =
			mb->b_mvs[0] = mb->b_mvs[1] = mb->b_mvs[2] = mb->b_mvs[3] =
			mb->mvs[0] = mb->mvs[1] = mb->mvs[2] = mb->mvs[3] = zeromv;

			/*
			 * skip if the co-located P_VOP macroblock is not coded
			 * if not codec in co-located S_VOP macroblock is _not_
			 * automatically skipped
			 */

			if (last_mb->mode == MODE_NOT_CODED) {
				mb->cbp = 0;
				mb->mode = MODE_FORWARD;
				decoder_mbinter(dec, mb, mb->cbp, bs, 0);
				continue;
			}

			if (!BitstreamGetBit(bs)) {	/* modb=='0' */
				const uint8_t modb2 = BitstreamGetBit(bs);

				mb->mode = get_mbtype(bs);

				if (!modb2)		/* modb=='00' */
					mb->cbp = BitstreamGetBits(bs, 6);
				else
					mb->cbp = 0;

				if (mb->mode && mb->cbp) {
					quant += get_dbquant(bs);
					if (quant > 31)
						quant = 31;
					else if (quant < 1)
						quant = 1;
				}

				if (dec->interlacing) {
					if (mb->cbp) {
						mb->field_dct = BitstreamGetBit(bs);
						DPRINTF(XVID_DEBUG_MB,"decp: field_dct: %i\n", mb->field_dct);
					}

					if (mb->mode) {
						mb->field_pred = BitstreamGetBit(bs);
						DPRINTF(XVID_DEBUG_MB, "decp: field_pred: %i\n", mb->field_pred);

						if (mb->field_pred) {
							mb->field_for_top = BitstreamGetBit(bs);
							DPRINTF(XVID_DEBUG_MB,"decp: field_for_top: %i\n", mb->field_for_top);
							mb->field_for_bot = BitstreamGetBit(bs);
							DPRINTF(XVID_DEBUG_MB,"decp: field_for_bot: %i\n", mb->field_for_bot);
						}
					}
				}

			} else {
				mb->mode = MODE_DIRECT_NONE_MV;
				mb->cbp = 0;
			}

			switch (mb->mode) {
			case MODE_DIRECT:
				get_b_motion_vector(bs, &mv, 1, zeromv, dec, x, y);

			case MODE_DIRECT_NONE_MV:
				for (i = 0; i < 4; i++) {
					mb->mvs[i].x = last_mb->mvs[i].x*dec->time_bp/dec->time_pp + mv.x;
					mb->mvs[i].y = last_mb->mvs[i].y*dec->time_bp/dec->time_pp + mv.y;
					
					mb->b_mvs[i].x = (mv.x)
						?  mb->mvs[i].x - last_mb->mvs[i].x
						: last_mb->mvs[i].x*(dec->time_bp - dec->time_pp)/dec->time_pp;
					mb->b_mvs[i].y = (mv.y)
						? mb->mvs[i].y - last_mb->mvs[i].y
						: last_mb->mvs[i].y*(dec->time_bp - dec->time_pp)/dec->time_pp;
				}

				decoder_bf_interpolate_mbinter(dec, mb, bs, 1);
				break;

			case MODE_INTERPOLATE:
				get_b_motion_vector(bs, &mb->mvs[0], fcode_forward, dec->p_fmv, dec, x, y);
				dec->p_fmv = mb->mvs[1] = mb->mvs[2] = mb->mvs[3] =	mb->mvs[0];

				get_b_motion_vector(bs, &mb->b_mvs[0], fcode_backward, dec->p_bmv, dec, x, y);
				dec->p_bmv = mb->b_mvs[1] = mb->b_mvs[2] = mb->b_mvs[3] = mb->b_mvs[0];

				decoder_bf_interpolate_mbinter(dec, mb, bs, 0);
				break;

			case MODE_BACKWARD:
				get_b_motion_vector(bs, &mb->mvs[0], fcode_backward, dec->p_bmv, dec, x, y);
				dec->p_bmv = mb->mvs[1] = mb->mvs[2] = mb->mvs[3] =	mb->mvs[0];

				decoder_mbinter(dec, mb, mb->cbp, bs, 0);
				break;

			case MODE_FORWARD:
				get_b_motion_vector(bs, &mb->mvs[0], fcode_forward, dec->p_fmv, dec, x, y);
				dec->p_fmv = mb->mvs[1] = mb->mvs[2] = mb->mvs[3] =	mb->mvs[0];

				decoder_mbinter(dec, mb, mb->cbp, bs, 0);
				break;

			default:
				DPRINTF(XVID_DEBUG_ERROR,"Not supported B-frame mb_type = %i\n", mb->mode);
			}
		} /* End of for */
	}
}

/* perform post processing if necessary, and output the image */
static void decoder_output(DECODER * dec, xvid_dec_stats_t * stats, int coding_type)
{
	if (stats) {
		stats->type = coding2type(coding_type);
	}
}

static int
decoder_decode(DECODER * dec,
				xvid_dec_frame_t * frame, xvid_dec_stats_t * stats)
{

	Bitstream bs;
	uint32_t rounding;
	uint32_t reduced_resolution;
	uint32_t quant = 2;
	uint32_t fcode_forward;
	uint32_t fcode_backward;
	uint32_t intra_dc_threshold;
	WARPPOINTS gmc_warp;
	int coding_type;
	int success, output, seen_something;

	if (XVID_VERSION_MAJOR(frame->version) != 1 || (stats && XVID_VERSION_MAJOR(stats->version) != 1))	/* v1.x.x */
		return XVID_ERR_VERSION;

	dec->macro_intra       = 0;
	dec->macro_inter       = 0;
	dec->motion            = 0;
	dec->motion_global     = 0;
	
	stats->coding_type     = 0;

	dec->low_delay_default = (frame->general & XVID_LOWDELAY);
	if ((frame->general & XVID_DISCONTINUITY))
		dec->frames = 0;

	BitstreamInit(&bs, frame->bitstream, frame->length);

	success = 0;
	output = 0;
	seen_something = 0;

repeat:

	coding_type = BitstreamReadHeaders(&bs, dec, &rounding, &reduced_resolution,
			&quant, &fcode_forward, &fcode_backward, &intra_dc_threshold, &gmc_warp);

	DPRINTF(XVID_DEBUG_HEADER, "coding_type=%i,  packed=%i,  time=%lli,  time_pp=%i,  time_bp=%i\n",
							coding_type,	dec->packed_mode, dec->time, dec->time_pp, dec->time_bp);

	if (coding_type == -1) { /* nothing */
		if (success) goto done;
		if (stats) stats->type = XVID_TYPE_NOTHING;
		return BitstreamPos(&bs)/8;
	}

	if (coding_type == -2 || coding_type == -3) {	/* vol and/or resize */

		if (coding_type == -3)
			decoder_resize(dec);

		if (stats) {
			stats->type = XVID_TYPE_VOL;
			stats->data.vol.width = dec->width;
			stats->data.vol.height = dec->height;
			return BitstreamPos(&bs)/8;	/* number of bytes consumed */
		}
		goto repeat;
	}

	if(dec->frames == 0 && coding_type != I_VOP) {
		/* 1st frame is not an i-vop */
		goto repeat;
	}

	dec->p_bmv.x = dec->p_bmv.y = dec->p_fmv.y = dec->p_fmv.y = 0;	/* init pred vector to 0 */

	/* packed_mode: special-N_VOP treament */
	if (dec->packed_mode && coding_type == N_VOP) {
		if (dec->low_delay_default && dec->frames > 0) {
			decoder_output(dec, stats, dec->last_coding_type);
			output = 1;
		}
		/* ignore otherwise */
	} else if (coding_type != B_VOP) {
		switch(coding_type) {
		case I_VOP :
			decoder_iframe(dec, &bs, reduced_resolution, quant, intra_dc_threshold);
			break;
		case P_VOP :
			decoder_pframe(dec, &bs, rounding, reduced_resolution, quant,
						fcode_forward, intra_dc_threshold, NULL);
			break;
		case S_VOP :
			decoder_pframe(dec, &bs, rounding, reduced_resolution, quant,
						fcode_forward, intra_dc_threshold, &gmc_warp);
			break;
		case N_VOP :
			/* XXX: not_coded vops are not used for forward prediction */
			/* we should not swap(last_mbs,mbs) */
			SWAP(MACROBLOCK *, dec->mbs, dec->last_mbs); /* it will be swapped back */
			break;
		}

		/* note: for packed_mode, output is performed when the special-N_VOP is decoded */
		if (!(dec->low_delay_default && dec->packed_mode)) {
			if (dec->low_delay) {
				decoder_output(dec, stats, coding_type);
				output = 1;
			} else if (dec->frames > 0)	{ /* is the reference frame valid? */
				/* output the reference frame */
				decoder_output(dec, stats, dec->last_coding_type);
				output = 1;
			}
		}

		SWAP(MACROBLOCK *, dec->mbs, dec->last_mbs);
		dec->last_reduced_resolution = reduced_resolution;
		dec->last_coding_type = coding_type;

		dec->frames++;
		seen_something = 1;

	} else {	/* B_VOP */

		if (dec->low_delay) {
			DPRINTF(XVID_DEBUG_ERROR, "warning: bvop found in low_delay==1 stream\n");
			dec->low_delay = 0;
		}

		if (dec->frames < 2) {
			if (stats) stats->type = XVID_TYPE_NOTHING;
		} else if (dec->time_pp <= dec->time_bp) {
			if (stats) stats->type = XVID_TYPE_NOTHING;
		} else {
			decoder_bframe(dec, &bs, quant, fcode_forward, fcode_backward);
			decoder_output(dec, stats, coding_type);
		}

		output = 1;
		dec->frames++;
	}

#if 0 /* Avoids to read to much data because of 32bit reads in our BS functions */
	 BitstreamByteAlign(&bs);
#endif

	/* low_delay_default mode: repeat in packed_mode */
	if (dec->low_delay_default && dec->packed_mode && output == 0 && success == 0) {
		success = 1;
		goto repeat;
	}

done :

	stats->coding_type                = coding2type(coding_type);
	stats->data.vop.macro_intra       = dec->macro_intra;
	stats->data.vop.macro_inter       = dec->macro_inter;
	stats->data.vop.motion            = dec->motion;
	stats->data.vop.motion_global     = dec->motion_global;

	return (BitstreamPos(&bs) + 7) / 8;	/* number of bytes consumed */
}


/* FILE: xvidcore-1.0.3/src/bitstream/bitstream.c */

static const uint32_t intra_dc_threshold_table[] = {
	32,							/* never use */
	13,
	15,
	17,
	19,
	21,
	23,
	1,
};

static void
bs_get_matrix(Bitstream * bs,
			  uint8_t * matrix)
{
	int i = 0;
	int last, value = 0;

	do {
		last = value;
		value = BitstreamGetBits(bs, 8);
		i++;
	}
	while (value != 0 && i < 64);
}

/*
 * for PVOP addbits == fcode - 1
 * for BVOP addbits == max(fcode,bcode) - 1
 * returns mbpos
 */
static int
read_video_packet_header(Bitstream *bs,
						DECODER * dec,
						const int addbits,
						int * quant,
						int * fcode_forward,
						int  * fcode_backward,
						int * intra_dc_threshold)
{
	int startcode_bits = NUMBITS_VP_RESYNC_MARKER + addbits;
	int mbnum_bits = log2bin(dec->mb_width *  dec->mb_height - 1);
	int mbnum;
	int hec = 0;

	BitstreamSkip(bs, BitstreamNumBitsToByteAlign(bs));
	BitstreamSkip(bs, startcode_bits);

	DPRINTF(XVID_DEBUG_STARTCODE, "<video_packet_header>\n");

	if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR)
	{
		hec = BitstreamGetBit(bs);		/* header_extension_code */
		if (hec && !(dec->sprite_enable == SPRITE_STATIC /* && current_coding_type = I_VOP */))
		{
			BitstreamSkip(bs, 13);			/* vop_width */
			READ_MARKER();
			BitstreamSkip(bs, 13);			/* vop_height */
			READ_MARKER();
			BitstreamSkip(bs, 13);			/* vop_horizontal_mc_spatial_ref */
			READ_MARKER();
			BitstreamSkip(bs, 13);			/* vop_vertical_mc_spatial_ref */
			READ_MARKER();
		}
	}

	mbnum = BitstreamGetBits(bs, mbnum_bits);		/* macroblock_number */
	DPRINTF(XVID_DEBUG_HEADER, "mbnum %i\n", mbnum);

	if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY)
	{
		*quant = BitstreamGetBits(bs, dec->quant_bits);	/* quant_scale */
		DPRINTF(XVID_DEBUG_HEADER, "quant %i\n", *quant);
	}

	if (dec->shape == VIDOBJLAY_SHAPE_RECTANGULAR)
		hec = BitstreamGetBit(bs);		/* header_extension_code */


	DPRINTF(XVID_DEBUG_HEADER, "header_extension_code %i\n", hec);
	if (hec)
	{
		int time_base;
		int time_increment;
		int coding_type;

		for (time_base=0; BitstreamGetBit(bs)!=0; time_base++);		/* modulo_time_base */
		READ_MARKER();
		if (dec->time_inc_bits)
			time_increment = (BitstreamGetBits(bs, dec->time_inc_bits));	/* vop_time_increment */
		READ_MARKER();
		DPRINTF(XVID_DEBUG_HEADER,"time %i:%i\n", time_base, time_increment);

		coding_type = BitstreamGetBits(bs, 2);
		DPRINTF(XVID_DEBUG_HEADER,"coding_type %i\n", coding_type);

		if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR)
		{
			BitstreamSkip(bs, 1);	/* change_conv_ratio_disable */
			if (coding_type != I_VOP)
				BitstreamSkip(bs, 1);	/* vop_shape_coding_type */
		}

		if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY)
		{
			*intra_dc_threshold = intra_dc_threshold_table[BitstreamGetBits(bs, 3)];

			if (dec->sprite_enable == SPRITE_GMC && coding_type == S_VOP &&
				dec->sprite_warping_points > 0)
			{
				/* TODO: sprite trajectory */
			}
			if (dec->reduced_resolution_enable &&
				dec->shape == VIDOBJLAY_SHAPE_RECTANGULAR &&
				(coding_type == P_VOP || coding_type == I_VOP))
			{
				BitstreamSkip(bs, 1); /* XXX: vop_reduced_resolution */
			}

			if (coding_type != I_VOP && fcode_forward)
			{
				*fcode_forward = BitstreamGetBits(bs, 3);
				DPRINTF(XVID_DEBUG_HEADER,"fcode_forward %i\n", *fcode_forward);
			}

			if (coding_type == B_VOP && fcode_backward)
			{
				*fcode_backward = BitstreamGetBits(bs, 3);
				DPRINTF(XVID_DEBUG_HEADER,"fcode_backward %i\n", *fcode_backward);
			}
		}

	}

	if (dec->newpred_enable)
	{
		int vop_id;
		int vop_id_for_prediction;

		vop_id = BitstreamGetBits(bs, MIN(dec->time_inc_bits + 3, 15));
		DPRINTF(XVID_DEBUG_HEADER, "vop_id %i\n", vop_id);
		if (BitstreamGetBit(bs))	/* vop_id_for_prediction_indication */
		{
			vop_id_for_prediction = BitstreamGetBits(bs, MIN(dec->time_inc_bits + 3, 15));
			DPRINTF(XVID_DEBUG_HEADER, "vop_id_for_prediction %i\n", vop_id_for_prediction);
		}
		READ_MARKER();
	}

	return mbnum;
}

/* vol estimation header */
static void
read_vol_complexity_estimation_header(Bitstream * bs, DECODER * dec)
{
	ESTIMATION * e = &dec->estimation;

	e->method = BitstreamGetBits(bs, 2);	/* estimation_method */
	DPRINTF(XVID_DEBUG_HEADER,"+ complexity_estimation_header; method=%i\n", e->method);

	if (e->method == 0 || e->method == 1)
	{
		if (!BitstreamGetBit(bs))		/* shape_complexity_estimation_disable */
		{
			e->opaque = BitstreamGetBit(bs);		/* opaque */
			e->transparent = BitstreamGetBit(bs);		/* transparent */
			e->intra_cae = BitstreamGetBit(bs);		/* intra_cae */
			e->inter_cae = BitstreamGetBit(bs);		/* inter_cae */
			e->no_update = BitstreamGetBit(bs);		/* no_update */
			e->upsampling = BitstreamGetBit(bs);		/* upsampling */
		}

		if (!BitstreamGetBit(bs))	/* texture_complexity_estimation_set_1_disable */
		{
			e->intra_blocks = BitstreamGetBit(bs);		/* intra_blocks */
			e->inter_blocks = BitstreamGetBit(bs);		/* inter_blocks */
			e->inter4v_blocks = BitstreamGetBit(bs);		/* inter4v_blocks */
			e->not_coded_blocks = BitstreamGetBit(bs);		/* not_coded_blocks */
		}
	}

	READ_MARKER();

	if (!BitstreamGetBit(bs))		/* texture_complexity_estimation_set_2_disable */
	{
		e->dct_coefs = BitstreamGetBit(bs);		/* dct_coefs */
		e->dct_lines = BitstreamGetBit(bs);		/* dct_lines */
		e->vlc_symbols = BitstreamGetBit(bs);		/* vlc_symbols */
		e->vlc_bits = BitstreamGetBit(bs);		/* vlc_bits */
	}

	if (!BitstreamGetBit(bs))		/* motion_compensation_complexity_disable */
	{
		e->apm = BitstreamGetBit(bs);		/* apm */
		e->npm = BitstreamGetBit(bs);		/* npm */
		e->interpolate_mc_q = BitstreamGetBit(bs);		/* interpolate_mc_q */
		e->forw_back_mc_q = BitstreamGetBit(bs);		/* forw_back_mc_q */
		e->halfpel2 = BitstreamGetBit(bs);		/* halfpel2 */
		e->halfpel4 = BitstreamGetBit(bs);		/* halfpel4 */
	}

	READ_MARKER();

	if (e->method == 1)
	{
		if (!BitstreamGetBit(bs))	/* version2_complexity_estimation_disable */
		{
			e->sadct = BitstreamGetBit(bs);		/* sadct */
			e->quarterpel = BitstreamGetBit(bs);		/* quarterpel */
		}
	}
}

/* vop estimation header */
static void
read_vop_complexity_estimation_header(Bitstream * bs, DECODER * dec, int coding_type)
{
	ESTIMATION * e = &dec->estimation;

	if (e->method == 0 || e->method == 1)
	{
		if (coding_type == I_VOP) {
			if (e->opaque)		BitstreamSkip(bs, 8);	/* dcecs_opaque */
			if (e->transparent) BitstreamSkip(bs, 8);	/* */
			if (e->intra_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->no_update)	BitstreamSkip(bs, 8);	/* */
			if (e->upsampling)	BitstreamSkip(bs, 8);	/* */
			if (e->intra_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->not_coded_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->dct_coefs)	BitstreamSkip(bs, 8);	/* */
			if (e->dct_lines)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_symbols) BitstreamSkip(bs, 8);	/* */
			if (e->vlc_bits)	BitstreamSkip(bs, 8);	/* */
			if (e->sadct)		BitstreamSkip(bs, 8);	/* */
		}

		if (coding_type == P_VOP) {
			if (e->opaque) BitstreamSkip(bs, 8);		/* */
			if (e->transparent) BitstreamSkip(bs, 8);	/* */
			if (e->intra_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->no_update)	BitstreamSkip(bs, 8);	/* */
			if (e->upsampling) BitstreamSkip(bs, 8);	/* */
			if (e->intra_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->not_coded_blocks)	BitstreamSkip(bs, 8);	/* */
			if (e->dct_coefs)	BitstreamSkip(bs, 8);	/* */
			if (e->dct_lines)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_symbols) BitstreamSkip(bs, 8);	/* */
			if (e->vlc_bits)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->inter4v_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->apm)			BitstreamSkip(bs, 8);	/* */
			if (e->npm)			BitstreamSkip(bs, 8);	/* */
			if (e->forw_back_mc_q) BitstreamSkip(bs, 8);	/* */
			if (e->halfpel2)	BitstreamSkip(bs, 8);	/* */
			if (e->halfpel4)	BitstreamSkip(bs, 8);	/* */
			if (e->sadct)		BitstreamSkip(bs, 8);	/* */
			if (e->quarterpel)	BitstreamSkip(bs, 8);	/* */
		}
		if (coding_type == B_VOP) {
			if (e->opaque)		BitstreamSkip(bs, 8);	/* */
			if (e->transparent)	BitstreamSkip(bs, 8);	/* */
			if (e->intra_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->no_update)	BitstreamSkip(bs, 8);	/* */
			if (e->upsampling)	BitstreamSkip(bs, 8);	/* */
			if (e->intra_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->not_coded_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->dct_coefs)	BitstreamSkip(bs, 8);	/* */
			if (e->dct_lines)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_symbols)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_bits)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->inter4v_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->apm)			BitstreamSkip(bs, 8);	/* */
			if (e->npm)			BitstreamSkip(bs, 8);	/* */
			if (e->forw_back_mc_q) BitstreamSkip(bs, 8);	/* */
			if (e->halfpel2)	BitstreamSkip(bs, 8);	/* */
			if (e->halfpel4)	BitstreamSkip(bs, 8);	/* */
			if (e->interpolate_mc_q) BitstreamSkip(bs, 8);	/* */
			if (e->sadct)		BitstreamSkip(bs, 8);	/* */
			if (e->quarterpel)	BitstreamSkip(bs, 8);	/* */
		}

		if (coding_type == S_VOP && dec->sprite_enable == SPRITE_STATIC) {
			if (e->intra_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->not_coded_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->dct_coefs)	BitstreamSkip(bs, 8);	/* */
			if (e->dct_lines)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_symbols)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_bits)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->inter4v_blocks)	BitstreamSkip(bs, 8);	/* */
			if (e->apm)			BitstreamSkip(bs, 8);	/* */
			if (e->npm)			BitstreamSkip(bs, 8);	/* */
			if (e->forw_back_mc_q)	BitstreamSkip(bs, 8);	/* */
			if (e->halfpel2)	BitstreamSkip(bs, 8);	/* */
			if (e->halfpel4)	BitstreamSkip(bs, 8);	/* */
			if (e->interpolate_mc_q) BitstreamSkip(bs, 8);	/* */
		}
	}
}

/*
decode headers
returns coding_type, or -1 if error
*/

#define VIDOBJ_START_CODE_MASK		0x0000001f
#define VIDOBJLAY_START_CODE_MASK	0x0000000f

static int
BitstreamReadHeaders(Bitstream * bs,
					 DECODER * dec,
					 uint32_t * rounding,
					 uint32_t * reduced_resolution,
					 uint32_t * quant,
					 uint32_t * fcode_forward,
					 uint32_t * fcode_backward,
					 uint32_t * intra_dc_threshold,
					 WARPPOINTS *gmc_warp)
{
	uint32_t vol_ver_id;
	uint32_t coding_type;
	uint32_t start_code;
	uint32_t time_incr = 0;
	int32_t time_increment = 0;
	int resize = 0;

	while ((BitstreamPos(bs) >> 3) + 4 <= bs->length) {

		BitstreamByteAlign(bs);
		start_code = BitstreamShowBits(bs, 32);

		if (start_code == VISOBJSEQ_START_CODE) {

			int profile;

			DPRINTF(XVID_DEBUG_STARTCODE, "<visual_object_sequence>\n");

			BitstreamSkip(bs, 32);	/* visual_object_sequence_start_code */
			profile = BitstreamGetBits(bs, 8);	/* profile_and_level_indication */

			DPRINTF(XVID_DEBUG_HEADER, "profile_and_level_indication %i\n", profile);

		} else if (start_code == VISOBJSEQ_STOP_CODE) {

			BitstreamSkip(bs, 32);	/* visual_object_sequence_stop_code */

			DPRINTF(XVID_DEBUG_STARTCODE, "</visual_object_sequence>\n");

		} else if (start_code == VISOBJ_START_CODE) {
			int visobj_ver_id;

			DPRINTF(XVID_DEBUG_STARTCODE, "<visual_object>\n");

			BitstreamSkip(bs, 32);	/* visual_object_start_code */
			if (BitstreamGetBit(bs))	/* is_visual_object_identified */
			{
				visobj_ver_id = BitstreamGetBits(bs, 4);	/* visual_object_ver_id */
				DPRINTF(XVID_DEBUG_HEADER,"visobj_ver_id %i\n", visobj_ver_id);
				BitstreamSkip(bs, 3);	/* visual_object_priority */
			} else {
				visobj_ver_id = 1;
			}

			if (BitstreamShowBits(bs, 4) != VISOBJ_TYPE_VIDEO)	/* visual_object_type */
			{
				DPRINTF(XVID_DEBUG_ERROR, "visual_object_type != video\n");
				return -1;
			}
			BitstreamSkip(bs, 4);

			/* video_signal_type */

			if (BitstreamGetBit(bs))	/* video_signal_type */
			{
				DPRINTF(XVID_DEBUG_HEADER,"+ video_signal_type\n");
				BitstreamSkip(bs, 3);	/* video_format */
				BitstreamSkip(bs, 1);	/* video_range */
				if (BitstreamGetBit(bs))	/* color_description */
				{
					DPRINTF(XVID_DEBUG_HEADER,"+ color_description");
					BitstreamSkip(bs, 8);	/* color_primaries */
					BitstreamSkip(bs, 8);	/* transfer_characteristics */
					BitstreamSkip(bs, 8);	/* matrix_coefficients */
				}
			}
		} else if ((start_code & ~VIDOBJ_START_CODE_MASK) == VIDOBJ_START_CODE) {

			DPRINTF(XVID_DEBUG_STARTCODE, "<video_object>\n");
			DPRINTF(XVID_DEBUG_HEADER, "vo id %i\n", start_code & VIDOBJ_START_CODE_MASK);

			BitstreamSkip(bs, 32);	/* video_object_start_code */

		} else if ((start_code & ~VIDOBJLAY_START_CODE_MASK) == VIDOBJLAY_START_CODE) {

			DPRINTF(XVID_DEBUG_STARTCODE, "<video_object_layer>\n");
			DPRINTF(XVID_DEBUG_HEADER, "vol id %i\n", start_code & VIDOBJLAY_START_CODE_MASK);

			BitstreamSkip(bs, 32);	/* video_object_layer_start_code */
			BitstreamSkip(bs, 1);	/* random_accessible_vol */

            BitstreamSkip(bs, 8);   /* video_object_type_indication */

			if (BitstreamGetBit(bs))	/* is_object_layer_identifier */
			{
				DPRINTF(XVID_DEBUG_HEADER, "+ is_object_layer_identifier\n");
				vol_ver_id = BitstreamGetBits(bs, 4);	/* video_object_layer_verid */
				DPRINTF(XVID_DEBUG_HEADER,"ver_id %i\n", vol_ver_id);
				BitstreamSkip(bs, 3);	/* video_object_layer_priority */
			} else {
				vol_ver_id = 1;
			}

			dec->aspect_ratio = BitstreamGetBits(bs, 4);

			if (dec->aspect_ratio == VIDOBJLAY_AR_EXTPAR)	/* aspect_ratio_info */
			{
				DPRINTF(XVID_DEBUG_HEADER, "+ aspect_ratio_info\n");
				dec->par_width = BitstreamGetBits(bs, 8);	/* par_width */
				dec->par_height = BitstreamGetBits(bs, 8);	/* par_height */
			}

			if (BitstreamGetBit(bs))	/* vol_control_parameters */
			{
				DPRINTF(XVID_DEBUG_HEADER, "+ vol_control_parameters\n");
				BitstreamSkip(bs, 2);	/* chroma_format */
				dec->low_delay = BitstreamGetBit(bs);	/* low_delay */
				DPRINTF(XVID_DEBUG_HEADER, "low_delay %i\n", dec->low_delay);
				if (BitstreamGetBit(bs))	/* vbv_parameters */
				{
					unsigned int bitrate;
					unsigned int buffer_size;
					unsigned int occupancy;

					DPRINTF(XVID_DEBUG_HEADER,"+ vbv_parameters\n");

					bitrate = BitstreamGetBits(bs,15) << 15;	/* first_half_bit_rate */
					READ_MARKER();
					bitrate |= BitstreamGetBits(bs,15);		/* latter_half_bit_rate */
					READ_MARKER();

					buffer_size = BitstreamGetBits(bs, 15) << 3;	/* first_half_vbv_buffer_size */
					READ_MARKER();
					buffer_size |= BitstreamGetBits(bs, 3);		/* latter_half_vbv_buffer_size */

					occupancy = BitstreamGetBits(bs, 11) << 15;	/* first_half_vbv_occupancy */
					READ_MARKER();
					occupancy |= BitstreamGetBits(bs, 15);	/* latter_half_vbv_occupancy */
					READ_MARKER();

					DPRINTF(XVID_DEBUG_HEADER,"bitrate %d (unit=400 bps)\n", bitrate);
					DPRINTF(XVID_DEBUG_HEADER,"buffer_size %d (unit=16384 bits)\n", buffer_size);
					DPRINTF(XVID_DEBUG_HEADER,"occupancy %d (unit=64 bits)\n", occupancy);
				}
			}else{
				dec->low_delay = dec->low_delay_default;
			}

			dec->shape = BitstreamGetBits(bs, 2);	/* video_object_layer_shape */

			DPRINTF(XVID_DEBUG_HEADER, "shape %i\n", dec->shape);
			if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR)
			{
				DPRINTF(XVID_DEBUG_ERROR,"non-rectangular shapes are not supported\n");
			}

			if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE && vol_ver_id != 1) {
				BitstreamSkip(bs, 4);	/* video_object_layer_shape_extension */
			}

			READ_MARKER();

			/********************** for decode B-frame time ***********************/
			dec->time_inc_resolution = BitstreamGetBits(bs, 16);	/* vop_time_increment_resolution */
			DPRINTF(XVID_DEBUG_HEADER,"vop_time_increment_resolution %i\n", dec->time_inc_resolution);

#if 0
			dec->time_inc_resolution--;
#endif

			if (dec->time_inc_resolution > 0) {
				dec->time_inc_bits = MAX(log2bin(dec->time_inc_resolution-1), 1);
			} else {
#if 0
				dec->time_inc_bits = 0;
#endif
				/* for "old" xvid compatibility, set time_inc_bits = 1 */
				dec->time_inc_bits = 1;
			}

			READ_MARKER();

			if (BitstreamGetBit(bs))	/* fixed_vop_rate */
			{
				DPRINTF(XVID_DEBUG_HEADER, "+ fixed_vop_rate\n");
				BitstreamSkip(bs, dec->time_inc_bits);	/* fixed_vop_time_increment */
			}

			if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY) {

				if (dec->shape == VIDOBJLAY_SHAPE_RECTANGULAR) {
					uint32_t width, height;

					READ_MARKER();
					width = BitstreamGetBits(bs, 13);	/* video_object_layer_width */
					READ_MARKER();
					height = BitstreamGetBits(bs, 13);	/* video_object_layer_height */
					READ_MARKER();

					DPRINTF(XVID_DEBUG_HEADER, "width %i\n", width);
					DPRINTF(XVID_DEBUG_HEADER, "height %i\n", height);

					if (dec->width != width || dec->height != height)
					{
						if (dec->fixed_dimensions)
						{
							DPRINTF(XVID_DEBUG_ERROR, "decoder width/height does not match bitstream\n");
							return -1;
						}
						resize = 1;
						dec->width = width;
						dec->height = height;
					}
				}

				dec->interlacing = BitstreamGetBit(bs);
				DPRINTF(XVID_DEBUG_HEADER, "interlacing %i\n", dec->interlacing);

				if (!BitstreamGetBit(bs))	/* obmc_disable */
				{
					DPRINTF(XVID_DEBUG_ERROR, "obmc_disabled==false not supported\n");
					/* TODO */
					/* fucking divx4.02 has this enabled */
				}

				dec->sprite_enable = BitstreamGetBits(bs, (vol_ver_id == 1 ? 1 : 2));	/* sprite_enable */

				if (dec->sprite_enable == SPRITE_STATIC || dec->sprite_enable == SPRITE_GMC)
				{
					int low_latency_sprite_enable;

					if (dec->sprite_enable != SPRITE_GMC)
					{
						int sprite_width;
						int sprite_height;
						int sprite_left_coord;
						int sprite_top_coord;
						sprite_width = BitstreamGetBits(bs, 13);		/* sprite_width */
						READ_MARKER();
						sprite_height = BitstreamGetBits(bs, 13);	/* sprite_height */
						READ_MARKER();
						sprite_left_coord = BitstreamGetBits(bs, 13);	/* sprite_left_coordinate */
						READ_MARKER();
						sprite_top_coord = BitstreamGetBits(bs, 13);	/* sprite_top_coordinate */
						READ_MARKER();
					}
					dec->sprite_warping_points = BitstreamGetBits(bs, 6);		/* no_of_sprite_warping_points */
					dec->sprite_warping_accuracy = BitstreamGetBits(bs, 2);		/* sprite_warping_accuracy */
					dec->sprite_brightness_change = BitstreamGetBits(bs, 1);		/* brightness_change */
					if (dec->sprite_enable != SPRITE_GMC)
					{
						low_latency_sprite_enable = BitstreamGetBits(bs, 1);		/* low_latency_sprite_enable */
					}
				}

				if (vol_ver_id != 1 &&
					dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR) {
					BitstreamSkip(bs, 1);	/* sadct_disable */
				}

				if (BitstreamGetBit(bs))	/* not_8_bit */
				{
					DPRINTF(XVID_DEBUG_HEADER, "not_8_bit==true (ignored)\n");
					dec->quant_bits = BitstreamGetBits(bs, 4);	/* quant_precision */
					BitstreamSkip(bs, 4);	/* bits_per_pixel */
				} else {
					dec->quant_bits = 5;
				}

				if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE) {
					BitstreamSkip(bs, 1);	/* no_gray_quant_update */
					BitstreamSkip(bs, 1);	/* composition_method */
					BitstreamSkip(bs, 1);	/* linear_composition */
				}

				dec->quant_type = BitstreamGetBit(bs);	/* quant_type */
				DPRINTF(XVID_DEBUG_HEADER, "quant_type %i\n", dec->quant_type);

				if (dec->quant_type) {
					if (BitstreamGetBit(bs))	/* load_intra_quant_mat */
					{
						uint8_t matrix[64];

						DPRINTF(XVID_DEBUG_HEADER, "load_intra_quant_mat\n");

						bs_get_matrix(bs, matrix);
					}

					if (BitstreamGetBit(bs))	/* load_inter_quant_mat */
					{
						uint8_t matrix[64];

						DPRINTF(XVID_DEBUG_HEADER, "load_inter_quant_mat\n");

						bs_get_matrix(bs, matrix);
					}

					if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE) {
						DPRINTF(XVID_DEBUG_ERROR, "greyscale matrix not supported\n");
						return -1;
					}

				}


				if (vol_ver_id != 1) {
					dec->quarterpel = BitstreamGetBit(bs);	/* quarter_sample */
					DPRINTF(XVID_DEBUG_HEADER,"quarterpel %i\n", dec->quarterpel);
				}
				else
					dec->quarterpel = 0;


				dec->complexity_estimation_disable = BitstreamGetBit(bs);	/* complexity estimation disable */
				if (!dec->complexity_estimation_disable)
				{
					read_vol_complexity_estimation_header(bs, dec);
				}

				BitstreamSkip(bs, 1);	/* resync_marker_disable */

				if (BitstreamGetBit(bs))	/* data_partitioned */
				{
					DPRINTF(XVID_DEBUG_ERROR, "data_partitioned not supported\n");
					BitstreamSkip(bs, 1);	/* reversible_vlc */
				}

				if (vol_ver_id != 1) {
					dec->newpred_enable = BitstreamGetBit(bs);
					if (dec->newpred_enable)	/* newpred_enable */
					{
						DPRINTF(XVID_DEBUG_HEADER, "+ newpred_enable\n");
						BitstreamSkip(bs, 2);	/* requested_upstream_message_type */
						BitstreamSkip(bs, 1);	/* newpred_segment_type */
					}
					dec->reduced_resolution_enable = BitstreamGetBit(bs);	/* reduced_resolution_vop_enable */
					DPRINTF(XVID_DEBUG_HEADER, "reduced_resolution_enable %i\n", dec->reduced_resolution_enable);
				}
				else
				{
					dec->newpred_enable = 0;
					dec->reduced_resolution_enable = 0;
				}

				dec->scalability = BitstreamGetBit(bs);	/* scalability */
				if (dec->scalability)
				{
					DPRINTF(XVID_DEBUG_ERROR, "scalability not supported\n");
					BitstreamSkip(bs, 1);	/* hierarchy_type */
					BitstreamSkip(bs, 4);	/* ref_layer_id */
					BitstreamSkip(bs, 1);	/* ref_layer_sampling_direc */
					BitstreamSkip(bs, 5);	/* hor_sampling_factor_n */
					BitstreamSkip(bs, 5);	/* hor_sampling_factor_m */
					BitstreamSkip(bs, 5);	/* vert_sampling_factor_n */
					BitstreamSkip(bs, 5);	/* vert_sampling_factor_m */
					BitstreamSkip(bs, 1);	/* enhancement_type */
					if(dec->shape == VIDOBJLAY_SHAPE_BINARY /* && hierarchy_type==0 */) {
						BitstreamSkip(bs, 1);	/* use_ref_shape */
						BitstreamSkip(bs, 1);	/* use_ref_texture */
						BitstreamSkip(bs, 5);	/* shape_hor_sampling_factor_n */
						BitstreamSkip(bs, 5);	/* shape_hor_sampling_factor_m */
						BitstreamSkip(bs, 5);	/* shape_vert_sampling_factor_n */
						BitstreamSkip(bs, 5);	/* shape_vert_sampling_factor_m */
					}
					return -1;
				}
			} else				/* dec->shape == BINARY_ONLY */
			{
				if (vol_ver_id != 1) {
					dec->scalability = BitstreamGetBit(bs); /* scalability */
					if (dec->scalability)
					{
						DPRINTF(XVID_DEBUG_ERROR, "scalability not supported\n");
						BitstreamSkip(bs, 4);	/* ref_layer_id */
						BitstreamSkip(bs, 5);	/* hor_sampling_factor_n */
						BitstreamSkip(bs, 5);	/* hor_sampling_factor_m */
						BitstreamSkip(bs, 5);	/* vert_sampling_factor_n */
						BitstreamSkip(bs, 5);	/* vert_sampling_factor_m */
						return -1;
					}
				}
				BitstreamSkip(bs, 1);	/* resync_marker_disable */

			}

			return (resize ? -3 : -2 );	/* VOL */

		} else if (start_code == GRPOFVOP_START_CODE) {

			DPRINTF(XVID_DEBUG_STARTCODE, "<group_of_vop>\n");

			BitstreamSkip(bs, 32);
			{
				int hours, minutes, seconds;

				hours = BitstreamGetBits(bs, 5);
				minutes = BitstreamGetBits(bs, 6);
				READ_MARKER();
				seconds = BitstreamGetBits(bs, 6);

				DPRINTF(XVID_DEBUG_HEADER, "time %ih%im%is\n", hours,minutes,seconds);
			}
			BitstreamSkip(bs, 1);	/* closed_gov */
			BitstreamSkip(bs, 1);	/* broken_link */

		} else if (start_code == VOP_START_CODE) {

			DPRINTF(XVID_DEBUG_STARTCODE, "<vop>\n");

			BitstreamSkip(bs, 32);	/* vop_start_code */

			coding_type = BitstreamGetBits(bs, 2);	/* vop_coding_type */
			DPRINTF(XVID_DEBUG_HEADER, "coding_type %i\n", coding_type);

			/*********************** for decode B-frame time ***********************/
			while (BitstreamGetBit(bs) != 0)	/* time_base */
				time_incr++;

			READ_MARKER();

			if (dec->time_inc_bits) {
				time_increment = (BitstreamGetBits(bs, dec->time_inc_bits));	/* vop_time_increment */
			}

			DPRINTF(XVID_DEBUG_HEADER, "time_base %i\n", time_incr);
			DPRINTF(XVID_DEBUG_HEADER, "time_increment %i\n", time_increment);

			DPRINTF(XVID_DEBUG_TIMECODE, "%c %i:%i\n",
				coding_type == I_VOP ? 'I' : coding_type == P_VOP ? 'P' : coding_type == B_VOP ? 'B' : 'S',
				time_incr, time_increment);

			if (coding_type != B_VOP) {
				dec->last_time_base = dec->time_base;
				dec->time_base += time_incr;
				dec->time = dec->time_base*dec->time_inc_resolution + time_increment;
				dec->time_pp = (int32_t)(dec->time - dec->last_non_b_time);
				dec->last_non_b_time = dec->time;
			} else {
				dec->time = (dec->last_time_base + time_incr)*dec->time_inc_resolution + time_increment;
				dec->time_bp = dec->time_pp - (int32_t)(dec->last_non_b_time - dec->time);
			}
			DPRINTF(XVID_DEBUG_HEADER,"time_pp=%i\n", dec->time_pp);
			DPRINTF(XVID_DEBUG_HEADER,"time_bp=%i\n", dec->time_bp);

			READ_MARKER();

			if (!BitstreamGetBit(bs))	/* vop_coded */
			{
				DPRINTF(XVID_DEBUG_HEADER, "vop_coded==false\n");
				return N_VOP;
			}

			if (dec->newpred_enable)
			{
				int vop_id;
				int vop_id_for_prediction;

				vop_id = BitstreamGetBits(bs, MIN(dec->time_inc_bits + 3, 15));
				DPRINTF(XVID_DEBUG_HEADER, "vop_id %i\n", vop_id);
				if (BitstreamGetBit(bs))	/* vop_id_for_prediction_indication */
				{
					vop_id_for_prediction = BitstreamGetBits(bs, MIN(dec->time_inc_bits + 3, 15));
					DPRINTF(XVID_DEBUG_HEADER, "vop_id_for_prediction %i\n", vop_id_for_prediction);
				}
				READ_MARKER();
			}



			/* fix a little bug by MinChen <chenm002@163.com> */
			if ((dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY) &&
				( (coding_type == P_VOP) || (coding_type == S_VOP && dec->sprite_enable == SPRITE_GMC) ) ) {
				*rounding = BitstreamGetBit(bs);	/* rounding_type */
				DPRINTF(XVID_DEBUG_HEADER, "rounding %i\n", *rounding);
			}

			if (dec->reduced_resolution_enable &&
				dec->shape == VIDOBJLAY_SHAPE_RECTANGULAR &&
				(coding_type == P_VOP || coding_type == I_VOP)) {

				*reduced_resolution = BitstreamGetBit(bs);
				DPRINTF(XVID_DEBUG_HEADER, "reduced_resolution %i\n", *reduced_resolution);
			}
			else
			{
				*reduced_resolution = 0;
			}

			if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR) {
				if(!(dec->sprite_enable == SPRITE_STATIC && coding_type == I_VOP)) {

					uint32_t width, height;
					uint32_t horiz_mc_ref, vert_mc_ref;

					width = BitstreamGetBits(bs, 13);
					READ_MARKER();
					height = BitstreamGetBits(bs, 13);
					READ_MARKER();
					horiz_mc_ref = BitstreamGetBits(bs, 13);
					READ_MARKER();
					vert_mc_ref = BitstreamGetBits(bs, 13);
					READ_MARKER();

					DPRINTF(XVID_DEBUG_HEADER, "width %i\n", width);
					DPRINTF(XVID_DEBUG_HEADER, "height %i\n", height);
					DPRINTF(XVID_DEBUG_HEADER, "horiz_mc_ref %i\n", horiz_mc_ref);
					DPRINTF(XVID_DEBUG_HEADER, "vert_mc_ref %i\n", vert_mc_ref);
				}

				BitstreamSkip(bs, 1);	/* change_conv_ratio_disable */
				if (BitstreamGetBit(bs))	/* vop_constant_alpha */
				{
					BitstreamSkip(bs, 8);	/* vop_constant_alpha_value */
				}
			}

			if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY) {

				if (!dec->complexity_estimation_disable)
				{
					read_vop_complexity_estimation_header(bs, dec, coding_type);
				}

				/* intra_dc_vlc_threshold */
				*intra_dc_threshold =
					intra_dc_threshold_table[BitstreamGetBits(bs, 3)];

				dec->top_field_first = 0;
				dec->alternate_vertical_scan = 0;

				if (dec->interlacing) {
					dec->top_field_first = BitstreamGetBit(bs);
					DPRINTF(XVID_DEBUG_HEADER, "interlace top_field_first %i\n", dec->top_field_first);
					dec->alternate_vertical_scan = BitstreamGetBit(bs);
					DPRINTF(XVID_DEBUG_HEADER, "interlace alternate_vertical_scan %i\n", dec->alternate_vertical_scan);

				}
			}

			if ((dec->sprite_enable == SPRITE_STATIC || dec->sprite_enable== SPRITE_GMC) && coding_type == S_VOP) {

				int i;

				for (i = 0 ; i < dec->sprite_warping_points; i++)
				{
					int length;
					int x = 0, y = 0;

					/* sprite code borowed from ffmpeg; thx Michael Niedermayer <michaelni@gmx.at> */
					length = bs_get_spritetrajectory(bs);
					if(length){
						x= BitstreamGetBits(bs, length);
						if ((x >> (length - 1)) == 0) /* if MSB not set it is negative*/
							x = - (x ^ ((1 << length) - 1));
					}
					READ_MARKER();

					length = bs_get_spritetrajectory(bs);
					if(length){
						y = BitstreamGetBits(bs, length);
						if ((y >> (length - 1)) == 0) /* if MSB not set it is negative*/
							y = - (y ^ ((1 << length) - 1));
					}
					READ_MARKER();

					gmc_warp->duv[i].x = x;
					gmc_warp->duv[i].y = y;

					DPRINTF(XVID_DEBUG_HEADER,"sprite_warping_point[%i] xy=(%i,%i)\n", i, x, y);
				}

				if (dec->sprite_brightness_change)
				{
					/* XXX: brightness_change_factor() */
				}
				if (dec->sprite_enable == SPRITE_STATIC)
				{
					/* XXX: todo */
				}

			}

			if ((*quant = BitstreamGetBits(bs, dec->quant_bits)) < 1)	/* vop_quant */
				*quant = 1;
			DPRINTF(XVID_DEBUG_HEADER, "quant %i\n", *quant);

			if (coding_type != I_VOP) {
				*fcode_forward = BitstreamGetBits(bs, 3);	/* fcode_forward */
				DPRINTF(XVID_DEBUG_HEADER, "fcode_forward %i\n", *fcode_forward);
			}

			if (coding_type == B_VOP) {
				*fcode_backward = BitstreamGetBits(bs, 3);	/* fcode_backward */
				DPRINTF(XVID_DEBUG_HEADER, "fcode_backward %i\n", *fcode_backward);
			}
			if (!dec->scalability) {
				if ((dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR) &&
					(coding_type != I_VOP)) {
					BitstreamSkip(bs, 1);	/* vop_shape_coding_type */
				}
			}
			return coding_type;

		} else if (start_code == USERDATA_START_CODE) {
			char tmp[256];
		    int i, version, build;
			char packed;

			BitstreamSkip(bs, 32);	/* user_data_start_code */

			tmp[0] = BitstreamShowBits(bs, 8);

			for(i = 1; i < 256; i++){
				tmp[i] = (BitstreamShowBits(bs, 16) & 0xFF);

				if(tmp[i] == 0)
					break;

				BitstreamSkip(bs, 8);
			}

			DPRINTF(XVID_DEBUG_STARTCODE, "<user_data>: %s\n", tmp);

			/* read xvid bitstream version */
			if(strncmp(tmp, "XviD", 4) == 0) {
				if (tmp[strlen(tmp)-1] == 'C') {				
					sscanf(tmp, "XviD%dC", &dec->bs_version);
					dec->cartoon_mode = 1;
				}
				else
					sscanf(tmp, "XviD%d", &dec->bs_version);

				DPRINTF(XVID_DEBUG_HEADER, "xvid bitstream version=%i\n", dec->bs_version);
			}

		    /* divx detection */
			i = sscanf(tmp, "DivX%dBuild%d%c", &version, &build, &packed);
			if (i < 2)
				i = sscanf(tmp, "DivX%db%d%c", &version, &build, &packed);

			if (i >= 2)
			{
				dec->packed_mode = (i == 3 && packed == 'p');
				DPRINTF(XVID_DEBUG_HEADER, "divx version=%i, build=%i packed=%i\n",
						version, build, dec->packed_mode);
			}

		} else					/* start_code == ? */
		{
			if (BitstreamShowBits(bs, 24) == 0x000001) {
				DPRINTF(XVID_DEBUG_STARTCODE, "<unknown: %x>\n", BitstreamShowBits(bs, 32));
			}
			BitstreamSkip(bs, 8);
		}
	}

#if 0
	DPRINTF("*** WARNING: no vop_start_code found");
#endif
	return -1;					/* ignore it */
}


/* FILE: xvidcore-1.0.3/src/bitstream/mbcoding.c */

/*****************************************************************************
 * VLC tables and other constant arrays
 ****************************************************************************/

static
VLC_TABLE const coeff_tab[2][102] =
{
	/* intra = 0 */
	{
		{{ 2,  2}, {0, 0, 1}},
		{{15,  4}, {0, 0, 2}},
		{{21,  6}, {0, 0, 3}},
		{{23,  7}, {0, 0, 4}},
		{{31,  8}, {0, 0, 5}},
		{{37,  9}, {0, 0, 6}},
		{{36,  9}, {0, 0, 7}},
		{{33, 10}, {0, 0, 8}},
		{{32, 10}, {0, 0, 9}},
		{{ 7, 11}, {0, 0, 10}},
		{{ 6, 11}, {0, 0, 11}},
		{{32, 11}, {0, 0, 12}},
		{{ 6,  3}, {0, 1, 1}},
		{{20,  6}, {0, 1, 2}},
		{{30,  8}, {0, 1, 3}},
		{{15, 10}, {0, 1, 4}},
		{{33, 11}, {0, 1, 5}},
		{{80, 12}, {0, 1, 6}},
		{{14,  4}, {0, 2, 1}},
		{{29,  8}, {0, 2, 2}},
		{{14, 10}, {0, 2, 3}},
		{{81, 12}, {0, 2, 4}},
		{{13,  5}, {0, 3, 1}},
		{{35,  9}, {0, 3, 2}},
		{{13, 10}, {0, 3, 3}},
		{{12,  5}, {0, 4, 1}},
		{{34,  9}, {0, 4, 2}},
		{{82, 12}, {0, 4, 3}},
		{{11,  5}, {0, 5, 1}},
		{{12, 10}, {0, 5, 2}},
		{{83, 12}, {0, 5, 3}},
		{{19,  6}, {0, 6, 1}},
		{{11, 10}, {0, 6, 2}},
		{{84, 12}, {0, 6, 3}},
		{{18,  6}, {0, 7, 1}},
		{{10, 10}, {0, 7, 2}},
		{{17,  6}, {0, 8, 1}},
		{{ 9, 10}, {0, 8, 2}},
		{{16,  6}, {0, 9, 1}},
		{{ 8, 10}, {0, 9, 2}},
		{{22,  7}, {0, 10, 1}},
		{{85, 12}, {0, 10, 2}},
		{{21,  7}, {0, 11, 1}},
		{{20,  7}, {0, 12, 1}},
		{{28,  8}, {0, 13, 1}},
		{{27,  8}, {0, 14, 1}},
		{{33,  9}, {0, 15, 1}},
		{{32,  9}, {0, 16, 1}},
		{{31,  9}, {0, 17, 1}},
		{{30,  9}, {0, 18, 1}},
		{{29,  9}, {0, 19, 1}},
		{{28,  9}, {0, 20, 1}},
		{{27,  9}, {0, 21, 1}},
		{{26,  9}, {0, 22, 1}},
		{{34, 11}, {0, 23, 1}},
		{{35, 11}, {0, 24, 1}},
		{{86, 12}, {0, 25, 1}},
		{{87, 12}, {0, 26, 1}},
		{{ 7,  4}, {1, 0, 1}},
		{{25,  9}, {1, 0, 2}},
		{{ 5, 11}, {1, 0, 3}},
		{{15,  6}, {1, 1, 1}},
		{{ 4, 11}, {1, 1, 2}},
		{{14,  6}, {1, 2, 1}},
		{{13,  6}, {1, 3, 1}},
		{{12,  6}, {1, 4, 1}},
		{{19,  7}, {1, 5, 1}},
		{{18,  7}, {1, 6, 1}},
		{{17,  7}, {1, 7, 1}},
		{{16,  7}, {1, 8, 1}},
		{{26,  8}, {1, 9, 1}},
		{{25,  8}, {1, 10, 1}},
		{{24,  8}, {1, 11, 1}},
		{{23,  8}, {1, 12, 1}},
		{{22,  8}, {1, 13, 1}},
		{{21,  8}, {1, 14, 1}},
		{{20,  8}, {1, 15, 1}},
		{{19,  8}, {1, 16, 1}},
		{{24,  9}, {1, 17, 1}},
		{{23,  9}, {1, 18, 1}},
		{{22,  9}, {1, 19, 1}},
		{{21,  9}, {1, 20, 1}},
		{{20,  9}, {1, 21, 1}},
		{{19,  9}, {1, 22, 1}},
		{{18,  9}, {1, 23, 1}},
		{{17,  9}, {1, 24, 1}},
		{{ 7, 10}, {1, 25, 1}},
		{{ 6, 10}, {1, 26, 1}},
		{{ 5, 10}, {1, 27, 1}},
		{{ 4, 10}, {1, 28, 1}},
		{{36, 11}, {1, 29, 1}},
		{{37, 11}, {1, 30, 1}},
		{{38, 11}, {1, 31, 1}},
		{{39, 11}, {1, 32, 1}},
		{{88, 12}, {1, 33, 1}},
		{{89, 12}, {1, 34, 1}},
		{{90, 12}, {1, 35, 1}},
		{{91, 12}, {1, 36, 1}},
		{{92, 12}, {1, 37, 1}},
		{{93, 12}, {1, 38, 1}},
		{{94, 12}, {1, 39, 1}},
		{{95, 12}, {1, 40, 1}}
	},
	/* intra = 1 */
	{
		{{ 2,  2}, {0, 0, 1}},
		{{15,  4}, {0, 0, 3}},
		{{21,  6}, {0, 0, 6}},
		{{23,  7}, {0, 0, 9}},
		{{31,  8}, {0, 0, 10}},
		{{37,  9}, {0, 0, 13}},
		{{36,  9}, {0, 0, 14}},
		{{33, 10}, {0, 0, 17}},
		{{32, 10}, {0, 0, 18}},
		{{ 7, 11}, {0, 0, 21}},
		{{ 6, 11}, {0, 0, 22}},
		{{32, 11}, {0, 0, 23}},
		{{ 6,  3}, {0, 0, 2}},
		{{20,  6}, {0, 1, 2}},
		{{30,  8}, {0, 0, 11}},
		{{15, 10}, {0, 0, 19}},
		{{33, 11}, {0, 0, 24}},
		{{80, 12}, {0, 0, 25}},
		{{14,  4}, {0, 1, 1}},
		{{29,  8}, {0, 0, 12}},
		{{14, 10}, {0, 0, 20}},
		{{81, 12}, {0, 0, 26}},
		{{13,  5}, {0, 0, 4}},
		{{35,  9}, {0, 0, 15}},
		{{13, 10}, {0, 1, 7}},
		{{12,  5}, {0, 0, 5}},
		{{34,  9}, {0, 4, 2}},
		{{82, 12}, {0, 0, 27}},
		{{11,  5}, {0, 2, 1}},
		{{12, 10}, {0, 2, 4}},
		{{83, 12}, {0, 1, 9}},
		{{19,  6}, {0, 0, 7}},
		{{11, 10}, {0, 3, 4}},
		{{84, 12}, {0, 6, 3}},
		{{18,  6}, {0, 0, 8}},
		{{10, 10}, {0, 4, 3}},
		{{17,  6}, {0, 3, 1}},
		{{ 9, 10}, {0, 8, 2}},
		{{16,  6}, {0, 4, 1}},
		{{ 8, 10}, {0, 5, 3}},
		{{22,  7}, {0, 1, 3}},
		{{85, 12}, {0, 1, 10}},
		{{21,  7}, {0, 2, 2}},
		{{20,  7}, {0, 7, 1}},
		{{28,  8}, {0, 1, 4}},
		{{27,  8}, {0, 3, 2}},
		{{33,  9}, {0, 0, 16}},
		{{32,  9}, {0, 1, 5}},
		{{31,  9}, {0, 1, 6}},
		{{30,  9}, {0, 2, 3}},
		{{29,  9}, {0, 3, 3}},
		{{28,  9}, {0, 5, 2}},
		{{27,  9}, {0, 6, 2}},
		{{26,  9}, {0, 7, 2}},
		{{34, 11}, {0, 1, 8}},
		{{35, 11}, {0, 9, 2}},
		{{86, 12}, {0, 2, 5}},
		{{87, 12}, {0, 7, 3}},
		{{ 7,  4}, {1, 0, 1}},
		{{25,  9}, {0, 11, 1}},
		{{ 5, 11}, {1, 0, 6}},
		{{15,  6}, {1, 1, 1}},
		{{ 4, 11}, {1, 0, 7}},
		{{14,  6}, {1, 2, 1}},
		{{13,  6}, {0, 5, 1}},
		{{12,  6}, {1, 0, 2}},
		{{19,  7}, {1, 5, 1}},
		{{18,  7}, {0, 6, 1}},
		{{17,  7}, {1, 3, 1}},
		{{16,  7}, {1, 4, 1}},
		{{26,  8}, {1, 9, 1}},
		{{25,  8}, {0, 8, 1}},
		{{24,  8}, {0, 9, 1}},
		{{23,  8}, {0, 10, 1}},
		{{22,  8}, {1, 0, 3}},
		{{21,  8}, {1, 6, 1}},
		{{20,  8}, {1, 7, 1}},
		{{19,  8}, {1, 8, 1}},
		{{24,  9}, {0, 12, 1}},
		{{23,  9}, {1, 0, 4}},
		{{22,  9}, {1, 1, 2}},
		{{21,  9}, {1, 10, 1}},
		{{20,  9}, {1, 11, 1}},
		{{19,  9}, {1, 12, 1}},
		{{18,  9}, {1, 13, 1}},
		{{17,  9}, {1, 14, 1}},
		{{ 7, 10}, {0, 13, 1}},
		{{ 6, 10}, {1, 0, 5}},
		{{ 5, 10}, {1, 1, 3}},
		{{ 4, 10}, {1, 2, 2}},
		{{36, 11}, {1, 3, 2}},
		{{37, 11}, {1, 4, 2}},
		{{38, 11}, {1, 15, 1}},
		{{39, 11}, {1, 16, 1}},
		{{88, 12}, {0, 14, 1}},
		{{89, 12}, {1, 0, 8}},
		{{90, 12}, {1, 5, 2}},
		{{91, 12}, {1, 6, 2}},
		{{92, 12}, {1, 17, 1}},
		{{93, 12}, {1, 18, 1}},
		{{94, 12}, {1, 19, 1}},
		{{95, 12}, {1, 20, 1}}
	}
};

/* constants taken from momusys/vm_common/inlcude/max_level.h */
static
uint8_t const max_level[2][2][64] = {
	{
		/* intra = 0, last = 0 */
		{
			12, 6, 4, 3, 3, 3, 3, 2,
			2, 2, 2, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0
		},
		/* intra = 0, last = 1 */
		{
			3, 2, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0
		}
	},
	{
		/* intra = 1, last = 0 */
		{
			27, 10, 5, 4, 3, 3, 3, 3,
			2, 2, 1, 1, 1, 1, 1, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0
		},
		/* intra = 1, last = 1 */
		{
			8, 3, 2, 2, 2, 2, 2, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0
		}
	}
};

static
uint8_t const max_run[2][2][64] = {
	{
		/* intra = 0, last = 0 */
		{
			0, 26, 10, 6, 2, 1, 1, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
		},
		/* intra = 0, last = 1 */
		{
			0, 40, 1, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
		}
	},
	{
		/* intra = 1, last = 0 */
		{
			0, 14, 9, 7, 3, 2, 1, 1,
			1, 1, 1, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
		},
		/* intra = 1, last = 1 */
		{
			0, 20, 6, 1, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
		}
	}
};

/******************************************************************
 * encoder tables                                                 *
 ******************************************************************/

static
VLC sprite_trajectory_code[32768];

static
VLC sprite_trajectory_len[15] = {
	{ 0x00 , 2},
	{ 0x02 , 3}, { 0x03, 3}, { 0x04, 3}, { 0x05, 3}, { 0x06, 3},
	{ 0x0E , 4}, { 0x1E, 5}, { 0x3E, 6}, { 0x7E, 7}, { 0xFE, 8},
	{ 0x1FE, 9}, {0x3FE,10}, {0x7FE,11}, {0xFFE,12} };

/******************************************************************
 * decoder tables                                                 *
 ******************************************************************/

static
VLC const mcbpc_intra_table[64] = {
	{-1, 0}, {20, 6}, {36, 6}, {52, 6}, {4, 4},  {4, 4},  {4, 4},  {4, 4},
	{19, 3}, {19, 3}, {19, 3}, {19, 3}, {19, 3}, {19, 3}, {19, 3}, {19, 3},
	{35, 3}, {35, 3}, {35, 3}, {35, 3}, {35, 3}, {35, 3}, {35, 3}, {35, 3},
	{51, 3}, {51, 3}, {51, 3}, {51, 3}, {51, 3}, {51, 3}, {51, 3}, {51, 3},
	{3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},
	{3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},
	{3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},
	{3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1}
};

static
VLC const mcbpc_inter_table[257] = {
	{VLC_ERROR, 0}, {255, 9}, {52, 9}, {36, 9}, {20, 9}, {49, 9}, {35, 8}, {35, 8},
	{19, 8}, {19, 8}, {50, 8}, {50, 8}, {51, 7}, {51, 7}, {51, 7}, {51, 7},
	{34, 7}, {34, 7}, {34, 7}, {34, 7}, {18, 7}, {18, 7}, {18, 7}, {18, 7},
	{33, 7}, {33, 7}, {33, 7}, {33, 7}, {17, 7}, {17, 7}, {17, 7}, {17, 7},
	{4, 6}, {4, 6}, {4, 6}, {4, 6}, {4, 6}, {4, 6}, {4, 6}, {4, 6},
	{48, 6}, {48, 6}, {48, 6}, {48, 6}, {48, 6}, {48, 6}, {48, 6}, {48, 6},
	{3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5},
	{3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5},
	{32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4},
	{32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4},
	{32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4},
	{32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4},
	{16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4},
	{16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4},
	{16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4},
	{16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{0, 1}
};

static
VLC const cbpy_table[64] = {
	{-1, 0}, {-1, 0}, {6, 6},  {9, 6},  {8, 5},  {8, 5},  {4, 5},  {4, 5},
	{2, 5},  {2, 5},  {1, 5},  {1, 5},  {0, 4},  {0, 4},  {0, 4},  {0, 4},
	{12, 4}, {12, 4}, {12, 4}, {12, 4}, {10, 4}, {10, 4}, {10, 4}, {10, 4},
	{14, 4}, {14, 4}, {14, 4}, {14, 4}, {5, 4},  {5, 4},  {5, 4},  {5, 4},
	{13, 4}, {13, 4}, {13, 4}, {13, 4}, {3, 4},  {3, 4},  {3, 4},  {3, 4},
	{11, 4}, {11, 4}, {11, 4}, {11, 4}, {7, 4},  {7, 4},  {7, 4},  {7, 4},
	{15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2},
	{15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2}
};

static
VLC const TMNMVtab0[] = {
	{3, 4}, {-3, 4}, {2, 3}, {2, 3}, {-2, 3}, {-2, 3}, {1, 2},
	{1, 2}, {1, 2}, {1, 2}, {-1, 2}, {-1, 2}, {-1, 2}, {-1, 2}
};

static
VLC const TMNMVtab1[] = {
	{12, 10}, {-12, 10}, {11, 10}, {-11, 10},
	{10, 9}, {10, 9}, {-10, 9}, {-10, 9},
	{9, 9}, {9, 9}, {-9, 9}, {-9, 9},
	{8, 9}, {8, 9}, {-8, 9}, {-8, 9},
	{7, 7}, {7, 7}, {7, 7}, {7, 7},
	{7, 7}, {7, 7}, {7, 7}, {7, 7},
	{-7, 7}, {-7, 7}, {-7, 7}, {-7, 7},
	{-7, 7}, {-7, 7}, {-7, 7}, {-7, 7},
	{6, 7}, {6, 7}, {6, 7}, {6, 7},
	{6, 7}, {6, 7}, {6, 7}, {6, 7},
	{-6, 7}, {-6, 7}, {-6, 7}, {-6, 7},
	{-6, 7}, {-6, 7}, {-6, 7}, {-6, 7},
	{5, 7}, {5, 7}, {5, 7}, {5, 7},
	{5, 7}, {5, 7}, {5, 7}, {5, 7},
	{-5, 7}, {-5, 7}, {-5, 7}, {-5, 7},
	{-5, 7}, {-5, 7}, {-5, 7}, {-5, 7},
	{4, 6}, {4, 6}, {4, 6}, {4, 6},
	{4, 6}, {4, 6}, {4, 6}, {4, 6},
	{4, 6}, {4, 6}, {4, 6}, {4, 6},
	{4, 6}, {4, 6}, {4, 6}, {4, 6},
	{-4, 6}, {-4, 6}, {-4, 6}, {-4, 6},
	{-4, 6}, {-4, 6}, {-4, 6}, {-4, 6},
	{-4, 6}, {-4, 6}, {-4, 6}, {-4, 6},
	{-4, 6}, {-4, 6}, {-4, 6}, {-4, 6}
};

static
VLC const TMNMVtab2[] = {
	{32, 12}, {-32, 12}, {31, 12}, {-31, 12},
	{30, 11}, {30, 11}, {-30, 11}, {-30, 11},
	{29, 11}, {29, 11}, {-29, 11}, {-29, 11},
	{28, 11}, {28, 11}, {-28, 11}, {-28, 11},
	{27, 11}, {27, 11}, {-27, 11}, {-27, 11},
	{26, 11}, {26, 11}, {-26, 11}, {-26, 11},
	{25, 11}, {25, 11}, {-25, 11}, {-25, 11},
	{24, 10}, {24, 10}, {24, 10}, {24, 10},
	{-24, 10}, {-24, 10}, {-24, 10}, {-24, 10},
	{23, 10}, {23, 10}, {23, 10}, {23, 10},
	{-23, 10}, {-23, 10}, {-23, 10}, {-23, 10},
	{22, 10}, {22, 10}, {22, 10}, {22, 10},
	{-22, 10}, {-22, 10}, {-22, 10}, {-22, 10},
	{21, 10}, {21, 10}, {21, 10}, {21, 10},
	{-21, 10}, {-21, 10}, {-21, 10}, {-21, 10},
	{20, 10}, {20, 10}, {20, 10}, {20, 10},
	{-20, 10}, {-20, 10}, {-20, 10}, {-20, 10},
	{19, 10}, {19, 10}, {19, 10}, {19, 10},
	{-19, 10}, {-19, 10}, {-19, 10}, {-19, 10},
	{18, 10}, {18, 10}, {18, 10}, {18, 10},
	{-18, 10}, {-18, 10}, {-18, 10}, {-18, 10},
	{17, 10}, {17, 10}, {17, 10}, {17, 10},
	{-17, 10}, {-17, 10}, {-17, 10}, {-17, 10},
	{16, 10}, {16, 10}, {16, 10}, {16, 10},
	{-16, 10}, {-16, 10}, {-16, 10}, {-16, 10},
	{15, 10}, {15, 10}, {15, 10}, {15, 10},
	{-15, 10}, {-15, 10}, {-15, 10}, {-15, 10},
	{14, 10}, {14, 10}, {14, 10}, {14, 10},
	{-14, 10}, {-14, 10}, {-14, 10}, {-14, 10},
	{13, 10}, {13, 10}, {13, 10}, {13, 10},
	{-13, 10}, {-13, 10}, {-13, 10}, {-13, 10}
};

static
VLC const dc_lum_tab[] = {
	{0, 0}, {4, 3}, {3, 3}, {0, 3},
	{2, 2}, {2, 2}, {1, 2}, {1, 2},
};


/* FILE: xvidcore-1.0.3/src/bitstream/mbcoding.c */

#define LEVELOFFSET 32

/* Initialized once during xvid_global call
 * RO access is thread safe */
static REVERSE_EVENT DCT3D[2][4096];
static VLC coeff_VLC[2][2][64][64];

static int bs_get_spritetrajectory(Bitstream * bs)
{
	int i;
	for (i = 0; i < 12; i++)
	{
		if (BitstreamShowBits(bs, sprite_trajectory_len[i].len) == sprite_trajectory_len[i].code)
		{
			BitstreamSkip(bs, sprite_trajectory_len[i].len);
			return i;
		}
	}
	return -1;
}

static void
init_vlc_tables(void)
{
	uint32_t i, j, k, intra, last, run,  run_esc, level, level_esc, escape, escape_len, offset;
	int32_t l;

	for (intra = 0; intra < 2; intra++)
		for (i = 0; i < 4096; i++)
			DCT3D[intra][i].event.level = 0;

	for (intra = 0; intra < 2; intra++) {
		for (last = 0; last < 2; last++) {
			for (run = 0; run < 63 + last; run++) {
				for (level = 0; level < (uint32_t)(32 << intra); level++) {
					offset = !intra * LEVELOFFSET;
					coeff_VLC[intra][last][level + offset][run].len = 128;
				}
			}
		}
	}

	for (intra = 0; intra < 2; intra++) {
		for (i = 0; i < 102; i++) {
			offset = !intra * LEVELOFFSET;

			for (j = 0; j < (uint32_t)(1 << (12 - coeff_tab[intra][i].vlc.len)); j++) {
				DCT3D[intra][(coeff_tab[intra][i].vlc.code << (12 - coeff_tab[intra][i].vlc.len)) | j].len	 = coeff_tab[intra][i].vlc.len;
				DCT3D[intra][(coeff_tab[intra][i].vlc.code << (12 - coeff_tab[intra][i].vlc.len)) | j].event = coeff_tab[intra][i].event;
			}

			coeff_VLC[intra][coeff_tab[intra][i].event.last][coeff_tab[intra][i].event.level + offset][coeff_tab[intra][i].event.run].code
				= coeff_tab[intra][i].vlc.code << 1;
			coeff_VLC[intra][coeff_tab[intra][i].event.last][coeff_tab[intra][i].event.level + offset][coeff_tab[intra][i].event.run].len
				= coeff_tab[intra][i].vlc.len + 1;

			if (!intra) {
				coeff_VLC[intra][coeff_tab[intra][i].event.last][offset - coeff_tab[intra][i].event.level][coeff_tab[intra][i].event.run].code
					= (coeff_tab[intra][i].vlc.code << 1) | 1;
				coeff_VLC[intra][coeff_tab[intra][i].event.last][offset - coeff_tab[intra][i].event.level][coeff_tab[intra][i].event.run].len
					= coeff_tab[intra][i].vlc.len + 1;
			}
		}
	}

	for (intra = 0; intra < 2; intra++) {
		for (last = 0; last < 2; last++) {
			for (run = 0; run < 63 + last; run++) {
				for (level = 1; level < (uint32_t)(32 << intra); level++) {

					if (level <= max_level[intra][last][run] && run <= max_run[intra][last][level])
					    continue;

					offset = !intra * LEVELOFFSET;
                    level_esc = level - max_level[intra][last][run];
					run_esc = run - 1 - max_run[intra][last][level];

					if (level_esc <= max_level[intra][last][run] && run <= max_run[intra][last][level_esc]) {
						escape     = ESCAPE1;
						escape_len = 7 + 1;
						run_esc    = run;
					} else {
						if (run_esc <= max_run[intra][last][level] && level <= max_level[intra][last][run_esc]) {
							escape     = ESCAPE2;
							escape_len = 7 + 2;
							level_esc  = level;
						} else {
							if (!intra) {
								coeff_VLC[intra][last][level + offset][run].code
									= (ESCAPE3 << 21) | (last << 20) | (run << 14) | (1 << 13) | ((level & 0xfff) << 1) | 1;
								coeff_VLC[intra][last][level + offset][run].len = 30;
									coeff_VLC[intra][last][offset - level][run].code
									= (ESCAPE3 << 21) | (last << 20) | (run << 14) | (1 << 13) | ((-(int32_t)level & 0xfff) << 1) | 1;
								coeff_VLC[intra][last][offset - level][run].len = 30;
							}
							continue;
						}
					}

					coeff_VLC[intra][last][level + offset][run].code
						= (escape << coeff_VLC[intra][last][level_esc + offset][run_esc].len)
						|  coeff_VLC[intra][last][level_esc + offset][run_esc].code;
					coeff_VLC[intra][last][level + offset][run].len
						= coeff_VLC[intra][last][level_esc + offset][run_esc].len + escape_len;

					if (!intra) {
						coeff_VLC[intra][last][offset - level][run].code
							= (escape << coeff_VLC[intra][last][level_esc + offset][run_esc].len)
							|  coeff_VLC[intra][last][level_esc + offset][run_esc].code | 1;
						coeff_VLC[intra][last][offset - level][run].len
							= coeff_VLC[intra][last][level_esc + offset][run_esc].len + escape_len;
					}
				}

				if (!intra) {
					coeff_VLC[intra][last][0][run].code
						= (ESCAPE3 << 21) | (last << 20) | (run << 14) | (1 << 13) | ((-32 & 0xfff) << 1) | 1;
					coeff_VLC[intra][last][0][run].len = 30;
				}
			}
		}
	}

	/* init sprite_trajectory tables
	 * even if GMC is not specified (it might be used later...) */

	sprite_trajectory_code[0+16384].code = 0;
	sprite_trajectory_code[0+16384].len = 0;
	for (k=0;k<14;k++) {
		int limit = (1<<k);

		for (l=-(2*limit-1); l <= -limit; l++) {
			sprite_trajectory_code[l+16384].code = (2*limit-1)+l;
			sprite_trajectory_code[l+16384].len = k+1;
		}

		for (l=limit; l<= 2*limit-1; l++) {
			sprite_trajectory_code[l+16384].code = l;
			sprite_trajectory_code[l+16384].len = k+1;
		}
	}
}

/***************************************************************
 * decoding stuff starts here                                  *
 ***************************************************************/

/*
 * for IVOP addbits == 0
 * for PVOP addbits == fcode - 1
 * for BVOP addbits == max(fcode,bcode) - 1
 * returns true or false
 */
static int
check_resync_marker(Bitstream * bs, int addbits)
{
	uint32_t nbits;
	uint32_t code;
	uint32_t nbitsresyncmarker = NUMBITS_VP_RESYNC_MARKER + addbits;

	nbits = BitstreamNumBitsToByteAlign(bs);
	code = BitstreamShowBits(bs, nbits);

	if (code == (((uint32_t)1 << (nbits - 1)) - 1))
	{
		return BitstreamShowBitsFromByteAlign(bs, nbitsresyncmarker) == RESYNC_MARKER;
	}

	return 0;
}

static int
get_mcbpc_intra(Bitstream * bs)
{

	uint32_t index;

	index = BitstreamShowBits(bs, 9);
	index >>= 3;

	BitstreamSkip(bs, mcbpc_intra_table[index].len);

	return mcbpc_intra_table[index].code;

}

static int
get_mcbpc_inter(Bitstream * bs)
{

	uint32_t index;

	index = MIN(BitstreamShowBits(bs, 9), 256);

	BitstreamSkip(bs, mcbpc_inter_table[index].len);

	return mcbpc_inter_table[index].code;

}

static int
get_cbpy(Bitstream * bs,
		 int intra)
{

	int cbpy;
	uint32_t index = BitstreamShowBits(bs, 6);

	BitstreamSkip(bs, cbpy_table[index].len);
	cbpy = cbpy_table[index].code;

	if (!intra)
		cbpy = 15 - cbpy;

	return cbpy;

}

static __inline int
get_mv_data(Bitstream * bs)
{

	uint32_t index;

	if (BitstreamGetBit(bs))
		return 0;

	index = BitstreamShowBits(bs, 12);

	if (index >= 512) {
		index = (index >> 8) - 2;
		BitstreamSkip(bs, TMNMVtab0[index].len);
		return TMNMVtab0[index].code;
	}

	if (index >= 128) {
		index = (index >> 2) - 32;
		BitstreamSkip(bs, TMNMVtab1[index].len);
		return TMNMVtab1[index].code;
	}

	index -= 4;

	BitstreamSkip(bs, TMNMVtab2[index].len);
	return TMNMVtab2[index].code;

}

static int
get_mv(Bitstream * bs,
	   int fcode)
{

	int data;
	int res;
	int mv;
	int scale_fac = 1 << (fcode - 1);

	data = get_mv_data(bs);

	if (scale_fac == 1 || data == 0)
		return data;

	res = BitstreamGetBits(bs, fcode - 1);
	mv = ((abs(data) - 1) * scale_fac) + res + 1;

	return data < 0 ? -mv : mv;

}

static int
get_dc_dif(Bitstream * bs,
		   uint32_t dc_size)
{

	int code = BitstreamGetBits(bs, dc_size);
	int msb = code >> (dc_size - 1);

	if (msb == 0)
		return (-1 * (code ^ ((1 << dc_size) - 1)));

	return code;

}

static int
get_dc_size_lum(Bitstream * bs)
{

	int code, i;

	code = BitstreamShowBits(bs, 11);

	for (i = 11; i > 3; i--) {
		if (code == 1) {
			BitstreamSkip(bs, i);
			return i + 1;
		}
		code >>= 1;
	}

	BitstreamSkip(bs, dc_lum_tab[code].len);
	return dc_lum_tab[code].code;

}

static int
get_dc_size_chrom(Bitstream * bs)
{

	uint32_t code, i;

	code = BitstreamShowBits(bs, 12);

	for (i = 12; i > 2; i--) {
		if (code == 1) {
			BitstreamSkip(bs, i);
			return i;
		}
		code >>= 1;
	}

	return 3 - BitstreamGetBits(bs, 2);

}

static __inline void
get_coeff(Bitstream * bs,
		  int *run,
		  int *last,
		  int intra)
{

	uint32_t mode;
	REVERSE_EVENT *reverse_event;

	if (BitstreamShowBits(bs, 7) != ESCAPE) {
		reverse_event = &DCT3D[intra][BitstreamShowBits(bs, 12)];

		if (reverse_event->event.level == 0)
			goto error;

		*last = reverse_event->event.last;
		*run  = reverse_event->event.run;

		BitstreamSkip(bs, reverse_event->len + 1);

		return;
	}

	BitstreamSkip(bs, 7);

	mode = BitstreamShowBits(bs, 2);

	if (mode < 3) {
		BitstreamSkip(bs, (mode == 2) ? 2 : 1);

		reverse_event = &DCT3D[intra][BitstreamShowBits(bs, 12)];

		if (reverse_event->event.level == 0)
			goto error;

		*last = reverse_event->event.last;
		*run  = reverse_event->event.run;

		BitstreamSkip(bs, reverse_event->len + 1);

		return;
	}

	/* third escape mode - fixed length codes */
	BitstreamSkip(bs, 2);
	*last = BitstreamGetBits(bs, 1);
	*run = BitstreamGetBits(bs, 6);
	BitstreamSkip(bs, 14);

	return;

  error:
	*run = VLC_ERROR;
	return;
}

static void
get_intra_block(Bitstream * bs)
{
	int run, last;

	do {
		get_coeff(bs, &run, &last, 1);
		if (run == -1) {
			DPRINTF(XVID_DEBUG_ERROR,"fatal: invalid run");
			break;
		}
	} while (!last);

}

static void
get_inter_block(Bitstream * bs)
{
	int run;
	int last;

	do {
		get_coeff(bs, &run, &last, 0);
		if (run == -1) {
			DPRINTF(XVID_DEBUG_ERROR,"fatal: invalid run");
			break;
		}
	} while (!last);

}


/* FILE: xvidcore-1.0.3/src/motion/gmc.c */

/* ************************************************************
 * Pts = 2 or 3
 *
 * Warning! *src is the global frame pointer (that is: adress
 * of pixel 0,0), not the macroblock one.
 * Conversely, *dst is the macroblock top-left adress.
 */

static
void Predict_16x16_C(const NEW_GMC_DATA * const This,
					 uint8_t *dst, const uint8_t *src,
					 int dststride, int srcstride, int x, int y, int rounding)
{
}

/* ************************************************************
 * simplified version for 1 warp point
 */

static
void Predict_1pt_16x16_C(const NEW_GMC_DATA * const This,
						 uint8_t *Dst, const uint8_t *Src,
						 int dststride, int srcstride, int x, int y, int rounding)
{
}

/* *************************************************************
 * Warning! It's Accuracy being passed, not 'resolution'!
 */

static
void generate_GMCparameters( int nb_pts, const int accuracy,
								 const WARPPOINTS *const pts,
								 const int width, const int height,
								 NEW_GMC_DATA *const gmc)
{
	gmc->sW = width	<< 4;
	gmc->sH = height << 4;
	gmc->accuracy = accuracy;
	gmc->num_wp = nb_pts;

	/* reduce the number of points, if possible */
	if (nb_pts<2 || (pts->duv[2].x==0 && pts->duv[2].y==0 && pts->duv[1].x==0 && pts->duv[1].y==0 )) {
  	if (nb_pts<2 || (pts->duv[1].x==0 && pts->duv[1].y==0)) {
	  	if (nb_pts<1 || (pts->duv[0].x==0 && pts->duv[0].y==0)) {
		    nb_pts = 0;
  		}
	  	else nb_pts = 1;
  	}
	  else nb_pts = 2;
  }

	/* now, nb_pts stores the actual number of points required for interpolation */

	if (nb_pts<=1)
	{
	if (nb_pts==1) {
		/* store as 4b fixed point */
		gmc->Uo = pts->duv[0].x << accuracy;
		gmc->Vo = pts->duv[0].y << accuracy;
		gmc->Uco = ((pts->duv[0].x>>1) | (pts->duv[0].x&1)) << accuracy;	 /* DIV2RND() */
		gmc->Vco = ((pts->duv[0].y>>1) | (pts->duv[0].y&1)) << accuracy;	 /* DIV2RND() */
	}
	else {	/* zero points?! */
		gmc->Uo	= gmc->Vo	= 0;
		gmc->Uco = gmc->Vco = 0;
	}

	gmc->predict_16x16	= Predict_1pt_16x16_C;
	}
	else {		/* 2 or 3 points */
	const int rho	 = 3 - accuracy;	/* = {3,2,1,0} for Acc={0,1,2,3} */
	int Alpha = log2bin(width-1);
	int Ws = 1 << Alpha;

	gmc->dU[0] = 16*Ws + RDIV( 8*Ws*pts->duv[1].x, width );	 /* dU/dx */
	gmc->dV[0] =		 RDIV( 8*Ws*pts->duv[1].y, width );	 /* dV/dx */

	if (nb_pts==2) {
		gmc->dU[1] = -gmc->dV[0];	/* -Sin */
		gmc->dV[1] =	gmc->dU[0] ;	/* Cos */
	}
	else
	{
		const int Beta = log2bin(height-1);
		const int Hs = 1<<Beta;
		gmc->dU[1] =		 RDIV( 8*Hs*pts->duv[2].x, height );	 /* dU/dy */
		gmc->dV[1] = 16*Hs + RDIV( 8*Hs*pts->duv[2].y, height );	 /* dV/dy */
		if (Beta>Alpha) {
		gmc->dU[0] <<= (Beta-Alpha);
		gmc->dV[0] <<= (Beta-Alpha);
		Alpha = Beta;
		Ws = Hs;
		}
		else {
		gmc->dU[1] <<= Alpha - Beta;
		gmc->dV[1] <<= Alpha - Beta;
		}
	}
	/* upscale to 16b fixed-point */
	gmc->dU[0] <<= (16-Alpha - rho);
	gmc->dU[1] <<= (16-Alpha - rho);
	gmc->dV[0] <<= (16-Alpha - rho);
	gmc->dV[1] <<= (16-Alpha - rho);

	gmc->Uo	= ( pts->duv[0].x	 <<(16+ accuracy)) + (1<<15);
	gmc->Vo	= ( pts->duv[0].y	 <<(16+ accuracy)) + (1<<15);
	gmc->Uco = ((pts->duv[0].x-1)<<(17+ accuracy)) + (1<<17);
	gmc->Vco = ((pts->duv[0].y-1)<<(17+ accuracy)) + (1<<17);
	gmc->Uco = (gmc->Uco + gmc->dU[0] + gmc->dU[1])>>2;
	gmc->Vco = (gmc->Vco + gmc->dV[0] + gmc->dV[1])>>2;

	gmc->predict_16x16	= Predict_16x16_C;
	}
}


/* FILE: xvidcore-1.0.3/src/utils/mem_align.c */

static void *
xvid_malloc(size_t size,
			uint8_t alignment)
{
	uint8_t *mem_ptr;

	if (!alignment) {

		/* We have not to satisfy any alignment */
		if ((mem_ptr = (uint8_t *) malloc(size + 1)) != NULL) {

			/* Store (mem_ptr - "real allocated memory") in *(mem_ptr-1) */
			*mem_ptr = (uint8_t)1;

			/* Return the mem_ptr pointer */
			return ((void *)(mem_ptr+1));
		}
	} else {
		uint8_t *tmp;

		/* Allocate the required size memory + alignment so we
		 * can realign the data if necessary */
		if ((tmp = (uint8_t *) malloc(size + alignment)) != NULL) {

			/* Align the tmp pointer */
			mem_ptr =
				(uint8_t *) ((ptr_t) (tmp + alignment - 1) &
							 (~(ptr_t) (alignment - 1)));

			/* Special case where malloc have already satisfied the alignment
			 * We must add alignment to mem_ptr because we must store
			 * (mem_ptr - tmp) in *(mem_ptr-1)
			 * If we do not add alignment to mem_ptr then *(mem_ptr-1) points
			 * to a forbidden memory space */
			if (mem_ptr == tmp)
				mem_ptr += alignment;

			/* (mem_ptr - tmp) is stored in *(mem_ptr-1) so we are able to retrieve
			 * the real malloc block allocated and free it in xvid_free */
			*(mem_ptr - 1) = (uint8_t) (mem_ptr - tmp);

			/* Return the aligned pointer */
			return ((void *)mem_ptr);
		}
	}

	return(NULL);
}

static void
xvid_free(void *mem_ptr)
{

	uint8_t *ptr;

	if (mem_ptr == NULL)
		return;

	/* Aligned pointer */
	ptr = mem_ptr;

	/* *(ptr - 1) holds the offset to the real allocated block
	 * we sub that offset os we free the real pointer */
	ptr -= *(ptr - 1);

	/* Free the memory */
	free(ptr);
}


#endif /* XVID_CODE_INCLUDED */
