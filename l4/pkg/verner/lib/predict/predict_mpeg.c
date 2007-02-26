#define COMPILE_MPEG \
  gcc -O3 -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wno-long-long -ansi -pedantic -DTESTING -DTEST_MPEG_MAIN -lm -o predict_mpeg $0 ; exit
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

#include "predict.h"

/* the libmpeg2 code needs to be parsed first, but I prefer to keep it at the end */
#ifndef MPEG_CODE_INCLUDED
#define MPEG_CODE_INCLUDED
#include "predict_mpeg.c"

#define MPEG_METRICS_COUNT 6
#define MPEG_CONTEXT_COUNT 4


/* internal structures */
struct predictor_s {
  char *learn_file;
  char *predict_file;
  
  /* libmpeg2 library context */
  mpeg2dec_t *mpeg;
  const mpeg2_info_t *mpeg_info;
  
  /* the MPEG metrics */
  double metrics[MPEG_METRICS_COUNT];
  int frame_type;
  char valid;
  
  /* the llsp contexts for the frametypes */
  llsp_t *llsp[MPEG_CONTEXT_COUNT];
};

enum {
  METRIC_MPEG_FRAME_BYTES, METRIC_MPEG_FRAME_PIXELS,
  METRIC_MPEG_MACRO_INTRA, METRIC_MPEG_MACRO_INTER, METRIC_MPEG_MACRO_INTER_SHORT,
  METRIC_MPEG_MOTION
};

/* helper functions */
#ifdef TESTING
#  include "llsp_solver.c"
#  undef LOG
#  define LOG(format, arg) printf(format "\n", arg)
#endif


predictor_t *predict_mpeg_new(const char *learn_file, const char *predict_file)
{
  predictor_t *predictor;
  int i;
  
  predictor = (predictor_t *)malloc(sizeof(predictor_t));
  if (!predictor) return NULL;
  memset(predictor, 0, sizeof(predictor));
  
  predictor->learn_file   = (learn_file   && learn_file[0])   ? strdup(learn_file)   : NULL;
  predictor->predict_file = (predict_file && predict_file[0]) ? strdup(predict_file) : NULL;
  
  if (!predictor->learn_file && !predictor->predict_file) goto fail;
  
  /* initialize our stripped down libmpeg2 */
  if (!(predictor->mpeg = mpeg2_init()))
    goto fail;
  predictor->mpeg_info = mpeg2_info(predictor->mpeg);
  
  for (i = 0; i < sizeof(predictor->llsp) / sizeof(predictor->llsp[0]); i++) {
    predictor->llsp[i] = llsp_new(PREDICT_MPEG_BASE + i, MPEG_METRICS_COUNT);
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

double predict_mpeg(predictor_t *predictor, unsigned char **data, unsigned *length)
{
  int frame_count;
  double time = 0.0;
  
  if (!predictor->learn_file && !predictor->predict_file) return -1.0;
  if (!data || !length) return -1.0;
  
  predictor->valid = 1;
  frame_count = 0;
  
  if (*length) mpeg2_buffer(predictor->mpeg, *data, *data + *length);
  
  while (1) {
    switch (mpeg2_parse(predictor->mpeg)) {
    case STATE_SLICE:
      /* we "decoded" one frame */
      frame_count++;
      
      predictor->metrics[METRIC_MPEG_FRAME_BYTES]       = predictor->mpeg_info->bytes;
      predictor->metrics[METRIC_MPEG_FRAME_PIXELS]      = predictor->mpeg_info->sequence->width * predictor->mpeg_info->sequence->height;
      predictor->metrics[METRIC_MPEG_MACRO_INTRA]       = predictor->mpeg_info->macro_intra;
      predictor->metrics[METRIC_MPEG_MACRO_INTER]       = predictor->mpeg_info->macro_inter;
      predictor->metrics[METRIC_MPEG_MACRO_INTER_SHORT] = predictor->mpeg_info->macro_inter_short;
      predictor->metrics[METRIC_MPEG_MOTION]            = predictor->mpeg_info->motion;
      
      switch (predictor->mpeg_info->current_picture->flags & PIC_MASK_CODING_TYPE) {
      case PIC_FLAG_CODING_TYPE_I:
	predictor->frame_type = PREDICT_MPEG_I;
	break;
      case PIC_FLAG_CODING_TYPE_P:
	predictor->frame_type = PREDICT_MPEG_P;
	break;
      case PIC_FLAG_CODING_TYPE_B:
	predictor->frame_type = PREDICT_MPEG_B;
	break;
      case PIC_FLAG_CODING_TYPE_D:
	predictor->frame_type = PREDICT_MPEG_D;
	break;
      default:
	predictor->valid = 0;
      }
      
      if (predictor->predict_file && predictor->valid)
	time += llsp_predict(predictor->llsp[predictor->frame_type - PREDICT_MPEG_BASE], predictor->metrics);
      else
	time = -1.0;
      
      if (!predictor->mpeg_info->display_fbuf)
	/* no frame yet, decoder cycle incomplete */
	break;
      
      /* falling through is intended */
    case STATE_BUFFER:
    case STATE_END:
      /* we consumed all data -> return and wait for next chunk */
      *data += *length;
      *length = 0;
      /* we can only use this predictor for further learning if only one frame was handled */
      predictor->valid = (frame_count == 1);
      return frame_count > 0 ? time : -1.0;
    case STATE_INVALID:
    case STATE_INVALID_END:
      predictor->valid = 0;
      mpeg2_reset(predictor->mpeg, 0);
      return -1.0;
    default:
      /* the rest is harmless -> iterate */
      break;
    }
  }
}

void predict_mpeg_learn(predictor_t *predictor, double time)
{
  if (predictor->learn_file && predictor->valid)
    llsp_accumulate(predictor->llsp[predictor->frame_type - PREDICT_MPEG_BASE], predictor->metrics, time);
}

void predict_mpeg_eval(predictor_t *predictor)
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

void predict_mpeg_discontinue(predictor_t *predictor)
{
  mpeg2_reset(predictor->mpeg, 1);
}

void predict_mpeg_dispose(predictor_t *predictor)
{
  int i;
  
  for (i = 0; i < sizeof(predictor->llsp) / sizeof(predictor->llsp[0]); i++)
    llsp_dispose(predictor->llsp[i]);
  mpeg2_close(predictor->mpeg);
  free(predictor->learn_file);
  free(predictor->predict_file);
  free(predictor);
}


#ifdef TEST_MPEG_MAIN
int main(void)
{
  predictor_t *predictor;
  unsigned char mem[2 * 1024 * 1024];
  unsigned char *buf = mem;
  unsigned length = 0;
  
  predictor = predict_mpeg_new("/dev/null", NULL);
  while (!feof(stdin)) {
    length += fread(buf, sizeof(mem[0]), mem + sizeof(mem)/sizeof(mem[0]) - buf, stdin);
    buf = mem;
    do {
      predict_mpeg(predictor, &buf, &length);
      if (predictor->valid)
	printf("%d, %G, %G, %G, %G, %G, %G\n", predictor->frame_type,
	  predictor->metrics[METRIC_MPEG_FRAME_BYTES],
	  predictor->metrics[METRIC_MPEG_FRAME_PIXELS],
	  predictor->metrics[METRIC_MPEG_MACRO_INTRA],
	  predictor->metrics[METRIC_MPEG_MACRO_INTER],
	  predictor->metrics[METRIC_MPEG_MACRO_INTER_SHORT],
	  predictor->metrics[METRIC_MPEG_MOTION]);
      else
	printf("error extracting metrics\n");
    } while (mpeg2_getpos(predictor->mpeg));
    memmove(mem, buf, length);
    buf = mem + length;
  }
  predict_mpeg_dispose(predictor);
  
  return 0;
}
#endif


#else /* MPEG_CODE_INCLUDED */

#define int8_t   char
#define uint8_t  unsigned char
#define int16_t  short
#define uint16_t unsigned short
#define int32_t  int
#define uint32_t unsigned int
#define int64_t  long long
#define uint64_t unsigned long long


/*
 * The following code is taken from various files of the original libmpeg2
 * library. Credits go to the members of the libmpeg2 team.
 *
 * Although the code has been heavily reduced to skip all the decoding
 * and only keep the bitstream parsing for metrics extraction, you should
 * still get something useful, when diffing this file against the
 * respective files of the mpeg2dec distribution, because indentation style
 * has been preserved. Use something like this to get a diff:
 *
 * sed -n '/FILE: mpeg2dec-0.4.0\/libmpeg2\/decode.c/,/FILE:/p' predict_mpeg.c | diff -u mpeg2dec-0.4.0/libmpeg2/decode.c -
 */


/* FILE: mpeg2dec-0.4.0/include/mpeg2.h */

#define MPEG2_VERSION(a,b,c) (((a)<<16)|((b)<<8)|(c))
#define MPEG2_RELEASE MPEG2_VERSION (0, 4, 0)	/* 0.4.0 */

#define SEQ_FLAG_MPEG2 1
#define SEQ_FLAG_CONSTRAINED_PARAMETERS 2
#define SEQ_FLAG_PROGRESSIVE_SEQUENCE 4
#define SEQ_FLAG_LOW_DELAY 8
#define SEQ_FLAG_COLOUR_DESCRIPTION 16

#define SEQ_MASK_VIDEO_FORMAT 0xe0
#define SEQ_VIDEO_FORMAT_COMPONENT 0
#define SEQ_VIDEO_FORMAT_PAL 0x20
#define SEQ_VIDEO_FORMAT_NTSC 0x40
#define SEQ_VIDEO_FORMAT_SECAM 0x60
#define SEQ_VIDEO_FORMAT_MAC 0x80
#define SEQ_VIDEO_FORMAT_UNSPECIFIED 0xa0

typedef struct mpeg2_sequence_s {
    unsigned int width, height;
    unsigned int chroma_width, chroma_height;
    unsigned int byte_rate;
    unsigned int vbv_buffer_size;
    uint32_t flags;

    unsigned int picture_width, picture_height;
    unsigned int display_width, display_height;
    unsigned int pixel_width, pixel_height;
    unsigned int frame_period;

    uint8_t profile_level_id;
    uint8_t colour_primaries;
    uint8_t transfer_characteristics;
    uint8_t matrix_coefficients;
} mpeg2_sequence_t;

#define GOP_FLAG_DROP_FRAME 1
#define GOP_FLAG_BROKEN_LINK 2
#define GOP_FLAG_CLOSED_GOP 4

typedef struct mpeg2_gop_s {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t pictures;
    uint32_t flags;
} mpeg2_gop_t;

#define PIC_MASK_CODING_TYPE 7
#define PIC_FLAG_CODING_TYPE_I 1
#define PIC_FLAG_CODING_TYPE_P 2
#define PIC_FLAG_CODING_TYPE_B 3
#define PIC_FLAG_CODING_TYPE_D 4

#define PIC_FLAG_TOP_FIELD_FIRST 8
#define PIC_FLAG_PROGRESSIVE_FRAME 16
#define PIC_FLAG_COMPOSITE_DISPLAY 32
#define PIC_FLAG_SKIP 64
#define PIC_FLAG_TAGS 128
#define PIC_MASK_COMPOSITE_DISPLAY 0xfffff000

typedef struct mpeg2_picture_s {
    unsigned int temporal_reference;
    unsigned int nb_fields;
    uint32_t tag, tag2;
    uint32_t flags;
    struct {
	int x, y;
    } display_offset[3];
} mpeg2_picture_t;

typedef struct mpeg2_fbuf_s {
    uint8_t * buf[3];
    void * id;
} mpeg2_fbuf_t;

typedef struct mpeg2_info_s {
    const mpeg2_sequence_t * sequence;
    const mpeg2_gop_t * gop;
    const mpeg2_picture_t * current_picture;
    const mpeg2_picture_t * current_picture_2nd;
    const mpeg2_fbuf_t * current_fbuf;
    const mpeg2_picture_t * display_picture;
    const mpeg2_picture_t * display_picture_2nd;
    const mpeg2_fbuf_t * display_fbuf;
    const mpeg2_fbuf_t * discard_fbuf;
    const uint8_t * user_data;
    unsigned int user_data_len;
    /* prediction */
    unsigned int bytes;
    unsigned int macro_intra;
    unsigned int macro_inter;
    unsigned int macro_inter_short;
    unsigned int motion;
} mpeg2_info_t;

typedef struct mpeg2dec_s mpeg2dec_t;
typedef struct mpeg2_decoder_s mpeg2_decoder_t;

typedef enum {
    STATE_BUFFER = 0,
    STATE_SEQUENCE = 1,
    STATE_SEQUENCE_REPEATED = 2,
    STATE_GOP = 3,
    STATE_PICTURE = 4,
    STATE_SLICE_1ST = 5,
    STATE_PICTURE_2ND = 6,
    STATE_SLICE = 7,
    STATE_END = 8,
    STATE_INVALID = 9,
    STATE_INVALID_END = 10
} mpeg2_state_t;

typedef struct mpeg2_convert_init_s {
    unsigned int id_size;
    unsigned int buf_size[3];
    void (* start) (void * id, const mpeg2_fbuf_t * fbuf,
		    const mpeg2_picture_t * picture, const mpeg2_gop_t * gop);
    void (* copy) (void * id, uint8_t * const * src, unsigned int v_offset);
} mpeg2_convert_init_t;
typedef enum {
    MPEG2_CONVERT_SET = 0,
    MPEG2_CONVERT_STRIDE = 1,
    MPEG2_CONVERT_START = 2
} mpeg2_convert_stage_t;
typedef int mpeg2_convert_t (int stage, void * id,
			     const mpeg2_sequence_t * sequence, int stride,
			     uint32_t accel, void * arg,
			     mpeg2_convert_init_t * result);

static
mpeg2dec_t * mpeg2_init (void);
static
const mpeg2_info_t * mpeg2_info (mpeg2dec_t * mpeg2dec);
static
void mpeg2_close (mpeg2dec_t * mpeg2dec);

static
void mpeg2_buffer (mpeg2dec_t * mpeg2dec, uint8_t * start, uint8_t * end);
#ifdef __GNUC__
static
int mpeg2_getpos (mpeg2dec_t * mpeg2dec) __attribute__((unused));
#else
static
int mpeg2_getpos (mpeg2dec_t * mpeg2dec);
#endif
static
mpeg2_state_t mpeg2_parse (mpeg2dec_t * mpeg2dec);

static
void mpeg2_reset (mpeg2dec_t * mpeg2dec, int full_reset);

static
void mpeg2_init_fbuf (mpeg2_decoder_t * decoder, uint8_t * current_fbuf[3],
		      uint8_t * forward_fbuf[3], uint8_t * backward_fbuf[3]);
static
void mpeg2_slice (mpeg2_decoder_t * decoder, int code, const uint8_t * buffer);

typedef enum {
    MPEG2_ALLOC_MPEG2DEC = 0,
    MPEG2_ALLOC_CHUNK = 1,
    MPEG2_ALLOC_YUV = 2,
    MPEG2_ALLOC_CONVERT_ID = 3,
    MPEG2_ALLOC_CONVERTED = 4
} mpeg2_alloc_t;

static
void * mpeg2_malloc (unsigned size, mpeg2_alloc_t reason);
static
void mpeg2_free (void * buf);


/* FILE: mpeg2dec-0.4.0/include/config.h */

/* x86 architecture */
#define ARCH_X86 

/* maximum supported data alignment */
#define ATTRIBUTE_ALIGNED_MAX 64

/* Define if you have the `__builtin_expect' function. */
#define HAVE_BUILTIN_EXPECT 

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the `ftime' function. */
#define HAVE_FTIME 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if the system has the type `struct timeval'. */
#define HAVE_STRUCT_TIMEVAL 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/timeb.h> header file. */
#define HAVE_SYS_TIMEB_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* libvo X11 support */
#define LIBVO_X11 

/* libvo Xv support */
#define LIBVO_XV 

/* Name of package */
#define PACKAGE "mpeg2dec"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Version number of package */
#define VERSION "0.4.0"

/* Number of bits in a file offset, on hosts where this is settable. */
#define _FILE_OFFSET_BITS 64

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#define inline __attribute__ ((__always_inline__))
#endif

/* Define as `__restrict' if that's what the C compiler calls it, or to
   nothing if it is not supported. */
#define restrict __restrict__


/* FILE: mpeg2dec-0.4.0/include/attributes.h */

/* use gcc attribs to align critical data structures */
#ifdef ATTRIBUTE_ALIGNED_MAX
#define ATTR_ALIGN(align) __attribute__ ((__aligned__ ((ATTRIBUTE_ALIGNED_MAX < align) ? ATTRIBUTE_ALIGNED_MAX : align)))
#else
#define ATTR_ALIGN(align)
#endif

#ifdef HAVE_BUILTIN_EXPECT
#define likely(x) __builtin_expect ((x) != 0, 1)
#define unlikely(x) __builtin_expect ((x) != 0, 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif


/* FILE: mpeg2dec-0.4.0/libmpeg2/mpeg2_internal.h */

/* macroblock modes */
#define MACROBLOCK_INTRA 1
#define MACROBLOCK_PATTERN 2
#define MACROBLOCK_MOTION_BACKWARD 4
#define MACROBLOCK_MOTION_FORWARD 8
#define MACROBLOCK_QUANT 16
#define DCT_TYPE_INTERLACED 32
/* motion_type */
#define MOTION_TYPE_SHIFT 6
#define MC_FIELD 1
#define MC_FRAME 2
#define MC_16X8 2
#define MC_DMV 3

/* picture structure */
#define TOP_FIELD 1
#define BOTTOM_FIELD 2
#define FRAME_PICTURE 3

/* picture coding type */
#define I_TYPE 1
#define P_TYPE 2
#define B_TYPE 3
#define D_TYPE 4

typedef void mpeg2_mc_fct (uint8_t *, const uint8_t *, int, int);

typedef struct {
    uint8_t * ref[2][3];
    uint8_t ** ref2[2];
    int pmv[2][2];
    int f_code[2];
} motion_t;

typedef void motion_parser_t (mpeg2_decoder_t * decoder,
			      motion_t * motion,
			      mpeg2_mc_fct * const * table);

struct mpeg2_decoder_s {
    /* first, state that carries information from one macroblock to the */
    /* next inside a slice, and is never used outside of mpeg2_slice() */

    /* bit parsing stuff */
    uint32_t bitstream_buf;		/* current 32 bit working set */
    int bitstream_bits;			/* used bits in working set */
    const uint8_t * bitstream_ptr;	/* buffer with stream data */

    uint8_t * dest[3];

    int offset;
    int stride;
    int uv_stride;
    int slice_stride;
    int slice_uv_stride;
    int stride_frame;
    unsigned int limit_x;
    unsigned int limit_y_16;
    unsigned int limit_y_8;
    unsigned int limit_y;

    /* Motion vectors */
    /* The f_ and b_ correspond to the forward and backward motion */
    /* predictors */
    motion_t b_motion;
    motion_t f_motion;
    motion_parser_t * motion_parser[5];

    /* predictor for DC coefficients in intra blocks */
    int16_t dc_dct_pred[3];

    /* DCT coefficients */
    int16_t DCTblock[64] ATTR_ALIGN(64);

    uint8_t * picture_dest[3];
    void (* convert) (void * convert_id, uint8_t * const * src,
		      unsigned int v_offset);
    void * convert_id;

    int dmv_offset;
    unsigned int v_offset;

    /* now non-slice-specific information */

    /* sequence header stuff */
    uint16_t * quantizer_matrix[4];
    uint16_t (* chroma_quantizer[2])[64];
    uint16_t quantizer_prescale[4][32][64];

    /* The width and height of the picture snapped to macroblock units */
    int width;
    int height;
    int vertical_position_extension;
    int chroma_format;

    /* picture header stuff */

    /* what type of picture this is (I, P, B, D) */
    int coding_type;

    /* picture coding extension stuff */

    /* quantization factor for intra dc coefficients */
    int intra_dc_precision;
    /* top/bottom/both fields */
    int picture_structure;
    /* bool to indicate all predictions are frame based */
    int frame_pred_frame_dct;
    /* bool to indicate whether intra blocks have motion vectors */
    /* (for concealment) */
    int concealment_motion_vectors;
    /* bool to use different vlc tables */
    int intra_vlc_format;
    /* used for DMV MC */
    int top_field_first;

    /* stuff derived from bitstream */

    /* pointer to the zigzag scan we're supposed to be using */
    const uint8_t * scan;

    int second_field;

    int mpeg1;
    
    /* decoding time prediction */
    unsigned macro_intra;
    unsigned macro_inter;
    unsigned macro_inter_short;
    unsigned motion;
};

typedef struct {
    mpeg2_fbuf_t fbuf;
} fbuf_alloc_t;

struct mpeg2dec_s {
    mpeg2_decoder_t decoder;

    mpeg2_info_t info;

    uint32_t shift;
    int is_display_initialized;
    mpeg2_state_t (* action) (struct mpeg2dec_s * mpeg2dec);
    mpeg2_state_t state;
    uint32_t ext_state;

    /* allocated in init - gcc has problems allocating such big structures */
    uint8_t * chunk_buffer;
    /* pointer to start of the current chunk */
    uint8_t * chunk_start;
    /* pointer to current position in chunk_buffer */
    uint8_t * chunk_ptr;
    /* last start code ? */
    uint8_t code;

    /* picture tags */
    uint32_t tag_current, tag2_current, tag_previous, tag2_previous;
    int num_tags;
    int bytes_since_tag;

    int first;
    int alloc_index_user;
    int alloc_index;
    uint8_t first_decode_slice;
    uint8_t nb_decode_slices;

    unsigned int user_data_len;

    mpeg2_sequence_t new_sequence;
    mpeg2_sequence_t sequence;
    mpeg2_gop_t new_gop;
    mpeg2_gop_t gop;
    mpeg2_picture_t new_picture;
    mpeg2_picture_t pictures[4];
    mpeg2_picture_t * picture;
    /*const*/ mpeg2_fbuf_t * fbuf[3];	/* 0: current fbuf, 1-2: prediction fbufs */

    fbuf_alloc_t fbuf_alloc[3];
    int custom_fbuf;

    uint8_t * yuv_buf[3][3];
    int yuv_index;
    mpeg2_convert_t * convert;
    void * convert_arg;
    unsigned int convert_id_size;
    int convert_stride;
    void (* convert_start) (void * id, const mpeg2_fbuf_t * fbuf,
			    const mpeg2_picture_t * picture,
			    const mpeg2_gop_t * gop);

    uint8_t * buf_start;
    uint8_t * buf_end;

    int16_t display_offset_x, display_offset_y;

    int copy_matrix;
    int8_t q_scale_type, scaled[4];
    uint8_t quantizer_matrix[4][64];
    uint8_t new_quantizer_matrix[4][64];
};

typedef struct {
#ifdef ARCH_PPC
    uint8_t regv[12*16];
#endif
    int dummy;
} cpu_state_t;

/* decode.c */
static
mpeg2_state_t mpeg2_seek_header (mpeg2dec_t * mpeg2dec);
static
mpeg2_state_t mpeg2_parse_header (mpeg2dec_t * mpeg2dec);

/* header.c */
static
void mpeg2_header_state_init (mpeg2dec_t * mpeg2dec);
static
void mpeg2_reset_info (mpeg2_info_t * info);
static
int mpeg2_header_sequence (mpeg2dec_t * mpeg2dec);
static
int mpeg2_header_gop (mpeg2dec_t * mpeg2dec);
static
mpeg2_state_t mpeg2_header_picture_start (mpeg2dec_t * mpeg2dec);
static
int mpeg2_header_picture (mpeg2dec_t * mpeg2dec);
static
int mpeg2_header_extension (mpeg2dec_t * mpeg2dec);
static
int mpeg2_header_user_data (mpeg2dec_t * mpeg2dec);
static
void mpeg2_header_sequence_finalize (mpeg2dec_t * mpeg2dec);
static
void mpeg2_header_gop_finalize (mpeg2dec_t * mpeg2dec);
static
void mpeg2_header_picture_finalize (mpeg2dec_t * mpeg2dec, uint32_t accels);
static
mpeg2_state_t mpeg2_header_slice_start (mpeg2dec_t * mpeg2dec);
static
mpeg2_state_t mpeg2_header_end (mpeg2dec_t * mpeg2dec);
static
void mpeg2_set_fbuf (mpeg2dec_t * mpeg2dec, int b_type);

/* idct.c */
static
void mpeg2_idct_init (uint32_t accel);

typedef struct {
    mpeg2_mc_fct * put [8];
    mpeg2_mc_fct * avg [8];
} mpeg2_mc_t;


/* FILE: mpeg2dec-0.4.0/libmpeg2/decode.c */

static int mpeg2_accels = 0;

#define BUFFER_SIZE (1194 * 1024)

static
const mpeg2_info_t * mpeg2_info (mpeg2dec_t * mpeg2dec)
{
    return &(mpeg2dec->info);
}

static inline int skip_chunk (mpeg2dec_t * mpeg2dec, int bytes)
{
    uint8_t * current;
    uint32_t shift;
    uint8_t * limit;
    uint8_t byte;

    if (!bytes)
	return 0;

    current = mpeg2dec->buf_start;
    shift = mpeg2dec->shift;
    limit = current + bytes;

    do {
	byte = *current++;
	if (shift == 0x00000100) {
	    int skipped;

	    mpeg2dec->shift = 0xffffff00;
	    skipped = current - mpeg2dec->buf_start;
	    mpeg2dec->buf_start = current;
	    return skipped;
	}
	shift = (shift | byte) << 8;
    } while (current < limit);

    mpeg2dec->shift = shift;
    mpeg2dec->buf_start = current;
    return 0;
}

static inline int copy_chunk (mpeg2dec_t * mpeg2dec, int bytes)
{
    uint8_t * current;
    uint32_t shift;
    uint8_t * chunk_ptr;
    uint8_t * limit;
    uint8_t byte;

    if (!bytes)
	return 0;

    current = mpeg2dec->buf_start;
    shift = mpeg2dec->shift;
    chunk_ptr = mpeg2dec->chunk_ptr;
    limit = current + bytes;

    do {
	byte = *current++;
	if (shift == 0x00000100) {
	    int copied;

	    mpeg2dec->shift = 0xffffff00;
	    mpeg2dec->chunk_ptr = chunk_ptr + 1;
	    copied = current - mpeg2dec->buf_start;
	    mpeg2dec->buf_start = current;
	    return copied;
	}
	shift = (shift | byte) << 8;
	*chunk_ptr++ = byte;
    } while (current < limit);

    mpeg2dec->shift = shift;
    mpeg2dec->buf_start = current;
    return 0;
}

static
void mpeg2_buffer (mpeg2dec_t * mpeg2dec, uint8_t * start, uint8_t * end)
{
    mpeg2dec->buf_start = start;
    mpeg2dec->buf_end = end;
}

static
int mpeg2_getpos (mpeg2dec_t * mpeg2dec)
{
    return mpeg2dec->buf_end - mpeg2dec->buf_start;
}

static inline mpeg2_state_t seek_chunk (mpeg2dec_t * mpeg2dec)
{
    int size, skipped;

    size = mpeg2dec->buf_end - mpeg2dec->buf_start;
    skipped = skip_chunk (mpeg2dec, size);
    if (!skipped) {
	mpeg2dec->bytes_since_tag += size;
	return STATE_BUFFER;
    }
    mpeg2dec->bytes_since_tag += skipped;
    mpeg2dec->code = mpeg2dec->buf_start[-1];
    return (mpeg2_state_t)-1;
}

static
mpeg2_state_t mpeg2_seek_header (mpeg2dec_t * mpeg2dec)
{
    while (mpeg2dec->code != 0xb3 &&
	   ((mpeg2dec->code != 0xb7 && mpeg2dec->code != 0xb8 &&
	     mpeg2dec->code) || mpeg2dec->sequence.width == (unsigned)-1))
	if (seek_chunk (mpeg2dec) == STATE_BUFFER)
	    return STATE_BUFFER;
    mpeg2dec->chunk_start = mpeg2dec->chunk_ptr = mpeg2dec->chunk_buffer;
    mpeg2dec->user_data_len = 0;
    return (mpeg2dec->code ? mpeg2_parse_header (mpeg2dec) :
	    mpeg2_header_picture_start (mpeg2dec));
}

#define RECEIVED(code,state) (((state) << 8) + (code))

static
mpeg2_state_t mpeg2_parse (mpeg2dec_t * mpeg2dec)
{
    int size_buffer, size_chunk, copied;

    if (mpeg2dec->action) {
	mpeg2_state_t state;

	state = mpeg2dec->action (mpeg2dec);
	if ((int)state >= 0)
	    return state;
    }

    while (1) {
	while ((unsigned) (mpeg2dec->code - mpeg2dec->first_decode_slice) <
	       mpeg2dec->nb_decode_slices) {
	    size_buffer = mpeg2dec->buf_end - mpeg2dec->buf_start;
	    size_chunk = (mpeg2dec->chunk_buffer + BUFFER_SIZE -
			  mpeg2dec->chunk_ptr);
	    if (size_buffer <= size_chunk) {
		copied = copy_chunk (mpeg2dec, size_buffer);
		if (!copied) {
		    mpeg2dec->bytes_since_tag += size_buffer;
		    mpeg2dec->chunk_ptr += size_buffer;
		    return STATE_BUFFER;
		}
	    } else {
		copied = copy_chunk (mpeg2dec, size_chunk);
		if (!copied) {
		    /* filled the chunk buffer without finding a start code */
		    mpeg2dec->bytes_since_tag += size_chunk;
		    mpeg2dec->action = seek_chunk;
		    return STATE_INVALID;
		}
	    }
	    mpeg2dec->bytes_since_tag += copied;

	    mpeg2_slice (&(mpeg2dec->decoder), mpeg2dec->code,
			 mpeg2dec->chunk_start);
	    mpeg2dec->code = mpeg2dec->buf_start[-1];
	    mpeg2dec->chunk_ptr = mpeg2dec->chunk_start;
	}
	if ((unsigned) (mpeg2dec->code - 1) >= 0xb0 - 1)
	    break;
	if (seek_chunk (mpeg2dec) == STATE_BUFFER)
	    return STATE_BUFFER;
    }

    switch (mpeg2dec->code) {
    case 0x00:
	mpeg2dec->action = mpeg2_header_picture_start;
	break;
    case 0xb7:
	mpeg2dec->action = mpeg2_header_end;
	break;
    case 0xb3:
    case 0xb8:
	mpeg2dec->action = mpeg2_parse_header;
	break;
    default:
	mpeg2dec->action = seek_chunk;
	return STATE_INVALID;
    }
    
    mpeg2dec->info.bytes             = mpeg2dec->bytes_since_tag;
    mpeg2dec->info.macro_intra       = mpeg2dec->decoder.macro_intra;
    mpeg2dec->info.macro_inter       = mpeg2dec->decoder.macro_inter;
    mpeg2dec->info.macro_inter_short = mpeg2dec->decoder.macro_inter_short;
    mpeg2dec->info.motion            = mpeg2dec->decoder.motion;
    
    mpeg2dec->bytes_since_tag           = 0;
    mpeg2dec->decoder.macro_intra       = 0;
    mpeg2dec->decoder.macro_inter       = 0;
    mpeg2dec->decoder.macro_inter_short = 0;
    mpeg2dec->decoder.motion            = 0;
    
    if (mpeg2dec->action == mpeg2_header_picture_start)
	return mpeg2dec->state;
    return (mpeg2dec->state == STATE_SLICE) ? STATE_SLICE : STATE_INVALID;
}

static
mpeg2_state_t mpeg2_parse_header (mpeg2dec_t * mpeg2dec)
{
    static int (* process_header[]) (mpeg2dec_t * mpeg2dec) = {
	mpeg2_header_picture, mpeg2_header_extension, mpeg2_header_user_data,
	mpeg2_header_sequence, NULL, NULL, NULL, NULL, mpeg2_header_gop
    };
    int size_buffer, size_chunk, copied;

    mpeg2dec->action = mpeg2_parse_header;
    mpeg2dec->info.user_data = NULL;	mpeg2dec->info.user_data_len = 0;
    while (1) {
	size_buffer = mpeg2dec->buf_end - mpeg2dec->buf_start;
	size_chunk = (mpeg2dec->chunk_buffer + BUFFER_SIZE -
		      mpeg2dec->chunk_ptr);
	if (size_buffer <= size_chunk) {
	    copied = copy_chunk (mpeg2dec, size_buffer);
	    if (!copied) {
		mpeg2dec->bytes_since_tag += size_buffer;
		mpeg2dec->chunk_ptr += size_buffer;
		return STATE_BUFFER;
	    }
	} else {
	    copied = copy_chunk (mpeg2dec, size_chunk);
	    if (!copied) {
		/* filled the chunk buffer without finding a start code */
		mpeg2dec->bytes_since_tag += size_chunk;
		mpeg2dec->code = 0xb4;
		mpeg2dec->action = mpeg2_seek_header;
		return STATE_INVALID;
	    }
	}
	mpeg2dec->bytes_since_tag += copied;

	if (process_header[mpeg2dec->code & 0x0b] (mpeg2dec)) {
	    mpeg2dec->code = mpeg2dec->buf_start[-1];
	    mpeg2dec->action = mpeg2_seek_header;
	    return STATE_INVALID;
	}

	mpeg2dec->code = mpeg2dec->buf_start[-1];
	switch (RECEIVED (mpeg2dec->code, mpeg2dec->state)) {

	/* state transition after a sequence header */
	case RECEIVED (0x00, STATE_SEQUENCE):
	    mpeg2dec->action = mpeg2_header_picture_start;
	case RECEIVED (0xb8, STATE_SEQUENCE):
	    mpeg2_header_sequence_finalize (mpeg2dec);
	    break;

	/* other legal state transitions */
	case RECEIVED (0x00, STATE_GOP):
	    mpeg2_header_gop_finalize (mpeg2dec);
	    mpeg2dec->action = mpeg2_header_picture_start;
	    break;
	case RECEIVED (0x01, STATE_PICTURE):
	case RECEIVED (0x01, STATE_PICTURE_2ND):
	    mpeg2_header_picture_finalize (mpeg2dec, mpeg2_accels);
	    mpeg2dec->action = mpeg2_header_slice_start;
	    break;

	/* legal headers within a given state */
	case RECEIVED (0xb2, STATE_SEQUENCE):
	case RECEIVED (0xb2, STATE_GOP):
	case RECEIVED (0xb2, STATE_PICTURE):
	case RECEIVED (0xb2, STATE_PICTURE_2ND):
	case RECEIVED (0xb5, STATE_SEQUENCE):
	case RECEIVED (0xb5, STATE_PICTURE):
	case RECEIVED (0xb5, STATE_PICTURE_2ND):
	    mpeg2dec->chunk_ptr = mpeg2dec->chunk_start;
	    continue;

	default:
	    mpeg2dec->action = mpeg2_seek_header;
	    return STATE_INVALID;
	}

	mpeg2dec->chunk_start = mpeg2dec->chunk_ptr = mpeg2dec->chunk_buffer;
	mpeg2dec->user_data_len = 0;
	return mpeg2dec->state;
    }
}

static
void mpeg2_reset (mpeg2dec_t * mpeg2dec, int full_reset)
{
    mpeg2dec->buf_start = mpeg2dec->buf_end = NULL;
    mpeg2dec->num_tags = 0;
    mpeg2dec->shift = 0xffffff00;
    mpeg2dec->code = 0xb4;
    mpeg2dec->action = mpeg2_seek_header;
    mpeg2dec->state = STATE_INVALID;
    mpeg2dec->first = 1;

    mpeg2_reset_info(&(mpeg2dec->info));
    mpeg2dec->info.gop = NULL;
    mpeg2dec->info.user_data = NULL;
    mpeg2dec->info.user_data_len = 0;
    if (full_reset) {
	mpeg2dec->info.sequence = NULL;
	mpeg2_header_state_init (mpeg2dec);
    }

    mpeg2dec->bytes_since_tag           = 0;
    mpeg2dec->decoder.macro_intra       = 0;
    mpeg2dec->decoder.macro_inter       = 0;
    mpeg2dec->decoder.macro_inter_short = 0;
    mpeg2dec->decoder.motion            = 0;
}

static
mpeg2dec_t * mpeg2_init (void)
{
    mpeg2dec_t * mpeg2dec;

    mpeg2_idct_init (0);

    mpeg2dec = (mpeg2dec_t *) mpeg2_malloc (sizeof (mpeg2dec_t),
					    MPEG2_ALLOC_MPEG2DEC);
    if (mpeg2dec == NULL)
	return NULL;

    memset (mpeg2dec->decoder.DCTblock, 0, 64 * sizeof (int16_t));
    memset (mpeg2dec->quantizer_matrix, 0, 4 * 64 * sizeof (uint8_t));

    mpeg2dec->chunk_buffer = (uint8_t *) mpeg2_malloc (BUFFER_SIZE + 4,
						       MPEG2_ALLOC_CHUNK);

    mpeg2dec->sequence.width = (unsigned)-1;
    mpeg2_reset (mpeg2dec, 1);

    return mpeg2dec;
}

static
void mpeg2_close (mpeg2dec_t * mpeg2dec)
{
    mpeg2_header_state_init (mpeg2dec);
    mpeg2_free (mpeg2dec->chunk_buffer);
    mpeg2_free (mpeg2dec);
}


/* FILE: mpeg2dec-0.4.0/libmpeg2/header.c */

#define SEQ_EXT 2
#define SEQ_DISPLAY_EXT 4
#define QUANT_MATRIX_EXT 8
#define COPYRIGHT_EXT 0x10
#define PIC_DISPLAY_EXT 0x80
#define PIC_CODING_EXT 0x100

/* default intra quant matrix, in zig-zag order */
static const uint8_t default_intra_quantizer_matrix[64] ATTR_ALIGN(16) = {
    8,
    16, 16,
    19, 16, 19,
    22, 22, 22, 22,
    22, 22, 26, 24, 26,
    27, 27, 27, 26, 26, 26,
    26, 27, 27, 27, 29, 29, 29,
    34, 34, 34, 29, 29, 29, 27, 27,
    29, 29, 32, 32, 34, 34, 37,
    38, 37, 35, 35, 34, 35,
    38, 38, 40, 40, 40,
    48, 48, 46, 46,
    56, 56, 58,
    69, 69,
    83
};

static
uint8_t mpeg2_scan_norm[64] ATTR_ALIGN(16) = {
    /* Zig-Zag scan pattern */
     0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

static
uint8_t mpeg2_scan_alt[64] ATTR_ALIGN(16) = {
    /* Alternate scan pattern */
     0, 8,  16, 24,  1,  9,  2, 10, 17, 25, 32, 40, 48, 56, 57, 49,
    41, 33, 26, 18,  3, 11,  4, 12, 19, 27, 34, 42, 50, 58, 35, 43,
    51, 59, 20, 28,  5, 13,  6, 14, 21, 29, 36, 44, 52, 60, 37, 45,
    53, 61, 22, 30,  7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63
};

static
void mpeg2_header_state_init (mpeg2dec_t * mpeg2dec)
{
    if (mpeg2dec->sequence.width != (unsigned)-1) {
	int i;

	mpeg2dec->sequence.width = (unsigned)-1;
	if (!mpeg2dec->custom_fbuf)
	    for (i = mpeg2dec->alloc_index_user;
		 i < mpeg2dec->alloc_index; i++) {
		mpeg2_free (mpeg2dec->fbuf_alloc[i].fbuf.buf[0]);
		mpeg2_free (mpeg2dec->fbuf_alloc[i].fbuf.buf[1]);
		mpeg2_free (mpeg2dec->fbuf_alloc[i].fbuf.buf[2]);
	    }
	if (mpeg2dec->convert_start)
	    for (i = 0; i < 3; i++) {
		mpeg2_free (mpeg2dec->yuv_buf[i][0]);
		mpeg2_free (mpeg2dec->yuv_buf[i][1]);
		mpeg2_free (mpeg2dec->yuv_buf[i][2]);
	    }
	if (mpeg2dec->decoder.convert_id)
	    mpeg2_free (mpeg2dec->decoder.convert_id);
    }
    mpeg2dec->decoder.coding_type = I_TYPE;
    mpeg2dec->decoder.convert = NULL;
    mpeg2dec->decoder.convert_id = NULL;
    mpeg2dec->picture = mpeg2dec->pictures;
    mpeg2dec->fbuf[0] = &mpeg2dec->fbuf_alloc[0].fbuf;
    mpeg2dec->fbuf[1] = &mpeg2dec->fbuf_alloc[1].fbuf;
    mpeg2dec->fbuf[2] = &mpeg2dec->fbuf_alloc[2].fbuf;
    mpeg2dec->first = 1;
    mpeg2dec->alloc_index = 0;
    mpeg2dec->alloc_index_user = 0;
    mpeg2dec->first_decode_slice = 1;
    mpeg2dec->nb_decode_slices = 0xb0 - 1;
    mpeg2dec->convert = NULL;
    mpeg2dec->convert_start = NULL;
    mpeg2dec->custom_fbuf = 0;
    mpeg2dec->yuv_index = 0;
}

static
void mpeg2_reset_info (mpeg2_info_t * info)
{
    info->current_picture = info->current_picture_2nd = NULL;
    info->display_picture = info->display_picture_2nd = NULL;
    info->current_fbuf = info->display_fbuf = info->discard_fbuf = NULL;
}

static void info_user_data (mpeg2dec_t * mpeg2dec)
{
    if (mpeg2dec->user_data_len) {
	mpeg2dec->info.user_data = mpeg2dec->chunk_buffer;
	mpeg2dec->info.user_data_len = mpeg2dec->user_data_len - 3;
    }
}

static
int mpeg2_header_sequence (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    mpeg2_sequence_t * sequence = &(mpeg2dec->new_sequence);
    static unsigned int frame_period[16] = {
	0, 1126125, 1125000, 1080000, 900900, 900000, 540000, 450450, 450000,
	/* unofficial: xing 15 fps */
	1800000,
	/* unofficial: libmpeg3 "Unofficial economy rates" 5/10/12/15 fps */
	5400000, 2700000, 2250000, 1800000, 0, 0
    };
    int i;

    if ((buffer[6] & 0x20) != 0x20)	/* missing marker_bit */
	return 1;

    i = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
    if (! (sequence->display_width = sequence->picture_width = i >> 12))
	return 1;
    if (! (sequence->display_height = sequence->picture_height = i & 0xfff))
	return 1;
    sequence->width = (sequence->picture_width + 15) & ~15;
    sequence->height = (sequence->picture_height + 15) & ~15;
    sequence->chroma_width = sequence->width >> 1;
    sequence->chroma_height = sequence->height >> 1;

    sequence->flags = (SEQ_FLAG_PROGRESSIVE_SEQUENCE |
		       SEQ_VIDEO_FORMAT_UNSPECIFIED);

    sequence->pixel_width = buffer[3] >> 4;	/* aspect ratio */
    sequence->frame_period = frame_period[buffer[3] & 15];

    sequence->byte_rate = (buffer[4]<<10) | (buffer[5]<<2) | (buffer[6]>>6);

    sequence->vbv_buffer_size = ((buffer[6]<<16)|(buffer[7]<<8))&0x1ff800;

    if (buffer[7] & 4)
	sequence->flags |= SEQ_FLAG_CONSTRAINED_PARAMETERS;

    mpeg2dec->copy_matrix = 3;
    if (buffer[7] & 2) {
	for (i = 0; i < 64; i++)
	    mpeg2dec->new_quantizer_matrix[0][mpeg2_scan_norm[i]] =
		(buffer[i+7] << 7) | (buffer[i+8] >> 1);
	buffer += 64;
    } else
	for (i = 0; i < 64; i++)
	    mpeg2dec->new_quantizer_matrix[0][mpeg2_scan_norm[i]] =
		default_intra_quantizer_matrix[i];

    if (buffer[7] & 1)
	for (i = 0; i < 64; i++)
	    mpeg2dec->new_quantizer_matrix[1][mpeg2_scan_norm[i]] =
		buffer[i+8];
    else
	memset (mpeg2dec->new_quantizer_matrix[1], 16, 64);

    sequence->profile_level_id = 0x80;
    sequence->colour_primaries = 0;
    sequence->transfer_characteristics = 0;
    sequence->matrix_coefficients = 0;

    mpeg2dec->ext_state = SEQ_EXT;
    mpeg2dec->state = STATE_SEQUENCE;
    mpeg2dec->display_offset_x = mpeg2dec->display_offset_y = 0;

    return 0;
}

static int sequence_ext (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    mpeg2_sequence_t * sequence = &(mpeg2dec->new_sequence);
    uint32_t flags;

    if (!(buffer[3] & 1))
	return 1;

    sequence->profile_level_id = (buffer[0] << 4) | (buffer[1] >> 4);

    sequence->display_width = sequence->picture_width +=
	((buffer[1] << 13) | (buffer[2] << 5)) & 0x3000;
    sequence->display_height = sequence->picture_height +=
	(buffer[2] << 7) & 0x3000;
    sequence->width = (sequence->picture_width + 15) & ~15;
    sequence->height = (sequence->picture_height + 15) & ~15;
    flags = sequence->flags | SEQ_FLAG_MPEG2;
    if (!(buffer[1] & 8)) {
	flags &= ~SEQ_FLAG_PROGRESSIVE_SEQUENCE;
	sequence->height = (sequence->height + 31) & ~31;
    }
    if (buffer[5] & 0x80)
	flags |= SEQ_FLAG_LOW_DELAY;
    sequence->flags = flags;
    sequence->chroma_width = sequence->width;
    sequence->chroma_height = sequence->height;
    switch (buffer[1] & 6) {
    case 0:	/* invalid */
	return 1;
    case 2:	/* 4:2:0 */
	sequence->chroma_height >>= 1;
    case 4:	/* 4:2:2 */
	sequence->chroma_width >>= 1;
    }

    sequence->byte_rate += ((buffer[2]<<25) | (buffer[3]<<17)) & 0x3ffc0000;

    sequence->vbv_buffer_size |= buffer[4] << 21;

    sequence->frame_period =
	sequence->frame_period * ((buffer[5]&31)+1) / (((buffer[5]>>2)&3)+1);

    mpeg2dec->ext_state = SEQ_DISPLAY_EXT;

    return 0;
}

static int sequence_display_ext (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    mpeg2_sequence_t * sequence = &(mpeg2dec->new_sequence);
    uint32_t flags;

    flags = ((sequence->flags & ~SEQ_MASK_VIDEO_FORMAT) |
	     ((buffer[0]<<4) & SEQ_MASK_VIDEO_FORMAT));
    if (buffer[0] & 1) {
	flags |= SEQ_FLAG_COLOUR_DESCRIPTION;
	sequence->colour_primaries = buffer[1];
	sequence->transfer_characteristics = buffer[2];
	sequence->matrix_coefficients = buffer[3];
	buffer += 3;
    }

    if (!(buffer[2] & 2))	/* missing marker_bit */
	return 1;

    sequence->display_width = (buffer[1] << 6) | (buffer[2] >> 2);
    sequence->display_height =
	((buffer[2]& 1 ) << 13) | (buffer[3] << 5) | (buffer[4] >> 3);

    return 0;
}

static inline void finalize_sequence (mpeg2_sequence_t * sequence)
{
    int width;
    int height;

    sequence->byte_rate *= 50;

    if (sequence->flags & SEQ_FLAG_MPEG2) {
	switch (sequence->pixel_width) {
	case 1:		/* square pixels */
	    sequence->pixel_width = sequence->pixel_height = 1;	return;
	case 2:		/* 4:3 aspect ratio */
	    width = 4; height = 3;	break;
	case 3:		/* 16:9 aspect ratio */
	    width = 16; height = 9;	break;
	case 4:		/* 2.21:1 aspect ratio */
	    width = 221; height = 100;	break;
	default:	/* illegal */
	    sequence->pixel_width = sequence->pixel_height = 0;	return;
	}
	width *= sequence->display_height;
	height *= sequence->display_width;

    } else {
	if (sequence->byte_rate == 50 * 0x3ffff) 
	    sequence->byte_rate = 0;        /* mpeg-1 VBR */ 

	switch (sequence->pixel_width) {
	case 0:	case 15:	/* illegal */
	    sequence->pixel_width = sequence->pixel_height = 0;		return;
	case 1:	/* square pixels */
	    sequence->pixel_width = sequence->pixel_height = 1;		return;
	case 3:	/* 720x576 16:9 */
	    sequence->pixel_width = 64;	sequence->pixel_height = 45;	return;
	case 6:	/* 720x480 16:9 */
	    sequence->pixel_width = 32;	sequence->pixel_height = 27;	return;
	case 12:	/* 720*480 4:3 */
	    sequence->pixel_width = 8;	sequence->pixel_height = 9;	return;
	default:
	    height = 88 * sequence->pixel_width + 1171;
	    width = 2000;
	}
    }

    sequence->pixel_width = width;
    sequence->pixel_height = height;
    while (width) {	/* find greatest common divisor */
	int tmp = width;
	width = height % tmp;
	height = tmp;
    }
    sequence->pixel_width /= height;
    sequence->pixel_height /= height;
}

static void copy_matrix (mpeg2dec_t * mpeg2dec, int index)
{
    if (memcmp (mpeg2dec->quantizer_matrix[index],
		mpeg2dec->new_quantizer_matrix[index], 64)) {
	memcpy (mpeg2dec->quantizer_matrix[index],
		mpeg2dec->new_quantizer_matrix[index], 64);
	mpeg2dec->scaled[index] = -1;
    }
}

static void finalize_matrix (mpeg2dec_t * mpeg2dec)
{
    mpeg2_decoder_t * decoder = &(mpeg2dec->decoder);
    int i;

    for (i = 0; i < 2; i++) {
	if (mpeg2dec->copy_matrix & (1 << i))
	    copy_matrix (mpeg2dec, i);
	if ((mpeg2dec->copy_matrix & (4 << i)) &&
	    memcmp (mpeg2dec->quantizer_matrix[i],
		    mpeg2dec->new_quantizer_matrix[i+2], 64)) {
	    copy_matrix (mpeg2dec, i + 2);
	    decoder->chroma_quantizer[i] = decoder->quantizer_prescale[i+2];
	} else if (mpeg2dec->copy_matrix & (5 << i))
	    decoder->chroma_quantizer[i] = decoder->quantizer_prescale[i];
    }
}

static mpeg2_state_t invalid_end_action (mpeg2dec_t * mpeg2dec)
{
    mpeg2_reset_info (&(mpeg2dec->info));
    mpeg2dec->info.gop = NULL;
    info_user_data (mpeg2dec);
    mpeg2_header_state_init (mpeg2dec);
    mpeg2dec->sequence = mpeg2dec->new_sequence;
    mpeg2dec->action = mpeg2_seek_header;
    mpeg2dec->state = STATE_SEQUENCE;
    return STATE_SEQUENCE;
}

static
void mpeg2_header_sequence_finalize (mpeg2dec_t * mpeg2dec)
{
    mpeg2_sequence_t * sequence = &(mpeg2dec->new_sequence);
    mpeg2_decoder_t * decoder = &(mpeg2dec->decoder);

    finalize_sequence (sequence);
    finalize_matrix (mpeg2dec);

    decoder->mpeg1 = !(sequence->flags & SEQ_FLAG_MPEG2);
    decoder->width = sequence->width;
    decoder->height = sequence->height;
    decoder->vertical_position_extension = (sequence->picture_height > 2800);
    decoder->chroma_format = ((sequence->chroma_width == sequence->width) +
			      (sequence->chroma_height == sequence->height));

    if (mpeg2dec->sequence.width != (unsigned)-1) {
	unsigned int new_byte_rate;

	/*
	 * According to 6.1.1.6, repeat sequence headers should be
	 * identical to the original. However some DVDs dont respect
	 * that and have different bitrates in the repeat sequence
	 * headers. So we'll ignore that in the comparison and still
	 * consider these as repeat sequence headers.
	 *
	 * However, be careful not to alter the current sequence when
	 * returning STATE_INVALID_END.
	 */
	new_byte_rate = sequence->byte_rate;
	sequence->byte_rate = mpeg2dec->sequence.byte_rate;
	if (memcmp (&(mpeg2dec->sequence), sequence,
		    sizeof (mpeg2_sequence_t))) {
	    decoder->stride_frame = sequence->width;
	    sequence->byte_rate = new_byte_rate;
	    mpeg2_header_end (mpeg2dec);
	    mpeg2dec->action = invalid_end_action;
	    mpeg2dec->state = STATE_INVALID_END;
	    return;
	}
	sequence->byte_rate = new_byte_rate;
	mpeg2dec->state = STATE_SEQUENCE_REPEATED;
    } else
	decoder->stride_frame = sequence->width;
    mpeg2dec->sequence = *sequence;
    mpeg2_reset_info (&(mpeg2dec->info));
    mpeg2dec->info.sequence = &(mpeg2dec->sequence);
    mpeg2dec->info.gop = NULL;
    info_user_data (mpeg2dec);
}

static
int mpeg2_header_gop (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    mpeg2_gop_t * gop = &(mpeg2dec->new_gop);

    if (! (buffer[1] & 8))
	return 1;
    gop->hours = (buffer[0] >> 2) & 31;
    gop->minutes = ((buffer[0] << 4) | (buffer[1] >> 4)) & 63;
    gop->seconds = ((buffer[1] << 3) | (buffer[2] >> 5)) & 63;
    gop->pictures = ((buffer[2] << 1) | (buffer[3] >> 7)) & 63;
    gop->flags = (buffer[0] >> 7) | ((buffer[3] >> 4) & 6);
    mpeg2dec->state = STATE_GOP;
    return 0;
}

static
void mpeg2_header_gop_finalize (mpeg2dec_t * mpeg2dec)
{
    mpeg2dec->gop = mpeg2dec->new_gop;
    mpeg2_reset_info (&(mpeg2dec->info));
    mpeg2dec->info.gop = &(mpeg2dec->gop);
    info_user_data (mpeg2dec);
}

static
void mpeg2_set_fbuf (mpeg2dec_t * mpeg2dec, int b_type)
{
    int i;

    for (i = 0; i < 3; i++)
	if (mpeg2dec->fbuf[1] != &mpeg2dec->fbuf_alloc[i].fbuf &&
	    mpeg2dec->fbuf[2] != &mpeg2dec->fbuf_alloc[i].fbuf) {
	    mpeg2dec->fbuf[0] = &mpeg2dec->fbuf_alloc[i].fbuf;
	    mpeg2dec->info.current_fbuf = mpeg2dec->fbuf[0];
	    if (b_type || (mpeg2dec->sequence.flags & SEQ_FLAG_LOW_DELAY)) {
		if (b_type || mpeg2dec->convert)
		    mpeg2dec->info.discard_fbuf = mpeg2dec->fbuf[0];
		mpeg2dec->info.display_fbuf = mpeg2dec->fbuf[0];
	    }
	    break;
	}
}

static
mpeg2_state_t mpeg2_header_picture_start (mpeg2dec_t * mpeg2dec)
{
    mpeg2_picture_t * picture = &(mpeg2dec->new_picture);

    mpeg2dec->state = ((mpeg2dec->state != STATE_SLICE_1ST) ?
		       STATE_PICTURE : STATE_PICTURE_2ND);
    picture->flags = 0;
    picture->tag = picture->tag2 = 0;
    if (mpeg2dec->num_tags) {
	if (mpeg2dec->bytes_since_tag >= 4) {
	    mpeg2dec->num_tags = 0;
	    picture->tag = mpeg2dec->tag_current;
	    picture->tag2 = mpeg2dec->tag2_current;
	    picture->flags = PIC_FLAG_TAGS;
	} else if (mpeg2dec->num_tags > 1) {
	    mpeg2dec->num_tags = 1;
	    picture->tag = mpeg2dec->tag_previous;
	    picture->tag2 = mpeg2dec->tag2_previous;
	    picture->flags = PIC_FLAG_TAGS;
	}
    }
    picture->display_offset[0].x = picture->display_offset[1].x =
	picture->display_offset[2].x = mpeg2dec->display_offset_x;
    picture->display_offset[0].y = picture->display_offset[1].y =
	picture->display_offset[2].y = mpeg2dec->display_offset_y;
    return mpeg2_parse_header (mpeg2dec);
}

static
int mpeg2_header_picture (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    mpeg2_picture_t * picture = &(mpeg2dec->new_picture);
    mpeg2_decoder_t * decoder = &(mpeg2dec->decoder);
    int type;

    type = (buffer [1] >> 3) & 7;
    mpeg2dec->ext_state = PIC_CODING_EXT;

    picture->temporal_reference = (buffer[0] << 2) | (buffer[1] >> 6);

    picture->flags |= type;

    if (type == PIC_FLAG_CODING_TYPE_P || type == PIC_FLAG_CODING_TYPE_B) {
	/* forward_f_code and backward_f_code - used in mpeg1 only */
	decoder->f_motion.f_code[1] = (buffer[3] >> 2) & 1;
	decoder->f_motion.f_code[0] =
	    (((buffer[3] << 1) | (buffer[4] >> 7)) & 7) - 1;
	decoder->b_motion.f_code[1] = (buffer[4] >> 6) & 1;
	decoder->b_motion.f_code[0] = ((buffer[4] >> 3) & 7) - 1;
    }

    /* XXXXXX decode extra_information_picture as well */

    picture->nb_fields = 2;

    mpeg2dec->q_scale_type = 0;
    decoder->intra_dc_precision = 7;
    decoder->frame_pred_frame_dct = 1;
    decoder->concealment_motion_vectors = 0;
    decoder->scan = mpeg2_scan_norm;
    decoder->picture_structure = FRAME_PICTURE;
    mpeg2dec->copy_matrix = 0;

    return 0;
}

static int picture_coding_ext (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    mpeg2_picture_t * picture = &(mpeg2dec->new_picture);
    mpeg2_decoder_t * decoder = &(mpeg2dec->decoder);
    uint32_t flags;

    /* pre subtract 1 for use later in compute_motion_vector */
    decoder->f_motion.f_code[0] = (buffer[0] & 15) - 1;
    decoder->f_motion.f_code[1] = (buffer[1] >> 4) - 1;
    decoder->b_motion.f_code[0] = (buffer[1] & 15) - 1;
    decoder->b_motion.f_code[1] = (buffer[2] >> 4) - 1;

    flags = picture->flags;
    decoder->intra_dc_precision = 7 - ((buffer[2] >> 2) & 3);
    decoder->picture_structure = buffer[2] & 3;
    switch (decoder->picture_structure) {
    case TOP_FIELD:
	flags |= PIC_FLAG_TOP_FIELD_FIRST;
    case BOTTOM_FIELD:
	picture->nb_fields = 1;
	break;
    case FRAME_PICTURE:
	if (!(mpeg2dec->sequence.flags & SEQ_FLAG_PROGRESSIVE_SEQUENCE)) {
	    picture->nb_fields = (buffer[3] & 2) ? 3 : 2;
	    flags |= (buffer[3] & 128) ? PIC_FLAG_TOP_FIELD_FIRST : 0;
	} else
	    picture->nb_fields = (buffer[3]&2) ? ((buffer[3]&128) ? 6 : 4) : 2;
	break;
    default:
	return 1;
    }
    decoder->top_field_first = buffer[3] >> 7;
    decoder->frame_pred_frame_dct = (buffer[3] >> 6) & 1;
    decoder->concealment_motion_vectors = (buffer[3] >> 5) & 1;
    mpeg2dec->q_scale_type = buffer[3] & 16;
    decoder->intra_vlc_format = (buffer[3] >> 3) & 1;
    decoder->scan = (buffer[3] & 4) ? mpeg2_scan_alt : mpeg2_scan_norm;
    flags |= (buffer[4] & 0x80) ? PIC_FLAG_PROGRESSIVE_FRAME : 0;
    if (buffer[4] & 0x40)
	flags |= (((buffer[4]<<26) | (buffer[5]<<18) | (buffer[6]<<10)) &
		  PIC_MASK_COMPOSITE_DISPLAY) | PIC_FLAG_COMPOSITE_DISPLAY;
    picture->flags = flags;

    mpeg2dec->ext_state = PIC_DISPLAY_EXT | COPYRIGHT_EXT | QUANT_MATRIX_EXT;

    return 0;
}

static int picture_display_ext (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    mpeg2_picture_t * picture = &(mpeg2dec->new_picture);
    int i, nb_pos;

    nb_pos = picture->nb_fields;
    if (mpeg2dec->sequence.flags & SEQ_FLAG_PROGRESSIVE_SEQUENCE)
	nb_pos >>= 1;

    for (i = 0; i < nb_pos; i++) {
	int x, y;

	x = ((buffer[4*i] << 24) | (buffer[4*i+1] << 16) |
	     (buffer[4*i+2] << 8) | buffer[4*i+3]) >> (11-2*i);
	y = ((buffer[4*i+2] << 24) | (buffer[4*i+3] << 16) |
	     (buffer[4*i+4] << 8) | buffer[4*i+5]) >> (10-2*i);
	if (! (x & y & 1))
	    return 1;
	picture->display_offset[i].x = mpeg2dec->display_offset_x = x >> 1;
	picture->display_offset[i].y = mpeg2dec->display_offset_y = y >> 1;
    }
    for (; i < 3; i++) {
	picture->display_offset[i].x = mpeg2dec->display_offset_x;
	picture->display_offset[i].y = mpeg2dec->display_offset_y;
    }
    return 0;
}

static
void mpeg2_header_picture_finalize (mpeg2dec_t * mpeg2dec, uint32_t accels)
{
    mpeg2_decoder_t * decoder = &(mpeg2dec->decoder);
    int old_type_b = (decoder->coding_type == B_TYPE);
    int low_delay = mpeg2dec->sequence.flags & SEQ_FLAG_LOW_DELAY;

    finalize_matrix (mpeg2dec);
    decoder->coding_type = mpeg2dec->new_picture.flags & PIC_MASK_CODING_TYPE;

    if (mpeg2dec->state == STATE_PICTURE) {
	mpeg2_picture_t * picture;
	mpeg2_picture_t * other;

	decoder->second_field = 0;

	picture = other = mpeg2dec->pictures;
	if (old_type_b ^ (mpeg2dec->picture < mpeg2dec->pictures + 2))
	    picture += 2;
	else
	    other += 2;
	mpeg2dec->picture = picture;
	*picture = mpeg2dec->new_picture;

	if (!old_type_b) {
	    mpeg2dec->fbuf[2] = mpeg2dec->fbuf[1];
	    mpeg2dec->fbuf[1] = mpeg2dec->fbuf[0];
	}
	mpeg2dec->fbuf[0] = NULL;
	mpeg2_reset_info (&(mpeg2dec->info));
	mpeg2dec->info.current_picture = picture;
	mpeg2dec->info.display_picture = picture;
	if (decoder->coding_type != B_TYPE) {
	    if (!low_delay) {
		if (mpeg2dec->first) {
		    mpeg2dec->info.display_picture = NULL;
		    mpeg2dec->first = 0;
		} else {
		    mpeg2dec->info.display_picture = other;
		    if (other->nb_fields == 1)
			mpeg2dec->info.display_picture_2nd = other + 1;
		    mpeg2dec->info.display_fbuf = mpeg2dec->fbuf[1];
		}
	    }
	    if (!low_delay + !mpeg2dec->convert)
		mpeg2dec->info.discard_fbuf =
		    mpeg2dec->fbuf[!low_delay + !mpeg2dec->convert];
	}
	if (mpeg2dec->convert) {
	    mpeg2_convert_init_t convert_init;
	    if (!mpeg2dec->convert_start) {
		int y_size, uv_size;

		mpeg2dec->decoder.convert_id =
		    mpeg2_malloc (mpeg2dec->convert_id_size,
				  MPEG2_ALLOC_CONVERT_ID);
		mpeg2dec->convert (MPEG2_CONVERT_START,
				   mpeg2dec->decoder.convert_id,
				   &(mpeg2dec->sequence),
				   mpeg2dec->convert_stride, accels,
				   mpeg2dec->convert_arg, &convert_init);
		mpeg2dec->convert_start = convert_init.start;
		mpeg2dec->decoder.convert = convert_init.copy;

		y_size = decoder->stride_frame * mpeg2dec->sequence.height;
		uv_size = y_size >> (2 - mpeg2dec->decoder.chroma_format);
		mpeg2dec->yuv_buf[0][0] =
		    (uint8_t *) mpeg2_malloc (y_size, MPEG2_ALLOC_YUV);
		mpeg2dec->yuv_buf[0][1] =
		    (uint8_t *) mpeg2_malloc (uv_size, MPEG2_ALLOC_YUV);
		mpeg2dec->yuv_buf[0][2] =
		    (uint8_t *) mpeg2_malloc (uv_size, MPEG2_ALLOC_YUV);
		mpeg2dec->yuv_buf[1][0] =
		    (uint8_t *) mpeg2_malloc (y_size, MPEG2_ALLOC_YUV);
		mpeg2dec->yuv_buf[1][1] =
		    (uint8_t *) mpeg2_malloc (uv_size, MPEG2_ALLOC_YUV);
		mpeg2dec->yuv_buf[1][2] =
		    (uint8_t *) mpeg2_malloc (uv_size, MPEG2_ALLOC_YUV);
		y_size = decoder->stride_frame * 32;
		uv_size = y_size >> (2 - mpeg2dec->decoder.chroma_format);
		mpeg2dec->yuv_buf[2][0] =
		    (uint8_t *) mpeg2_malloc (y_size, MPEG2_ALLOC_YUV);
		mpeg2dec->yuv_buf[2][1] =
		    (uint8_t *) mpeg2_malloc (uv_size, MPEG2_ALLOC_YUV);
		mpeg2dec->yuv_buf[2][2] =
		    (uint8_t *) mpeg2_malloc (uv_size, MPEG2_ALLOC_YUV);
	    }
	    if (!mpeg2dec->custom_fbuf) {
		while (mpeg2dec->alloc_index < 3) {
		    mpeg2_fbuf_t * fbuf;

		    fbuf = &mpeg2dec->fbuf_alloc[mpeg2dec->alloc_index++].fbuf;
		    fbuf->id = NULL;
		    fbuf->buf[0] =
			(uint8_t *) mpeg2_malloc (convert_init.buf_size[0],
						  MPEG2_ALLOC_CONVERTED);
		    fbuf->buf[1] =
			(uint8_t *) mpeg2_malloc (convert_init.buf_size[1],
						  MPEG2_ALLOC_CONVERTED);
		    fbuf->buf[2] =
			(uint8_t *) mpeg2_malloc (convert_init.buf_size[2],
						  MPEG2_ALLOC_CONVERTED);
		}
		mpeg2_set_fbuf (mpeg2dec, (decoder->coding_type == B_TYPE));
	    }
	} else if (!mpeg2dec->custom_fbuf) {
	    while (mpeg2dec->alloc_index < 3) {
		mpeg2_fbuf_t * fbuf;
		int y_size, uv_size;

		fbuf = &(mpeg2dec->fbuf_alloc[mpeg2dec->alloc_index++].fbuf);
		fbuf->id = NULL;
		y_size = decoder->stride_frame * mpeg2dec->sequence.height;
		uv_size = y_size >> (2 - decoder->chroma_format);
		fbuf->buf[0] = (uint8_t *) mpeg2_malloc (y_size,
							 MPEG2_ALLOC_YUV);
		fbuf->buf[1] = (uint8_t *) mpeg2_malloc (uv_size,
							 MPEG2_ALLOC_YUV);
		fbuf->buf[2] = (uint8_t *) mpeg2_malloc (uv_size,
							 MPEG2_ALLOC_YUV);
	    }
	    mpeg2_set_fbuf (mpeg2dec, (decoder->coding_type == B_TYPE));
	}
    } else {
	decoder->second_field = 1;
	mpeg2dec->picture++;	/* second field picture */
	*(mpeg2dec->picture) = mpeg2dec->new_picture;
	mpeg2dec->info.current_picture_2nd = mpeg2dec->picture;
	if (low_delay || decoder->coding_type == B_TYPE)
	    mpeg2dec->info.display_picture_2nd = mpeg2dec->picture;
    }

    info_user_data (mpeg2dec);
}

static int copyright_ext (mpeg2dec_t * mpeg2dec)
{
    return 0;
}

static int quant_matrix_ext (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    int i, j;

    for (i = 0; i < 4; i++)
	if (buffer[0] & (8 >> i)) {
	    for (j = 0; j < 64; j++)
		mpeg2dec->new_quantizer_matrix[i][mpeg2_scan_norm[j]] =
		    (buffer[j] << (i+5)) | (buffer[j+1] >> (3-i));
	    mpeg2dec->copy_matrix |= 1 << i;
	    buffer += 64;
	}

    return 0;
}

static
int mpeg2_header_extension (mpeg2dec_t * mpeg2dec)
{
    static int (* parser[]) (mpeg2dec_t *) = {
	0, sequence_ext, sequence_display_ext, quant_matrix_ext,
	copyright_ext, 0, 0, picture_display_ext, picture_coding_ext
    };
    int ext, ext_bit;

    ext = mpeg2dec->chunk_start[0] >> 4;
    ext_bit = 1 << ext;

    if (!(mpeg2dec->ext_state & ext_bit))
	return 0;	/* ignore illegal extensions */
    mpeg2dec->ext_state &= ~ext_bit;
    return parser[ext] (mpeg2dec);
}

static
int mpeg2_header_user_data (mpeg2dec_t * mpeg2dec)
{
    mpeg2dec->user_data_len += mpeg2dec->chunk_ptr - 1 - mpeg2dec->chunk_start;
    mpeg2dec->chunk_start = mpeg2dec->chunk_ptr - 1;
    
    return 0;
}

static void prescale (mpeg2dec_t * mpeg2dec, int index)
{
    static int non_linear_scale [] = {
	 0,  1,  2,  3,  4,  5,   6,   7,
	 8, 10, 12, 14, 16, 18,  20,  22,
	24, 28, 32, 36, 40, 44,  48,  52,
	56, 64, 72, 80, 88, 96, 104, 112
    };
    int i, j, k;
    mpeg2_decoder_t * decoder = &(mpeg2dec->decoder);

    if (mpeg2dec->scaled[index] != mpeg2dec->q_scale_type) {
	mpeg2dec->scaled[index] = mpeg2dec->q_scale_type;
	for (i = 0; i < 32; i++) {
	    k = mpeg2dec->q_scale_type ? non_linear_scale[i] : (i << 1);
	    for (j = 0; j < 64; j++)
		decoder->quantizer_prescale[index][i][j] =
		    k * mpeg2dec->quantizer_matrix[index][j];
	}
    }
}

static
mpeg2_state_t mpeg2_header_slice_start (mpeg2dec_t * mpeg2dec)
{
    mpeg2_decoder_t * decoder = &(mpeg2dec->decoder);

    mpeg2dec->info.user_data = NULL;	mpeg2dec->info.user_data_len = 0;
    mpeg2dec->state = ((mpeg2dec->picture->nb_fields > 1 ||
			mpeg2dec->state == STATE_PICTURE_2ND) ?
		       STATE_SLICE : STATE_SLICE_1ST);

    if (mpeg2dec->decoder.coding_type != D_TYPE) {
	prescale (mpeg2dec, 0);
	if (decoder->chroma_quantizer[0] == decoder->quantizer_prescale[2])
	    prescale (mpeg2dec, 2);
	if (mpeg2dec->decoder.coding_type != I_TYPE) {
	    prescale (mpeg2dec, 1);
	    if (decoder->chroma_quantizer[1] == decoder->quantizer_prescale[3])
		prescale (mpeg2dec, 3);
	}
    }

    if (!(mpeg2dec->nb_decode_slices))
	mpeg2dec->picture->flags |= PIC_FLAG_SKIP;
    else if (mpeg2dec->convert_start) {
	mpeg2dec->convert_start (decoder->convert_id, mpeg2dec->fbuf[0],
				 mpeg2dec->picture, mpeg2dec->info.gop);

	if (mpeg2dec->decoder.coding_type == B_TYPE)
	    mpeg2_init_fbuf (&(mpeg2dec->decoder), mpeg2dec->yuv_buf[2],
			     mpeg2dec->yuv_buf[mpeg2dec->yuv_index ^ 1],
			     mpeg2dec->yuv_buf[mpeg2dec->yuv_index]);
	else {
	    mpeg2_init_fbuf (&(mpeg2dec->decoder),
			     mpeg2dec->yuv_buf[mpeg2dec->yuv_index ^ 1],
			     mpeg2dec->yuv_buf[mpeg2dec->yuv_index],
			     mpeg2dec->yuv_buf[mpeg2dec->yuv_index]);
	    if (mpeg2dec->state == STATE_SLICE)
		mpeg2dec->yuv_index ^= 1;
	}
    } else {
	int b_type;

	b_type = (mpeg2dec->decoder.coding_type == B_TYPE);
	mpeg2_init_fbuf (&(mpeg2dec->decoder), mpeg2dec->fbuf[0]->buf,
			 mpeg2dec->fbuf[b_type + 1]->buf,
			 mpeg2dec->fbuf[b_type]->buf);
    }
    mpeg2dec->action = NULL;
    return (mpeg2_state_t)-1;
}

static mpeg2_state_t seek_sequence (mpeg2dec_t * mpeg2dec)
{
    mpeg2_reset_info (&(mpeg2dec->info));
    mpeg2dec->info.sequence = NULL;
    mpeg2dec->info.gop = NULL;
    mpeg2_header_state_init (mpeg2dec);
    mpeg2dec->action = mpeg2_seek_header;
    return mpeg2_seek_header (mpeg2dec);
}

static
mpeg2_state_t mpeg2_header_end (mpeg2dec_t * mpeg2dec)
{
    mpeg2_picture_t * picture;
    int b_type;

    b_type = (mpeg2dec->decoder.coding_type == B_TYPE);
    picture = mpeg2dec->pictures;
    if ((mpeg2dec->picture >= picture + 2) ^ b_type)
	picture = mpeg2dec->pictures + 2;

    mpeg2_reset_info (&(mpeg2dec->info));
    if (!(mpeg2dec->sequence.flags & SEQ_FLAG_LOW_DELAY)) {
	mpeg2dec->info.display_picture = picture;
	if (picture->nb_fields == 1)
	    mpeg2dec->info.display_picture_2nd = picture + 1;
	mpeg2dec->info.display_fbuf = mpeg2dec->fbuf[b_type];
	if (!mpeg2dec->convert)
	    mpeg2dec->info.discard_fbuf = mpeg2dec->fbuf[b_type + 1];
    } else if (!mpeg2dec->convert)
	mpeg2dec->info.discard_fbuf = mpeg2dec->fbuf[b_type];
    mpeg2dec->action = seek_sequence;
    return STATE_END;
}


/* FILE: mpeg2dec-0.4.0/libmpeg2/vlc.h */

#define GETWORD(bit_buf,shift,bit_ptr)				\
do {								\
    bit_buf |= ((bit_ptr[0] << 8) | bit_ptr[1]) << (shift);	\
    bit_ptr += 2;						\
} while (0)

static inline void bitstream_init (mpeg2_decoder_t * decoder,
				   const uint8_t * start)
{
    decoder->bitstream_buf =
	(start[0] << 24) | (start[1] << 16) | (start[2] << 8) | start[3];
    decoder->bitstream_ptr = start + 4;
    decoder->bitstream_bits = -16;
}

/* make sure that there are at least 16 valid bits in bit_buf */
#define NEEDBITS(bit_buf,bits,bit_ptr)		\
do {						\
    if (unlikely (bits > 0)) {			\
	GETWORD (bit_buf, bits, bit_ptr);	\
	bits -= 16;				\
    }						\
} while (0)

/* remove num valid bits from bit_buf */
#define DUMPBITS(bit_buf,bits,num)	\
do {					\
    bit_buf <<= (num);			\
    bits += (num);			\
} while (0)

/* take num bits from the high part of bit_buf and zero extend them */
#define UBITS(bit_buf,num) (((uint32_t)(bit_buf)) >> (32 - (num)))

/* take num bits from the high part of bit_buf and sign extend them */
#define SBITS(bit_buf,num) (((int32_t)(bit_buf)) >> (32 - (num)))

typedef struct {
    uint8_t modes;
    uint8_t len;
} MBtab;

typedef struct {
    uint8_t delta;
    uint8_t len;
} MVtab;

typedef struct {
    int8_t dmv;
    uint8_t len;
} DMVtab;

typedef struct {
    uint8_t cbp;
    uint8_t len;
} CBPtab;

typedef struct {
    uint8_t size;
    uint8_t len;
} DCtab;

typedef struct {
    uint8_t run;
    uint8_t level;
    uint8_t len;
} DCTtab;

typedef struct {
    uint8_t mba;
    uint8_t len;
} MBAtab;


#define INTRA MACROBLOCK_INTRA
#define QUANT MACROBLOCK_QUANT

static const MBtab MB_I [] = {
    {INTRA|QUANT, 2}, {INTRA, 1}
};

#define MC MACROBLOCK_MOTION_FORWARD
#define CODED MACROBLOCK_PATTERN

static const MBtab MB_P [] = {
    {INTRA|QUANT, 6}, {CODED|QUANT, 5}, {MC|CODED|QUANT, 5}, {INTRA,    5},
    {MC,          3}, {MC,          3}, {MC,             3}, {MC,       3},
    {CODED,       2}, {CODED,       2}, {CODED,          2}, {CODED,    2},
    {CODED,       2}, {CODED,       2}, {CODED,          2}, {CODED,    2},
    {MC|CODED,    1}, {MC|CODED,    1}, {MC|CODED,       1}, {MC|CODED, 1},
    {MC|CODED,    1}, {MC|CODED,    1}, {MC|CODED,       1}, {MC|CODED, 1},
    {MC|CODED,    1}, {MC|CODED,    1}, {MC|CODED,       1}, {MC|CODED, 1},
    {MC|CODED,    1}, {MC|CODED,    1}, {MC|CODED,       1}, {MC|CODED, 1}
};

#define FWD MACROBLOCK_MOTION_FORWARD
#define BWD MACROBLOCK_MOTION_BACKWARD
#define INTER MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD

static const MBtab MB_B [] = {
    {0,                 0}, {INTRA|QUANT,       6},
    {BWD|CODED|QUANT,   6}, {FWD|CODED|QUANT,   6},
    {INTER|CODED|QUANT, 5}, {INTER|CODED|QUANT, 5},
					{INTRA,       5}, {INTRA,       5},
    {FWD,         4}, {FWD,         4}, {FWD,         4}, {FWD,         4},
    {FWD|CODED,   4}, {FWD|CODED,   4}, {FWD|CODED,   4}, {FWD|CODED,   4},
    {BWD,         3}, {BWD,         3}, {BWD,         3}, {BWD,         3},
    {BWD,         3}, {BWD,         3}, {BWD,         3}, {BWD,         3},
    {BWD|CODED,   3}, {BWD|CODED,   3}, {BWD|CODED,   3}, {BWD|CODED,   3},
    {BWD|CODED,   3}, {BWD|CODED,   3}, {BWD|CODED,   3}, {BWD|CODED,   3},
    {INTER,       2}, {INTER,       2}, {INTER,       2}, {INTER,       2},
    {INTER,       2}, {INTER,       2}, {INTER,       2}, {INTER,       2},
    {INTER,       2}, {INTER,       2}, {INTER,       2}, {INTER,       2},
    {INTER,       2}, {INTER,       2}, {INTER,       2}, {INTER,       2},
    {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2},
    {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2},
    {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2},
    {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2}
};

#undef INTRA
#undef QUANT
#undef MC
#undef CODED
#undef FWD
#undef BWD
#undef INTER


static const MVtab MV_4 [] = {
    { 3, 6}, { 2, 4}, { 1, 3}, { 1, 3}, { 0, 2}, { 0, 2}, { 0, 2}, { 0, 2}
};

static const MVtab MV_10 [] = {
    { 0,10}, { 0,10}, { 0,10}, { 0,10}, { 0,10}, { 0,10}, { 0,10}, { 0,10},
    { 0,10}, { 0,10}, { 0,10}, { 0,10}, {15,10}, {14,10}, {13,10}, {12,10},
    {11,10}, {10,10}, { 9, 9}, { 9, 9}, { 8, 9}, { 8, 9}, { 7, 9}, { 7, 9},
    { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7},
    { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7},
    { 4, 7}, { 4, 7}, { 4, 7}, { 4, 7}, { 4, 7}, { 4, 7}, { 4, 7}, { 4, 7}
};


static const DMVtab DMV_2 [] = {
    { 0, 1}, { 0, 1}, { 1, 2}, {-1, 2}
};


static const CBPtab CBP_7 [] = {
    {0x11, 7}, {0x12, 7}, {0x14, 7}, {0x18, 7},
    {0x21, 7}, {0x22, 7}, {0x24, 7}, {0x28, 7},
    {0x3f, 6}, {0x3f, 6}, {0x30, 6}, {0x30, 6},
    {0x09, 6}, {0x09, 6}, {0x06, 6}, {0x06, 6},
    {0x1f, 5}, {0x1f, 5}, {0x1f, 5}, {0x1f, 5},
    {0x10, 5}, {0x10, 5}, {0x10, 5}, {0x10, 5},
    {0x2f, 5}, {0x2f, 5}, {0x2f, 5}, {0x2f, 5},
    {0x20, 5}, {0x20, 5}, {0x20, 5}, {0x20, 5},
    {0x07, 5}, {0x07, 5}, {0x07, 5}, {0x07, 5},
    {0x0b, 5}, {0x0b, 5}, {0x0b, 5}, {0x0b, 5},
    {0x0d, 5}, {0x0d, 5}, {0x0d, 5}, {0x0d, 5},
    {0x0e, 5}, {0x0e, 5}, {0x0e, 5}, {0x0e, 5},
    {0x05, 5}, {0x05, 5}, {0x05, 5}, {0x05, 5},
    {0x0a, 5}, {0x0a, 5}, {0x0a, 5}, {0x0a, 5},
    {0x03, 5}, {0x03, 5}, {0x03, 5}, {0x03, 5},
    {0x0c, 5}, {0x0c, 5}, {0x0c, 5}, {0x0c, 5},
    {0x01, 4}, {0x01, 4}, {0x01, 4}, {0x01, 4},
    {0x01, 4}, {0x01, 4}, {0x01, 4}, {0x01, 4},
    {0x02, 4}, {0x02, 4}, {0x02, 4}, {0x02, 4},
    {0x02, 4}, {0x02, 4}, {0x02, 4}, {0x02, 4},
    {0x04, 4}, {0x04, 4}, {0x04, 4}, {0x04, 4},
    {0x04, 4}, {0x04, 4}, {0x04, 4}, {0x04, 4},
    {0x08, 4}, {0x08, 4}, {0x08, 4}, {0x08, 4},
    {0x08, 4}, {0x08, 4}, {0x08, 4}, {0x08, 4},
    {0x0f, 3}, {0x0f, 3}, {0x0f, 3}, {0x0f, 3},
    {0x0f, 3}, {0x0f, 3}, {0x0f, 3}, {0x0f, 3},
    {0x0f, 3}, {0x0f, 3}, {0x0f, 3}, {0x0f, 3},
    {0x0f, 3}, {0x0f, 3}, {0x0f, 3}, {0x0f, 3}
};

static const CBPtab CBP_9 [] = {
    {0,    0}, {0x00, 9}, {0x39, 9}, {0x36, 9},
    {0x37, 9}, {0x3b, 9}, {0x3d, 9}, {0x3e, 9},
    {0x17, 8}, {0x17, 8}, {0x1b, 8}, {0x1b, 8},
    {0x1d, 8}, {0x1d, 8}, {0x1e, 8}, {0x1e, 8},
    {0x27, 8}, {0x27, 8}, {0x2b, 8}, {0x2b, 8},
    {0x2d, 8}, {0x2d, 8}, {0x2e, 8}, {0x2e, 8},
    {0x19, 8}, {0x19, 8}, {0x16, 8}, {0x16, 8},
    {0x29, 8}, {0x29, 8}, {0x26, 8}, {0x26, 8},
    {0x35, 8}, {0x35, 8}, {0x3a, 8}, {0x3a, 8},
    {0x33, 8}, {0x33, 8}, {0x3c, 8}, {0x3c, 8},
    {0x15, 8}, {0x15, 8}, {0x1a, 8}, {0x1a, 8},
    {0x13, 8}, {0x13, 8}, {0x1c, 8}, {0x1c, 8},
    {0x25, 8}, {0x25, 8}, {0x2a, 8}, {0x2a, 8},
    {0x23, 8}, {0x23, 8}, {0x2c, 8}, {0x2c, 8},
    {0x31, 8}, {0x31, 8}, {0x32, 8}, {0x32, 8},
    {0x34, 8}, {0x34, 8}, {0x38, 8}, {0x38, 8}
};


static const DCtab DC_lum_5 [] = {
    {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
    {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
    {0, 3}, {0, 3}, {0, 3}, {0, 3}, {3, 3}, {3, 3}, {3, 3}, {3, 3},
    {4, 3}, {4, 3}, {4, 3}, {4, 3}, {5, 4}, {5, 4}, {6, 5}
};

static const DCtab DC_chrom_5 [] = {
    {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2},
    {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
    {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
    {3, 3}, {3, 3}, {3, 3}, {3, 3}, {4, 4}, {4, 4}, {5, 5}
};

static const DCtab DC_long [] = {
    {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, { 6, 5}, { 6, 5},
    {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, { 6, 5}, { 6, 5},
    {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, { 7, 6}, { 7, 6},
    {8, 7}, {8, 7}, {8, 7}, {8, 7}, {9, 8}, {9, 8}, {10, 9}, {11, 9}
};


static const DCTtab DCT_16 [] = {
    {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
    {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
    {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
    {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
    {  2,18, 0}, {  2,17, 0}, {  2,16, 0}, {  2,15, 0},
    {  7, 3, 0}, { 17, 2, 0}, { 16, 2, 0}, { 15, 2, 0},
    { 14, 2, 0}, { 13, 2, 0}, { 12, 2, 0}, { 32, 1, 0},
    { 31, 1, 0}, { 30, 1, 0}, { 29, 1, 0}, { 28, 1, 0}
};

static const DCTtab DCT_15 [] = {
    {  1,40,15}, {  1,39,15}, {  1,38,15}, {  1,37,15},
    {  1,36,15}, {  1,35,15}, {  1,34,15}, {  1,33,15},
    {  1,32,15}, {  2,14,15}, {  2,13,15}, {  2,12,15},
    {  2,11,15}, {  2,10,15}, {  2, 9,15}, {  2, 8,15},
    {  1,31,14}, {  1,31,14}, {  1,30,14}, {  1,30,14},
    {  1,29,14}, {  1,29,14}, {  1,28,14}, {  1,28,14},
    {  1,27,14}, {  1,27,14}, {  1,26,14}, {  1,26,14},
    {  1,25,14}, {  1,25,14}, {  1,24,14}, {  1,24,14},
    {  1,23,14}, {  1,23,14}, {  1,22,14}, {  1,22,14},
    {  1,21,14}, {  1,21,14}, {  1,20,14}, {  1,20,14},
    {  1,19,14}, {  1,19,14}, {  1,18,14}, {  1,18,14},
    {  1,17,14}, {  1,17,14}, {  1,16,14}, {  1,16,14}
};

static const DCTtab DCT_13 [] = {
    { 11, 2,13}, { 10, 2,13}, {  6, 3,13}, {  4, 4,13},
    {  3, 5,13}, {  2, 7,13}, {  2, 6,13}, {  1,15,13},
    {  1,14,13}, {  1,13,13}, {  1,12,13}, { 27, 1,13},
    { 26, 1,13}, { 25, 1,13}, { 24, 1,13}, { 23, 1,13},
    {  1,11,12}, {  1,11,12}, {  9, 2,12}, {  9, 2,12},
    {  5, 3,12}, {  5, 3,12}, {  1,10,12}, {  1,10,12},
    {  3, 4,12}, {  3, 4,12}, {  8, 2,12}, {  8, 2,12},
    { 22, 1,12}, { 22, 1,12}, { 21, 1,12}, { 21, 1,12},
    {  1, 9,12}, {  1, 9,12}, { 20, 1,12}, { 20, 1,12},
    { 19, 1,12}, { 19, 1,12}, {  2, 5,12}, {  2, 5,12},
    {  4, 3,12}, {  4, 3,12}, {  1, 8,12}, {  1, 8,12},
    {  7, 2,12}, {  7, 2,12}, { 18, 1,12}, { 18, 1,12}
};

static const DCTtab DCT_B14_10 [] = {
    { 17, 1,10}, {  6, 2,10}, {  1, 7,10}, {  3, 3,10},
    {  2, 4,10}, { 16, 1,10}, { 15, 1,10}, {  5, 2,10}
};

static const DCTtab DCT_B14_8 [] = {
    { 65, 0, 6}, { 65, 0, 6}, { 65, 0, 6}, { 65, 0, 6},
    {  3, 2, 7}, {  3, 2, 7}, { 10, 1, 7}, { 10, 1, 7},
    {  1, 4, 7}, {  1, 4, 7}, {  9, 1, 7}, {  9, 1, 7},
    {  8, 1, 6}, {  8, 1, 6}, {  8, 1, 6}, {  8, 1, 6},
    {  7, 1, 6}, {  7, 1, 6}, {  7, 1, 6}, {  7, 1, 6},
    {  2, 2, 6}, {  2, 2, 6}, {  2, 2, 6}, {  2, 2, 6},
    {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6},
    { 14, 1, 8}, {  1, 6, 8}, { 13, 1, 8}, { 12, 1, 8},
    {  4, 2, 8}, {  2, 3, 8}, {  1, 5, 8}, { 11, 1, 8}
};

static const DCTtab DCT_B14AC_5 [] = {
		 {  1, 3, 5}, {  5, 1, 5}, {  4, 1, 5},
    {  1, 2, 4}, {  1, 2, 4}, {  3, 1, 4}, {  3, 1, 4},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {129, 0, 2}, {129, 0, 2}, {129, 0, 2}, {129, 0, 2},
    {129, 0, 2}, {129, 0, 2}, {129, 0, 2}, {129, 0, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}
};

static const DCTtab DCT_B14DC_5 [] = {
		 {  1, 3, 5}, {  5, 1, 5}, {  4, 1, 5},
    {  1, 2, 4}, {  1, 2, 4}, {  3, 1, 4}, {  3, 1, 4},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1},
    {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1},
    {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1},
    {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}
};

static const DCTtab DCT_B15_10 [] = {
    {  6, 2, 9}, {  6, 2, 9}, { 15, 1, 9}, { 15, 1, 9},
    {  3, 4,10}, { 17, 1,10}, { 16, 1, 9}, { 16, 1, 9}
};

static const DCTtab DCT_B15_8 [] = {
    { 65, 0, 6}, { 65, 0, 6}, { 65, 0, 6}, { 65, 0, 6},
    {  8, 1, 7}, {  8, 1, 7}, {  9, 1, 7}, {  9, 1, 7},
    {  7, 1, 7}, {  7, 1, 7}, {  3, 2, 7}, {  3, 2, 7},
    {  1, 7, 6}, {  1, 7, 6}, {  1, 7, 6}, {  1, 7, 6},
    {  1, 6, 6}, {  1, 6, 6}, {  1, 6, 6}, {  1, 6, 6},
    {  5, 1, 6}, {  5, 1, 6}, {  5, 1, 6}, {  5, 1, 6},
    {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6},
    {  2, 5, 8}, { 12, 1, 8}, {  1,11, 8}, {  1,10, 8},
    { 14, 1, 8}, { 13, 1, 8}, {  4, 2, 8}, {  2, 4, 8},
    {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5},
    {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5},
    {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5},
    {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5},
    {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5},
    {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
    {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
    {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
    {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
    {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
    {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
    {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
    {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5},
    {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5},
    {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5},
    {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5},
    { 10, 1, 7}, { 10, 1, 7}, {  2, 3, 7}, {  2, 3, 7},
    { 11, 1, 7}, { 11, 1, 7}, {  1, 8, 7}, {  1, 8, 7},
    {  1, 9, 7}, {  1, 9, 7}, {  1,12, 8}, {  1,13, 8},
    {  3, 3, 8}, {  5, 2, 8}, {  1,14, 8}, {  1,15, 8}
};


static const MBAtab MBA_5 [] = {
		    {6, 5}, {5, 5}, {4, 4}, {4, 4}, {3, 4}, {3, 4},
    {2, 3}, {2, 3}, {2, 3}, {2, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
    {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
    {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}
};

static const MBAtab MBA_11 [] = {
    {32, 11}, {31, 11}, {30, 11}, {29, 11},
    {28, 11}, {27, 11}, {26, 11}, {25, 11},
    {24, 11}, {23, 11}, {22, 11}, {21, 11},
    {20, 10}, {20, 10}, {19, 10}, {19, 10},
    {18, 10}, {18, 10}, {17, 10}, {17, 10},
    {16, 10}, {16, 10}, {15, 10}, {15, 10},
    {14,  8}, {14,  8}, {14,  8}, {14,  8},
    {14,  8}, {14,  8}, {14,  8}, {14,  8},
    {13,  8}, {13,  8}, {13,  8}, {13,  8},
    {13,  8}, {13,  8}, {13,  8}, {13,  8},
    {12,  8}, {12,  8}, {12,  8}, {12,  8},
    {12,  8}, {12,  8}, {12,  8}, {12,  8},
    {11,  8}, {11,  8}, {11,  8}, {11,  8},
    {11,  8}, {11,  8}, {11,  8}, {11,  8},
    {10,  8}, {10,  8}, {10,  8}, {10,  8},
    {10,  8}, {10,  8}, {10,  8}, {10,  8},
    { 9,  8}, { 9,  8}, { 9,  8}, { 9,  8},
    { 9,  8}, { 9,  8}, { 9,  8}, { 9,  8},
    { 8,  7}, { 8,  7}, { 8,  7}, { 8,  7},
    { 8,  7}, { 8,  7}, { 8,  7}, { 8,  7},
    { 8,  7}, { 8,  7}, { 8,  7}, { 8,  7},
    { 8,  7}, { 8,  7}, { 8,  7}, { 8,  7},
    { 7,  7}, { 7,  7}, { 7,  7}, { 7,  7},
    { 7,  7}, { 7,  7}, { 7,  7}, { 7,  7},
    { 7,  7}, { 7,  7}, { 7,  7}, { 7,  7},
    { 7,  7}, { 7,  7}, { 7,  7}, { 7,  7}
};


/* FILE: mpeg2dec-0.4.0/libmpeg2/slice.c */

mpeg2_mc_t mpeg2_mc;

static inline int get_macroblock_modes (mpeg2_decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    int macroblock_modes;
    const MBtab * tab;

    switch (decoder->coding_type) {
    case I_TYPE:

	tab = MB_I + UBITS (bit_buf, 1);
	DUMPBITS (bit_buf, bits, tab->len);
	macroblock_modes = tab->modes;

	if ((! (decoder->frame_pred_frame_dct)) &&
	    (decoder->picture_structure == FRAME_PICTURE)) {
	    macroblock_modes |= UBITS (bit_buf, 1) * DCT_TYPE_INTERLACED;
	    DUMPBITS (bit_buf, bits, 1);
	}

	return macroblock_modes;

    case P_TYPE:

	tab = MB_P + UBITS (bit_buf, 5);
	DUMPBITS (bit_buf, bits, tab->len);
	macroblock_modes = tab->modes;

	if (decoder->picture_structure != FRAME_PICTURE) {
	    if (macroblock_modes & MACROBLOCK_MOTION_FORWARD) {
		macroblock_modes |= UBITS (bit_buf, 2) << MOTION_TYPE_SHIFT;
		DUMPBITS (bit_buf, bits, 2);
	    }
	    return macroblock_modes | MACROBLOCK_MOTION_FORWARD;
	} else if (decoder->frame_pred_frame_dct) {
	    if (macroblock_modes & MACROBLOCK_MOTION_FORWARD)
		macroblock_modes |= MC_FRAME << MOTION_TYPE_SHIFT;
	    return macroblock_modes | MACROBLOCK_MOTION_FORWARD;
	} else {
	    if (macroblock_modes & MACROBLOCK_MOTION_FORWARD) {
		macroblock_modes |= UBITS (bit_buf, 2) << MOTION_TYPE_SHIFT;
		DUMPBITS (bit_buf, bits, 2);
	    }
	    if (macroblock_modes & (MACROBLOCK_INTRA | MACROBLOCK_PATTERN)) {
		macroblock_modes |= UBITS (bit_buf, 1) * DCT_TYPE_INTERLACED;
		DUMPBITS (bit_buf, bits, 1);
	    }
	    return macroblock_modes | MACROBLOCK_MOTION_FORWARD;
	}

    case B_TYPE:

	tab = MB_B + UBITS (bit_buf, 6);
	DUMPBITS (bit_buf, bits, tab->len);
	macroblock_modes = tab->modes;

	if (decoder->picture_structure != FRAME_PICTURE) {
	    if (! (macroblock_modes & MACROBLOCK_INTRA)) {
		macroblock_modes |= UBITS (bit_buf, 2) << MOTION_TYPE_SHIFT;
		DUMPBITS (bit_buf, bits, 2);
	    }
	    return macroblock_modes;
	} else if (decoder->frame_pred_frame_dct) {
	    /* if (! (macroblock_modes & MACROBLOCK_INTRA)) */
	    macroblock_modes |= MC_FRAME << MOTION_TYPE_SHIFT;
	    return macroblock_modes;
	} else {
	    if (macroblock_modes & MACROBLOCK_INTRA)
		goto intra;
	    macroblock_modes |= UBITS (bit_buf, 2) << MOTION_TYPE_SHIFT;
	    DUMPBITS (bit_buf, bits, 2);
	    if (macroblock_modes & (MACROBLOCK_INTRA | MACROBLOCK_PATTERN)) {
	    intra:
		macroblock_modes |= UBITS (bit_buf, 1) * DCT_TYPE_INTERLACED;
		DUMPBITS (bit_buf, bits, 1);
	    }
	    return macroblock_modes;
	}

    case D_TYPE:

	DUMPBITS (bit_buf, bits, 1);
	return MACROBLOCK_INTRA;

    default:
	return 0;
    }
#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline void get_quantizer_scale (mpeg2_decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    int quantizer_scale_code;

    quantizer_scale_code = UBITS (bit_buf, 5);
    DUMPBITS (bit_buf, bits, 5);

    decoder->quantizer_matrix[0] =
	decoder->quantizer_prescale[0][quantizer_scale_code];
    decoder->quantizer_matrix[1] =
	decoder->quantizer_prescale[1][quantizer_scale_code];
    decoder->quantizer_matrix[2] =
	decoder->chroma_quantizer[0][quantizer_scale_code];
    decoder->quantizer_matrix[3] =
	decoder->chroma_quantizer[1][quantizer_scale_code];
#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline int get_motion_delta (mpeg2_decoder_t * const decoder,
				    const int f_code)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    int delta;
    int sign;
    const MVtab * tab;

    if (bit_buf & 0x80000000) {
	DUMPBITS (bit_buf, bits, 1);
	return 0;
    } else if (bit_buf >= 0x0c000000) {

	tab = MV_4 + UBITS (bit_buf, 4);
	delta = (tab->delta << f_code) + 1;
	bits += tab->len + f_code + 1;
	bit_buf <<= tab->len;

	sign = SBITS (bit_buf, 1);
	bit_buf <<= 1;

	if (f_code)
	    delta += UBITS (bit_buf, f_code);
	bit_buf <<= f_code;

	return (delta ^ sign) - sign;

    } else {

	tab = MV_10 + UBITS (bit_buf, 10);
	delta = (tab->delta << f_code) + 1;
	bits += tab->len + 1;
	bit_buf <<= tab->len;

	sign = SBITS (bit_buf, 1);
	bit_buf <<= 1;

	if (f_code) {
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    delta += UBITS (bit_buf, f_code);
	    DUMPBITS (bit_buf, bits, f_code);
	}

	return (delta ^ sign) - sign;

    }
#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline int bound_motion_vector (const int vector, const int f_code)
{
    return ((int32_t)vector << (27 - f_code)) >> (27 - f_code);
}

static inline int get_dmv (mpeg2_decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    const DMVtab * tab;

    tab = DMV_2 + UBITS (bit_buf, 2);
    DUMPBITS (bit_buf, bits, tab->len);
    return tab->dmv;
#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline int get_coded_block_pattern (mpeg2_decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    const CBPtab * tab;

    NEEDBITS (bit_buf, bits, bit_ptr);

    if (bit_buf >= 0x20000000) {

	tab = CBP_7 + (UBITS (bit_buf, 7) - 16);
	DUMPBITS (bit_buf, bits, tab->len);
	return tab->cbp;

    } else {

	tab = CBP_9 + UBITS (bit_buf, 9);
	DUMPBITS (bit_buf, bits, tab->len);
	return tab->cbp;
    }

#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline int get_luma_dc_dct_diff (mpeg2_decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    const DCtab * tab;
    int size;
    int dc_diff;

    if (bit_buf < 0xf8000000) {
	tab = DC_lum_5 + UBITS (bit_buf, 5);
	size = tab->size;
	if (size) {
	    bits += tab->len + size;
	    bit_buf <<= tab->len;
	    dc_diff =
		UBITS (bit_buf, size) - UBITS (SBITS (~bit_buf, 1), size);
	    bit_buf <<= size;
	    return dc_diff << decoder->intra_dc_precision;
	} else {
	    DUMPBITS (bit_buf, bits, 3);
	    return 0;
	}
    } else {
	tab = DC_long + (UBITS (bit_buf, 9) - 0x1e0);
	size = tab->size;
	DUMPBITS (bit_buf, bits, tab->len);
	NEEDBITS (bit_buf, bits, bit_ptr);
	dc_diff = UBITS (bit_buf, size) - UBITS (SBITS (~bit_buf, 1), size);
	DUMPBITS (bit_buf, bits, size);
	return dc_diff << decoder->intra_dc_precision;
    }
#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline int get_chroma_dc_dct_diff (mpeg2_decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    const DCtab * tab;
    int size;
    int dc_diff;

    if (bit_buf < 0xf8000000) {
	tab = DC_chrom_5 + UBITS (bit_buf, 5);
	size = tab->size;
	if (size) {
	    bits += tab->len + size;
	    bit_buf <<= tab->len;
	    dc_diff =
		UBITS (bit_buf, size) - UBITS (SBITS (~bit_buf, 1), size);
	    bit_buf <<= size;
	    return dc_diff << decoder->intra_dc_precision;
	} else {
	    DUMPBITS (bit_buf, bits, 2);
	    return 0;
	}
    } else {
	tab = DC_long + (UBITS (bit_buf, 10) - 0x3e0);
	size = tab->size;
	DUMPBITS (bit_buf, bits, tab->len + 1);
	NEEDBITS (bit_buf, bits, bit_ptr);
	dc_diff = UBITS (bit_buf, size) - UBITS (SBITS (~bit_buf, 1), size);
	DUMPBITS (bit_buf, bits, size);
	return dc_diff << decoder->intra_dc_precision;
    }
#undef bit_buf
#undef bits
#undef bit_ptr
}

#define SATURATE(val)				\
do {						\
    val <<= 4;					\
    if (unlikely (val != (int16_t) val))	\
	val = (SBITS (val, 1) ^ 2047) << 4;	\
} while (0)

static void get_intra_block_B14 (mpeg2_decoder_t * const decoder,
				 const uint16_t * const quant_matrix)
{
    int i;
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;

    i = 0;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	    i += tab->run;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	    /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    DUMPBITS (bit_buf, bits, 2);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
}

static void get_intra_block_B15 (mpeg2_decoder_t * const decoder,
				 const uint16_t * const quant_matrix)
{
    int i;
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;

    i = 0;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);

    while (1) {
	if (bit_buf >= 0x04000000) {

	    tab = DCT_B15_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64) {

	    normal_code:
		bit_buf <<= tab->len;
		bits += tab->len + 1;
		bit_buf <<= 1;
		NEEDBITS (bit_buf, bits, bit_ptr);

		continue;

	    } else {

		/* end of block. I commented out this code because if we */
		/* dont exit here we will still exit at the later test :) */

		/* if (i >= 128) break;	*/	/* end of block */

		/* escape code */

		i += UBITS (bit_buf << 6, 6) - 64;
		if (i >= 64)
		    break;	/* illegal, check against buffer overflow */

		DUMPBITS (bit_buf, bits, 12);
		NEEDBITS (bit_buf, bits, bit_ptr);
		DUMPBITS (bit_buf, bits, 12);
		NEEDBITS (bit_buf, bits, bit_ptr);

		continue;

	    }
	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B15_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    DUMPBITS (bit_buf, bits, 4);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
}

static int get_non_intra_block (mpeg2_decoder_t * const decoder,
				const uint16_t * const quant_matrix)
{
    char first = 1;
    int i;
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;

    i = -1;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);
    if (bit_buf >= 0x28000000) {
	tab = DCT_B14DC_5 + (UBITS (bit_buf, 5) - 5);
	goto entry_1;
    } else
	goto entry_2;

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	entry_1:
	    i += tab->run;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;

	    if (first) {
	      /* we need to decode the first value properly
	       * to detect the IDCT shortcut */
	      int val;
	      val = ((2 * tab->level + 1) * quant_matrix[0]) >> 5;
	      /* if (bitstream_get (1)) val = -val; */
	      val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);
	      SATURATE (val);
	      decoder->DCTblock[0] = val;
	      
	      first = 0;
	    }

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	}

    entry_2:
	if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	    /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    if (first) {
	      /* we need to decode the first value properly
	       * to detect the IDCT shortcut */
	      int val;
	      val = 2 * (SBITS (bit_buf, 12) + SBITS (bit_buf, 1)) + 1;
	      val = (val * quant_matrix[0]) / 32;
	      SATURATE (val);
	      decoder->DCTblock[0] = val;
	      
	      first = 0;
	    }

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    DUMPBITS (bit_buf, bits, 2);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
    return i;
}

static void get_mpeg1_intra_block (mpeg2_decoder_t * const decoder)
{
    int i;
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;

    i = 0;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	    i += tab->run;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	    /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    if (! (SBITS (bit_buf, 8) & 0x7f)) {
		DUMPBITS (bit_buf, bits, 8);
	    }

	    DUMPBITS (bit_buf, bits, 8);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    DUMPBITS (bit_buf, bits, 2);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
}

static int get_mpeg1_non_intra_block (mpeg2_decoder_t * const decoder)
{
    char first = 1;
    int i;
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;

    i = -1;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);
    if (bit_buf >= 0x28000000) {
	tab = DCT_B14DC_5 + (UBITS (bit_buf, 5) - 5);
	goto entry_1;
    } else
	goto entry_2;

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	entry_1:
	    i += tab->run;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;

	    if (first) {
	      /* we need to decode the first value properly
	       * to detect the IDCT shortcut */
	      int val;
	      val = ((2 * tab->level + 1) * decoder->quantizer_matrix[1][0]) >> 5;
	      /* oddification */
	      val = (val - 1) | 1;
	      /* if (bitstream_get (1)) val = -val; */
	      val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);
	      SATURATE (val);
	      decoder->DCTblock[0] = val;
	      
	      first = 0;
	    }

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	}

    entry_2:
	if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	    /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    if (first) {
	      /* we need to decode the first value properly
	       * to detect the IDCT shortcut */
	      int val;
	      val = SBITS (bit_buf, 8);
	      if (! (val & 0x7f)) {
		DUMPBITS (bit_buf, bits, 8);
		val = UBITS (bit_buf, 8) + 2 * val;
	      }
	      val = 2 * (val + SBITS (val, 1)) + 1;
	      val = (val * decoder->quantizer_matrix[1][0]) / 32;
	      /* oddification */
	      val = (val + ~SBITS (val, 1)) | 1;
	      SATURATE (val);
	      decoder->DCTblock[0] = val;
	      
	      first = 0;
	    } else {
	    if (! (SBITS (bit_buf, 8) & 0x7f)) {
		DUMPBITS (bit_buf, bits, 8);
	    }
	    }
	    DUMPBITS (bit_buf, bits, 8);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    DUMPBITS (bit_buf, bits, 2);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
    return i;
}

static inline void slice_intra_DCT (mpeg2_decoder_t * const decoder,
				    const int cc,
				    uint8_t * const dest, const int stride)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    decoder->macro_intra++;

    NEEDBITS (bit_buf, bits, bit_ptr);
    /* Get the intra DC coefficient and inverse quantize it */
    if (cc == 0)
	decoder->DCTblock[0] =
	    decoder->dc_dct_pred[0] += get_luma_dc_dct_diff (decoder);
    else
	decoder->DCTblock[0] =
	    decoder->dc_dct_pred[cc] += get_chroma_dc_dct_diff (decoder);

    if (decoder->mpeg1) {
	if (decoder->coding_type != D_TYPE)
	    get_mpeg1_intra_block (decoder);
    } else if (decoder->intra_vlc_format)
	get_intra_block_B15 (decoder, decoder->quantizer_matrix[cc ? 2 : 0]);
    else
	get_intra_block_B14 (decoder, decoder->quantizer_matrix[cc ? 2 : 0]);
#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline void slice_non_intra_DCT (mpeg2_decoder_t * const decoder,
					const int cc,
					uint8_t * const dest, const int stride)
{
    int last;

    if (decoder->mpeg1)
	last = get_mpeg1_non_intra_block (decoder);
    else
	last = get_non_intra_block (decoder,
				    decoder->quantizer_matrix[cc ? 3 : 1]);

    if (last != 129 || (decoder->DCTblock[0] & (7 << 4)) == (4 << 4))
	decoder->macro_inter++;
    else
	decoder->macro_inter_short++;
}

#define HPEL(X,Y) (hpel_cost[((X)&1)|(((Y)&1)<<1)])
static const int hpel_cost[] = {
  1, 2, 2, 4
};

#define MOTION_420(table,ref,motion_x,motion_y,size,y)			      \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = 2 * decoder->v_offset + motion_y + 2 * y;			      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y_ ## size)) {			      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y_ ## size;	      \
	motion_y = pos_y - 2 * decoder->v_offset - 2 * y;		      \
    }									      \
    decoder->motion += HPEL(pos_y,pos_x)*16*size;			      \
    motion_x /= 2;	motion_y /= 2;					      \
    decoder->motion += 2*(HPEL(motion_y,motion_x)*8*size/2);

#define MOTION_FIELD_420(table,ref,motion_x,motion_y,dest_field,op,src_field) \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = decoder->v_offset + motion_y;				      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y)) {				      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		      \
	motion_y = pos_y - decoder->v_offset;				      \
    }									      \
    decoder->motion += HPEL(pos_y,pos_x)*16*8;				      \
    motion_x /= 2;	motion_y /= 2;					      \
    decoder->motion += 2*(HPEL(motion_y,motion_x)*8*4);

#define MOTION_DMV_420(table,ref,motion_x,motion_y)			      \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = decoder->v_offset + motion_y;				      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y)) {				      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		      \
	motion_y = pos_y - decoder->v_offset;				      \
    }									      \
    decoder->motion += 2*(HPEL(pos_y,pos_x)*16*8);			      \
    motion_x /= 2;	motion_y /= 2;					      \
    decoder->motion += 4*(HPEL(motion_y,motion_x)*8*4);

#define MOTION_ZERO_420(table,ref)					      \
    decoder->motion += 16*16;						      \
    decoder->motion += 2*(8*8);

#define MOTION_422(table,ref,motion_x,motion_y,size,y)			      \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = 2 * decoder->v_offset + motion_y + 2 * y;			      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y_ ## size)) {			      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y_ ## size;	      \
	motion_y = pos_y - 2 * decoder->v_offset - 2 * y;		      \
    }									      \
    decoder->motion += HPEL(pos_y,pos_x)*16*size;			      \
    motion_x /= 2;							      \
    decoder->motion += 2*(HPEL(pos_y,motion_x)*8*size);

#define MOTION_FIELD_422(table,ref,motion_x,motion_y,dest_field,op,src_field) \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = decoder->v_offset + motion_y;				      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y)) {				      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		      \
	motion_y = pos_y - decoder->v_offset;				      \
    }									      \
    decoder->motion += HPEL(pos_y,pos_x)*16*8;				      \
    motion_x /= 2;							      \
    decoder->motion += 2*(HPEL(pos_y,motion_x)*8*8);

#define MOTION_DMV_422(table,ref,motion_x,motion_y)			      \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = decoder->v_offset + motion_y;				      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y)) {				      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		      \
	motion_y = pos_y - decoder->v_offset;				      \
    }									      \
    decoder->motion += 2*(HPEL(pos_y,pos_x)*16*8);			      \
    motion_x /= 2;							      \
    decoder->motion += 4*(HPEL(pos_y,motion_x)*8*8);

#define MOTION_ZERO_422(table,ref)					      \
    decoder->motion += 16*16;						      \
    decoder->motion += 2*(8*16);

#define MOTION_444(table,ref,motion_x,motion_y,size,y)			      \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = 2 * decoder->v_offset + motion_y + 2 * y;			      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y_ ## size)) {			      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y_ ## size;	      \
	motion_y = pos_y - 2 * decoder->v_offset - 2 * y;		      \
    }									      \
    decoder->motion += 3*(HPEL(pos_y,pos_x)*16*size);

#define MOTION_FIELD_444(table,ref,motion_x,motion_y,dest_field,op,src_field) \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = decoder->v_offset + motion_y;				      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y)) {				      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		      \
	motion_y = pos_y - decoder->v_offset;				      \
    }									      \
    decoder->motion += 3*(HPEL(pos_y,pos_x)*16*8);

#define MOTION_DMV_444(table,ref,motion_x,motion_y)			      \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = decoder->v_offset + motion_y;				      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y)) {				      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		      \
	motion_y = pos_y - decoder->v_offset;				      \
    }									      \
    decoder->motion += 6*(HPEL(pos_y,pos_x)*16*8);

#define MOTION_ZERO_444(table,ref)					      \
    decoder->motion += 3*(16*16);

#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

static void motion_mp1 (mpeg2_decoder_t * const decoder,
			motion_t * const motion,
			mpeg2_mc_fct * const * const table)
{
    int motion_x, motion_y;
    unsigned int pos_x, pos_y;

    NEEDBITS (bit_buf, bits, bit_ptr);
    motion_x = (motion->pmv[0][0] +
		(get_motion_delta (decoder,
				   motion->f_code[0]) << motion->f_code[1]));
    motion_x = bound_motion_vector (motion_x,
				    motion->f_code[0] + motion->f_code[1]);
    motion->pmv[0][0] = motion_x;

    NEEDBITS (bit_buf, bits, bit_ptr);
    motion_y = (motion->pmv[0][1] +
		(get_motion_delta (decoder,
				   motion->f_code[0]) << motion->f_code[1]));
    motion_y = bound_motion_vector (motion_y,
				    motion->f_code[0] + motion->f_code[1]);
    motion->pmv[0][1] = motion_y;

    MOTION_420 (table, motion->ref[0], motion_x, motion_y, 16, 0);
}

#define MOTION_FUNCTIONS(FORMAT,MOTION,MOTION_FIELD,MOTION_DMV,MOTION_ZERO)   \
									      \
static void motion_fr_frame_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				      motion_t * const motion,		      \
				      mpeg2_mc_fct * const * const table)     \
{									      \
    int motion_x, motion_y;						      \
    unsigned int pos_x, pos_y;						      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_x = motion->pmv[0][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[1][0] = motion->pmv[0][0] = motion_x;			      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_y = motion->pmv[0][1] + get_motion_delta (decoder,		      \
						     motion->f_code[1]);      \
    motion_y = bound_motion_vector (motion_y, motion->f_code[1]);	      \
    motion->pmv[1][1] = motion->pmv[0][1] = motion_y;			      \
									      \
    MOTION (table, motion->ref[0], motion_x, motion_y, 16, 0);		      \
}									      \
									      \
static void motion_fr_field_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				      motion_t * const motion,		      \
				      mpeg2_mc_fct * const * const table)     \
{									      \
    int motion_x, motion_y, field;					      \
    unsigned int pos_x, pos_y;						      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    field = UBITS (bit_buf, 1);						      \
    DUMPBITS (bit_buf, bits, 1);					      \
									      \
    motion_x = motion->pmv[0][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[0][0] = motion_x;					      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_y = ((motion->pmv[0][1] >> 1) +				      \
		get_motion_delta (decoder, motion->f_code[1]));		      \
    /* motion_y = bound_motion_vector (motion_y, motion->f_code[1]); */	      \
    motion->pmv[0][1] = motion_y << 1;					      \
									      \
    MOTION_FIELD (table, motion->ref[0], motion_x, motion_y, 0, & ~1, field); \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    field = UBITS (bit_buf, 1);						      \
    DUMPBITS (bit_buf, bits, 1);					      \
									      \
    motion_x = motion->pmv[1][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[1][0] = motion_x;					      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_y = ((motion->pmv[1][1] >> 1) +				      \
		get_motion_delta (decoder, motion->f_code[1]));		      \
    /* motion_y = bound_motion_vector (motion_y, motion->f_code[1]); */	      \
    motion->pmv[1][1] = motion_y << 1;					      \
									      \
    MOTION_FIELD (table, motion->ref[0], motion_x, motion_y, 1, & ~1, field); \
}									      \
									      \
static void motion_fr_dmv_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				    motion_t * const motion,		      \
				    mpeg2_mc_fct * const * const table)	      \
{									      \
    int motion_x, motion_y, dmv_x, dmv_y, m, other_x, other_y;		      \
    unsigned int pos_x, pos_y;						      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_x = motion->pmv[0][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[1][0] = motion->pmv[0][0] = motion_x;			      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    dmv_x = get_dmv (decoder);						      \
									      \
    motion_y = ((motion->pmv[0][1] >> 1) +				      \
		get_motion_delta (decoder, motion->f_code[1]));		      \
    /* motion_y = bound_motion_vector (motion_y, motion->f_code[1]); */	      \
    motion->pmv[1][1] = motion->pmv[0][1] = motion_y << 1;		      \
    dmv_y = get_dmv (decoder);						      \
									      \
    m = decoder->top_field_first ? 1 : 3;				      \
    other_x = ((motion_x * m + (motion_x > 0)) >> 1) + dmv_x;		      \
    other_y = ((motion_y * m + (motion_y > 0)) >> 1) + dmv_y - 1;	      \
    MOTION_FIELD (mpeg2_mc.put, motion->ref[0], other_x, other_y, 0, | 1, 0); \
									      \
    m = decoder->top_field_first ? 3 : 1;				      \
    other_x = ((motion_x * m + (motion_x > 0)) >> 1) + dmv_x;		      \
    other_y = ((motion_y * m + (motion_y > 0)) >> 1) + dmv_y + 1;	      \
    MOTION_FIELD (mpeg2_mc.put, motion->ref[0], other_x, other_y, 1, & ~1, 0);\
									      \
    MOTION_DMV (mpeg2_mc.avg, motion->ref[0], motion_x, motion_y);	      \
}									      \
									      \
static void motion_reuse_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				   motion_t * const motion,		      \
				   mpeg2_mc_fct * const * const table)	      \
{									      \
    int motion_x, motion_y;						      \
    unsigned int pos_x, pos_y;						      \
									      \
    motion_x = motion->pmv[0][0];					      \
    motion_y = motion->pmv[0][1];					      \
									      \
    MOTION (table, motion->ref[0], motion_x, motion_y, 16, 0);		      \
}									      \
									      \
static void motion_zero_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				  motion_t * const motion,		      \
				  mpeg2_mc_fct * const * const table)	      \
{									      \
    motion->pmv[0][0] = motion->pmv[0][1] = 0;				      \
    motion->pmv[1][0] = motion->pmv[1][1] = 0;				      \
									      \
    MOTION_ZERO (table, motion->ref[0]);				      \
}									      \
									      \
static void motion_fi_field_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				      motion_t * const motion,		      \
				      mpeg2_mc_fct * const * const table)     \
{									      \
    int motion_x, motion_y;						      \
    uint8_t ** ref_field;						      \
    unsigned int pos_x, pos_y;						      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    ref_field = motion->ref2[UBITS (bit_buf, 1)];			      \
    DUMPBITS (bit_buf, bits, 1);					      \
									      \
    motion_x = motion->pmv[0][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[1][0] = motion->pmv[0][0] = motion_x;			      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_y = motion->pmv[0][1] + get_motion_delta (decoder,		      \
						     motion->f_code[1]);      \
    motion_y = bound_motion_vector (motion_y, motion->f_code[1]);	      \
    motion->pmv[1][1] = motion->pmv[0][1] = motion_y;			      \
									      \
    MOTION (table, ref_field, motion_x, motion_y, 16, 0);		      \
}									      \
									      \
static void motion_fi_16x8_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				     motion_t * const motion,		      \
				     mpeg2_mc_fct * const * const table)      \
{									      \
    int motion_x, motion_y;						      \
    uint8_t ** ref_field;						      \
    unsigned int pos_x, pos_y;						      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    ref_field = motion->ref2[UBITS (bit_buf, 1)];			      \
    DUMPBITS (bit_buf, bits, 1);					      \
									      \
    motion_x = motion->pmv[0][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[0][0] = motion_x;					      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_y = motion->pmv[0][1] + get_motion_delta (decoder,		      \
						     motion->f_code[1]);      \
    motion_y = bound_motion_vector (motion_y, motion->f_code[1]);	      \
    motion->pmv[0][1] = motion_y;					      \
									      \
    MOTION (table, ref_field, motion_x, motion_y, 8, 0);		      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    ref_field = motion->ref2[UBITS (bit_buf, 1)];			      \
    DUMPBITS (bit_buf, bits, 1);					      \
									      \
    motion_x = motion->pmv[1][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[1][0] = motion_x;					      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_y = motion->pmv[1][1] + get_motion_delta (decoder,		      \
						     motion->f_code[1]);      \
    motion_y = bound_motion_vector (motion_y, motion->f_code[1]);	      \
    motion->pmv[1][1] = motion_y;					      \
									      \
    MOTION (table, ref_field, motion_x, motion_y, 8, 8);		      \
}									      \
									      \
static void motion_fi_dmv_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				    motion_t * const motion,		      \
				    mpeg2_mc_fct * const * const table)	      \
{									      \
    int motion_x, motion_y, other_x, other_y;				      \
    unsigned int pos_x, pos_y;						      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_x = motion->pmv[0][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[1][0] = motion->pmv[0][0] = motion_x;			      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    other_x = ((motion_x + (motion_x > 0)) >> 1) + get_dmv (decoder);	      \
									      \
    motion_y = motion->pmv[0][1] + get_motion_delta (decoder,		      \
						     motion->f_code[1]);      \
    motion_y = bound_motion_vector (motion_y, motion->f_code[1]);	      \
    motion->pmv[1][1] = motion->pmv[0][1] = motion_y;			      \
    other_y = (((motion_y + (motion_y > 0)) >> 1) + get_dmv (decoder) +	      \
	       decoder->dmv_offset);					      \
									      \
    MOTION (mpeg2_mc.put, motion->ref[0], motion_x, motion_y, 16, 0);	      \
    MOTION (mpeg2_mc.avg, motion->ref[1], other_x, other_y, 16, 0);	      \
}									      \

MOTION_FUNCTIONS (420, MOTION_420, MOTION_FIELD_420, MOTION_DMV_420,
		  MOTION_ZERO_420)
MOTION_FUNCTIONS (422, MOTION_422, MOTION_FIELD_422, MOTION_DMV_422,
		  MOTION_ZERO_422)
MOTION_FUNCTIONS (444, MOTION_444, MOTION_FIELD_444, MOTION_DMV_444,
		  MOTION_ZERO_444)

/* like motion_frame, but parsing without actual motion compensation */
static void motion_fr_conceal (mpeg2_decoder_t * const decoder)
{
    int tmp;

    NEEDBITS (bit_buf, bits, bit_ptr);
    tmp = (decoder->f_motion.pmv[0][0] +
	   get_motion_delta (decoder, decoder->f_motion.f_code[0]));
    tmp = bound_motion_vector (tmp, decoder->f_motion.f_code[0]);
    decoder->f_motion.pmv[1][0] = decoder->f_motion.pmv[0][0] = tmp;

    NEEDBITS (bit_buf, bits, bit_ptr);
    tmp = (decoder->f_motion.pmv[0][1] +
	   get_motion_delta (decoder, decoder->f_motion.f_code[1]));
    tmp = bound_motion_vector (tmp, decoder->f_motion.f_code[1]);
    decoder->f_motion.pmv[1][1] = decoder->f_motion.pmv[0][1] = tmp;

    DUMPBITS (bit_buf, bits, 1); /* remove marker_bit */
}

static void motion_fi_conceal (mpeg2_decoder_t * const decoder)
{
    int tmp;

    NEEDBITS (bit_buf, bits, bit_ptr);
    DUMPBITS (bit_buf, bits, 1); /* remove field_select */

    tmp = (decoder->f_motion.pmv[0][0] +
	   get_motion_delta (decoder, decoder->f_motion.f_code[0]));
    tmp = bound_motion_vector (tmp, decoder->f_motion.f_code[0]);
    decoder->f_motion.pmv[1][0] = decoder->f_motion.pmv[0][0] = tmp;

    NEEDBITS (bit_buf, bits, bit_ptr);
    tmp = (decoder->f_motion.pmv[0][1] +
	   get_motion_delta (decoder, decoder->f_motion.f_code[1]));
    tmp = bound_motion_vector (tmp, decoder->f_motion.f_code[1]);
    decoder->f_motion.pmv[1][1] = decoder->f_motion.pmv[0][1] = tmp;

    DUMPBITS (bit_buf, bits, 1); /* remove marker_bit */
}

#undef bit_buf
#undef bits
#undef bit_ptr

#define MOTION_CALL(routine,direction)				\
do {								\
    if ((direction) & MACROBLOCK_MOTION_FORWARD)		\
	routine (decoder, &(decoder->f_motion), mpeg2_mc.put);	\
    if ((direction) & MACROBLOCK_MOTION_BACKWARD)		\
	routine (decoder, &(decoder->b_motion),			\
		 ((direction) & MACROBLOCK_MOTION_FORWARD ?	\
		  mpeg2_mc.avg : mpeg2_mc.put));		\
} while (0)

#define NEXT_MACROBLOCK							\
do {									\
    decoder->offset += 16;						\
    if (decoder->offset == decoder->width) {				\
	do { /* just so we can use the break statement */		\
	    if (decoder->convert) {					\
		decoder->convert (decoder->convert_id, decoder->dest,	\
				  decoder->v_offset);			\
		if (decoder->coding_type == B_TYPE)			\
		    break;						\
	    }								\
	    decoder->dest[0] += decoder->slice_stride;			\
	    decoder->dest[1] += decoder->slice_uv_stride;		\
	    decoder->dest[2] += decoder->slice_uv_stride;		\
	} while (0);							\
	decoder->v_offset += 16;					\
	if (decoder->v_offset > decoder->limit_y) {			\
	    return;							\
	}								\
	decoder->offset = 0;						\
    }									\
} while (0)

static
void mpeg2_init_fbuf (mpeg2_decoder_t * decoder, uint8_t * current_fbuf[3],
		      uint8_t * forward_fbuf[3], uint8_t * backward_fbuf[3])
{
    int offset, stride, height, bottom_field;

    stride = decoder->stride_frame;
    bottom_field = (decoder->picture_structure == BOTTOM_FIELD);
    offset = bottom_field ? stride : 0;
    height = decoder->height;

    decoder->picture_dest[0] = current_fbuf[0] + offset;
    decoder->picture_dest[1] = current_fbuf[1] + (offset >> 1);
    decoder->picture_dest[2] = current_fbuf[2] + (offset >> 1);

    decoder->f_motion.ref[0][0] = forward_fbuf[0] + offset;
    decoder->f_motion.ref[0][1] = forward_fbuf[1] + (offset >> 1);
    decoder->f_motion.ref[0][2] = forward_fbuf[2] + (offset >> 1);

    decoder->b_motion.ref[0][0] = backward_fbuf[0] + offset;
    decoder->b_motion.ref[0][1] = backward_fbuf[1] + (offset >> 1);
    decoder->b_motion.ref[0][2] = backward_fbuf[2] + (offset >> 1);

    if (decoder->picture_structure != FRAME_PICTURE) {
	decoder->dmv_offset = bottom_field ? 1 : -1;
	decoder->f_motion.ref2[0] = decoder->f_motion.ref[bottom_field];
	decoder->f_motion.ref2[1] = decoder->f_motion.ref[!bottom_field];
	decoder->b_motion.ref2[0] = decoder->b_motion.ref[bottom_field];
	decoder->b_motion.ref2[1] = decoder->b_motion.ref[!bottom_field];
	offset = stride - offset;

	if (decoder->second_field && (decoder->coding_type != B_TYPE))
	    forward_fbuf = current_fbuf;

	decoder->f_motion.ref[1][0] = forward_fbuf[0] + offset;
	decoder->f_motion.ref[1][1] = forward_fbuf[1] + (offset >> 1);
	decoder->f_motion.ref[1][2] = forward_fbuf[2] + (offset >> 1);

	decoder->b_motion.ref[1][0] = backward_fbuf[0] + offset;
	decoder->b_motion.ref[1][1] = backward_fbuf[1] + (offset >> 1);
	decoder->b_motion.ref[1][2] = backward_fbuf[2] + (offset >> 1);

	stride <<= 1;
	height >>= 1;
    }

    decoder->stride = stride;
    decoder->uv_stride = stride >> 1;
    decoder->slice_stride = 16 * stride;
    decoder->slice_uv_stride =
	decoder->slice_stride >> (2 - decoder->chroma_format);
    decoder->limit_x = 2 * decoder->width - 32;
    decoder->limit_y_16 = 2 * height - 32;
    decoder->limit_y_8 = 2 * height - 16;
    decoder->limit_y = height - 16;

    if (decoder->mpeg1) {
	decoder->motion_parser[0] = motion_zero_420;
	decoder->motion_parser[MC_FRAME] = motion_mp1;
	decoder->motion_parser[4] = motion_reuse_420;
    } else if (decoder->picture_structure == FRAME_PICTURE) {
	if (decoder->chroma_format == 0) {
	    decoder->motion_parser[0] = motion_zero_420;
	    decoder->motion_parser[MC_FIELD] = motion_fr_field_420;
	    decoder->motion_parser[MC_FRAME] = motion_fr_frame_420;
	    decoder->motion_parser[MC_DMV] = motion_fr_dmv_420;
	    decoder->motion_parser[4] = motion_reuse_420;
	} else if (decoder->chroma_format == 1) {
	    decoder->motion_parser[0] = motion_zero_422;
	    decoder->motion_parser[MC_FIELD] = motion_fr_field_422;
	    decoder->motion_parser[MC_FRAME] = motion_fr_frame_422;
	    decoder->motion_parser[MC_DMV] = motion_fr_dmv_422;
	    decoder->motion_parser[4] = motion_reuse_422;
	} else {
	    decoder->motion_parser[0] = motion_zero_444;
	    decoder->motion_parser[MC_FIELD] = motion_fr_field_444;
	    decoder->motion_parser[MC_FRAME] = motion_fr_frame_444;
	    decoder->motion_parser[MC_DMV] = motion_fr_dmv_444;
	    decoder->motion_parser[4] = motion_reuse_444;
	}
    } else {
	if (decoder->chroma_format == 0) {
	    decoder->motion_parser[0] = motion_zero_420;
	    decoder->motion_parser[MC_FIELD] = motion_fi_field_420;
	    decoder->motion_parser[MC_16X8] = motion_fi_16x8_420;
	    decoder->motion_parser[MC_DMV] = motion_fi_dmv_420;
	    decoder->motion_parser[4] = motion_reuse_420;
	} else if (decoder->chroma_format == 1) {
	    decoder->motion_parser[0] = motion_zero_422;
	    decoder->motion_parser[MC_FIELD] = motion_fi_field_422;
	    decoder->motion_parser[MC_16X8] = motion_fi_16x8_422;
	    decoder->motion_parser[MC_DMV] = motion_fi_dmv_422;
	    decoder->motion_parser[4] = motion_reuse_422;
	} else {
	    decoder->motion_parser[0] = motion_zero_444;
	    decoder->motion_parser[MC_FIELD] = motion_fi_field_444;
	    decoder->motion_parser[MC_16X8] = motion_fi_16x8_444;
	    decoder->motion_parser[MC_DMV] = motion_fi_dmv_444;
	    decoder->motion_parser[4] = motion_reuse_444;
	}
    }
}

static inline int slice_init (mpeg2_decoder_t * const decoder, int code)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    int offset;
    const MBAtab * mba;

    decoder->dc_dct_pred[0] = decoder->dc_dct_pred[1] =
	decoder->dc_dct_pred[2] = 16384;

    decoder->f_motion.pmv[0][0] = decoder->f_motion.pmv[0][1] = 0;
    decoder->f_motion.pmv[1][0] = decoder->f_motion.pmv[1][1] = 0;
    decoder->b_motion.pmv[0][0] = decoder->b_motion.pmv[0][1] = 0;
    decoder->b_motion.pmv[1][0] = decoder->b_motion.pmv[1][1] = 0;

    if (decoder->vertical_position_extension) {
	code += UBITS (bit_buf, 3) << 7;
	DUMPBITS (bit_buf, bits, 3);
    }
    decoder->v_offset = (code - 1) * 16;
    offset = 0;
    if (!(decoder->convert) || decoder->coding_type != B_TYPE)
	offset = (code - 1) * decoder->slice_stride;

    decoder->dest[0] = decoder->picture_dest[0] + offset;
    offset >>= (2 - decoder->chroma_format);
    decoder->dest[1] = decoder->picture_dest[1] + offset;
    decoder->dest[2] = decoder->picture_dest[2] + offset;

    get_quantizer_scale (decoder);

    /* ignore intra_slice and all the extra data */
    while (bit_buf & 0x80000000) {
	DUMPBITS (bit_buf, bits, 9);
	NEEDBITS (bit_buf, bits, bit_ptr);
    }

    /* decode initial macroblock address increment */
    offset = 0;
    while (1) {
	if (bit_buf >= 0x08000000) {
	    mba = MBA_5 + (UBITS (bit_buf, 6) - 2);
	    break;
	} else if (bit_buf >= 0x01800000) {
	    mba = MBA_11 + (UBITS (bit_buf, 12) - 24);
	    break;
	} else switch (UBITS (bit_buf, 12)) {
	case 8:		/* macroblock_escape */
	    offset += 33;
	    DUMPBITS (bit_buf, bits, 11);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    continue;
	case 15:	/* macroblock_stuffing (MPEG1 only) */
	    bit_buf &= 0xfffff;
	    DUMPBITS (bit_buf, bits, 11);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    continue;
	default:	/* error */
	    return 1;
	}
    }
    DUMPBITS (bit_buf, bits, mba->len + 1);
    decoder->offset = (offset + mba->mba) << 4;

    while (decoder->offset - decoder->width >= 0) {
	decoder->offset -= decoder->width;
	if (!(decoder->convert) || decoder->coding_type != B_TYPE) {
	    decoder->dest[0] += decoder->slice_stride;
	    decoder->dest[1] += decoder->slice_uv_stride;
	    decoder->dest[2] += decoder->slice_uv_stride;
	}
	decoder->v_offset += 16;
    }
    if (decoder->v_offset > decoder->limit_y)
	return 1;

    return 0;
#undef bit_buf
#undef bits
#undef bit_ptr
}

static
void mpeg2_slice (mpeg2_decoder_t * const decoder, const int code,
		  const uint8_t * const buffer)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    bitstream_init (decoder, buffer);

    if (slice_init (decoder, code))
	return;

    while (1) {
	int macroblock_modes;
	int mba_inc;
	const MBAtab * mba;

	NEEDBITS (bit_buf, bits, bit_ptr);

	macroblock_modes = get_macroblock_modes (decoder);

	/* maybe integrate MACROBLOCK_QUANT test into get_macroblock_modes ? */
	if (macroblock_modes & MACROBLOCK_QUANT)
	    get_quantizer_scale (decoder);

	if (macroblock_modes & MACROBLOCK_INTRA) {

	    int DCT_offset, DCT_stride;
	    int offset;
	    uint8_t * dest_y;

	    if (decoder->concealment_motion_vectors) {
		if (decoder->picture_structure == FRAME_PICTURE)
		    motion_fr_conceal (decoder);
		else
		    motion_fi_conceal (decoder);
	    } else {
		decoder->f_motion.pmv[0][0] = decoder->f_motion.pmv[0][1] = 0;
		decoder->f_motion.pmv[1][0] = decoder->f_motion.pmv[1][1] = 0;
		decoder->b_motion.pmv[0][0] = decoder->b_motion.pmv[0][1] = 0;
		decoder->b_motion.pmv[1][0] = decoder->b_motion.pmv[1][1] = 0;
	    }

	    if (macroblock_modes & DCT_TYPE_INTERLACED) {
		DCT_offset = decoder->stride;
		DCT_stride = decoder->stride * 2;
	    } else {
		DCT_offset = decoder->stride * 8;
		DCT_stride = decoder->stride;
	    }

	    offset = decoder->offset;
	    dest_y = decoder->dest[0] + offset;
	    slice_intra_DCT (decoder, 0, dest_y, DCT_stride);
	    slice_intra_DCT (decoder, 0, dest_y + 8, DCT_stride);
	    slice_intra_DCT (decoder, 0, dest_y + DCT_offset, DCT_stride);
	    slice_intra_DCT (decoder, 0, dest_y + DCT_offset + 8, DCT_stride);
	    if (likely (decoder->chroma_format == 0)) {
		slice_intra_DCT (decoder, 1, decoder->dest[1] + (offset >> 1),
				 decoder->uv_stride);
		slice_intra_DCT (decoder, 2, decoder->dest[2] + (offset >> 1),
				 decoder->uv_stride);
		if (decoder->coding_type == D_TYPE) {
		    NEEDBITS (bit_buf, bits, bit_ptr);
		    DUMPBITS (bit_buf, bits, 1);
		}
	    } else if (likely (decoder->chroma_format == 1)) {
		uint8_t * dest_u = decoder->dest[1] + (offset >> 1);
		uint8_t * dest_v = decoder->dest[2] + (offset >> 1);
		DCT_stride >>= 1;
		DCT_offset >>= 1;
		slice_intra_DCT (decoder, 1, dest_u, DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v, DCT_stride);
		slice_intra_DCT (decoder, 1, dest_u + DCT_offset, DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v + DCT_offset, DCT_stride);
	    } else {
		uint8_t * dest_u = decoder->dest[1] + offset;
		uint8_t * dest_v = decoder->dest[2] + offset;
		slice_intra_DCT (decoder, 1, dest_u, DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v, DCT_stride);
		slice_intra_DCT (decoder, 1, dest_u + DCT_offset, DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v + DCT_offset, DCT_stride);
		slice_intra_DCT (decoder, 1, dest_u + 8, DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v + 8, DCT_stride);
		slice_intra_DCT (decoder, 1, dest_u + DCT_offset + 8,
				 DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v + DCT_offset + 8,
				 DCT_stride);
	    }
	} else {

	    motion_parser_t * parser;

	    parser =
		decoder->motion_parser[macroblock_modes >> MOTION_TYPE_SHIFT];
	    MOTION_CALL (parser, macroblock_modes);

	    if (macroblock_modes & MACROBLOCK_PATTERN) {
		int coded_block_pattern;
		int DCT_offset, DCT_stride;

		if (macroblock_modes & DCT_TYPE_INTERLACED) {
		    DCT_offset = decoder->stride;
		    DCT_stride = decoder->stride * 2;
		} else {
		    DCT_offset = decoder->stride * 8;
		    DCT_stride = decoder->stride;
		}

		coded_block_pattern = get_coded_block_pattern (decoder);

		if (likely (decoder->chroma_format == 0)) {
		    int offset = decoder->offset;
		    uint8_t * dest_y = decoder->dest[0] + offset;
		    if (coded_block_pattern & 1)
			slice_non_intra_DCT (decoder, 0, dest_y, DCT_stride);
		    if (coded_block_pattern & 2)
			slice_non_intra_DCT (decoder, 0, dest_y + 8,
					     DCT_stride);
		    if (coded_block_pattern & 4)
			slice_non_intra_DCT (decoder, 0, dest_y + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & 8)
			slice_non_intra_DCT (decoder, 0,
					     dest_y + DCT_offset + 8,
					     DCT_stride);
		    if (coded_block_pattern & 16)
			slice_non_intra_DCT (decoder, 1,
					     decoder->dest[1] + (offset >> 1),
					     decoder->uv_stride);
		    if (coded_block_pattern & 32)
			slice_non_intra_DCT (decoder, 2,
					     decoder->dest[2] + (offset >> 1),
					     decoder->uv_stride);
		} else if (likely (decoder->chroma_format == 1)) {
		    int offset;
		    uint8_t * dest_y;

		    coded_block_pattern |= bit_buf & (3 << 30);
		    DUMPBITS (bit_buf, bits, 2);

		    offset = decoder->offset;
		    dest_y = decoder->dest[0] + offset;
		    if (coded_block_pattern & 1)
			slice_non_intra_DCT (decoder, 0, dest_y, DCT_stride);
		    if (coded_block_pattern & 2)
			slice_non_intra_DCT (decoder, 0, dest_y + 8,
					     DCT_stride);
		    if (coded_block_pattern & 4)
			slice_non_intra_DCT (decoder, 0, dest_y + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & 8)
			slice_non_intra_DCT (decoder, 0,
					     dest_y + DCT_offset + 8,
					     DCT_stride);

		    DCT_stride >>= 1;
		    DCT_offset = (DCT_offset + offset) >> 1;
		    if (coded_block_pattern & 16)
			slice_non_intra_DCT (decoder, 1,
					     decoder->dest[1] + (offset >> 1),
					     DCT_stride);
		    if (coded_block_pattern & 32)
			slice_non_intra_DCT (decoder, 2,
					     decoder->dest[2] + (offset >> 1),
					     DCT_stride);
		    if (coded_block_pattern & (2 << 30))
			slice_non_intra_DCT (decoder, 1,
					     decoder->dest[1] + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & (1 << 30))
			slice_non_intra_DCT (decoder, 2,
					     decoder->dest[2] + DCT_offset,
					     DCT_stride);
		} else {
		    int offset;
		    uint8_t * dest_y, * dest_u, * dest_v;

		    coded_block_pattern |= bit_buf & (63 << 26);
		    DUMPBITS (bit_buf, bits, 6);

		    offset = decoder->offset;
		    dest_y = decoder->dest[0] + offset;
		    dest_u = decoder->dest[1] + offset;
		    dest_v = decoder->dest[2] + offset;

		    if (coded_block_pattern & 1)
			slice_non_intra_DCT (decoder, 0, dest_y, DCT_stride);
		    if (coded_block_pattern & 2)
			slice_non_intra_DCT (decoder, 0, dest_y + 8,
					     DCT_stride);
		    if (coded_block_pattern & 4)
			slice_non_intra_DCT (decoder, 0, dest_y + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & 8)
			slice_non_intra_DCT (decoder, 0,
					     dest_y + DCT_offset + 8,
					     DCT_stride);

		    if (coded_block_pattern & 16)
			slice_non_intra_DCT (decoder, 1, dest_u, DCT_stride);
		    if (coded_block_pattern & 32)
			slice_non_intra_DCT (decoder, 2, dest_v, DCT_stride);
		    if (coded_block_pattern & (32 << 26))
			slice_non_intra_DCT (decoder, 1, dest_u + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & (16 << 26))
			slice_non_intra_DCT (decoder, 2, dest_v + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & (8 << 26))
			slice_non_intra_DCT (decoder, 1, dest_u + 8,
					     DCT_stride);
		    if (coded_block_pattern & (4 << 26))
			slice_non_intra_DCT (decoder, 2, dest_v + 8,
					     DCT_stride);
		    if (coded_block_pattern & (2 << 26))
			slice_non_intra_DCT (decoder, 1,
					     dest_u + DCT_offset + 8,
					     DCT_stride);
		    if (coded_block_pattern & (1 << 26))
			slice_non_intra_DCT (decoder, 2,
					     dest_v + DCT_offset + 8,
					     DCT_stride);
		}
	    }

	    decoder->dc_dct_pred[0] = decoder->dc_dct_pred[1] =
		decoder->dc_dct_pred[2] = 16384;
	}

	NEXT_MACROBLOCK;

	NEEDBITS (bit_buf, bits, bit_ptr);
	mba_inc = 0;
	while (1) {
	    if (bit_buf >= 0x10000000) {
		mba = MBA_5 + (UBITS (bit_buf, 5) - 2);
		break;
	    } else if (bit_buf >= 0x03000000) {
		mba = MBA_11 + (UBITS (bit_buf, 11) - 24);
		break;
	    } else switch (UBITS (bit_buf, 11)) {
	    case 8:		/* macroblock_escape */
		mba_inc += 33;
		/* pass through */
	    case 15:	/* macroblock_stuffing (MPEG1 only) */
		DUMPBITS (bit_buf, bits, 11);
		NEEDBITS (bit_buf, bits, bit_ptr);
		continue;
	    default:	/* end of slice, or error */
		return;
	    }
	}
	DUMPBITS (bit_buf, bits, mba->len);
	mba_inc += mba->mba;

	if (mba_inc) {
	    decoder->dc_dct_pred[0] = decoder->dc_dct_pred[1] =
		decoder->dc_dct_pred[2] = 16384;

	    if (decoder->coding_type == P_TYPE) {
		do {
		    MOTION_CALL (decoder->motion_parser[0],
				 MACROBLOCK_MOTION_FORWARD);
		    NEXT_MACROBLOCK;
		} while (--mba_inc);
	    } else {
		do {
		    MOTION_CALL (decoder->motion_parser[4], macroblock_modes);
		    NEXT_MACROBLOCK;
		} while (--mba_inc);
	    }
	}
    }
#undef bit_buf
#undef bits
#undef bit_ptr
}


/* FILE: mpeg2dec-0.4.0/libmpeg2/idct.c */

static
void mpeg2_idct_init (uint32_t accel)
{
	extern uint8_t mpeg2_scan_norm[64];
	extern uint8_t mpeg2_scan_alt[64];
	int i, j;

	for (i = 0; i < 64; i++) {
	    j = mpeg2_scan_norm[i];
	    mpeg2_scan_norm[i] = ((j & 0x36) >> 1) | ((j & 0x09) << 2);
	    j = mpeg2_scan_alt[i];
	    mpeg2_scan_alt[i] = ((j & 0x36) >> 1) | ((j & 0x09) << 2);
	}
}


/* FILE: mpeg2dec-0.4.0/libmpeg2/alloc.c */

void * mpeg2_malloc (unsigned size, mpeg2_alloc_t reason)
{
    char * buf;

    if (size) {
	buf = (char *) malloc (size + 63 + sizeof (void **));
	if (buf) {
	    char * align_buf;

	    align_buf = buf + 63 + sizeof (void **);
	    align_buf -= (long)align_buf & 63;
	    *(((void **)align_buf) - 1) = buf;
	    return align_buf;
	}
    }
    return NULL;
}

void mpeg2_free (void * buf)
{
    if (buf)
	free (*(((void **)buf) - 1));
}


#endif /* MPEG_CODE_INCLUDED */
