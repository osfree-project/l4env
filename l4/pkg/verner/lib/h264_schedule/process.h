/*
 * Copyright (C) 2006 Michael Roitzsch <mroi@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 */

#include "config.h"
#define HAVE_AV_CONFIG_H

#include "avcodec.h"
#include "ssim.h"
#include "llsp.h"

/* FFmpeg wants to replace those, but I want to use them */
#undef printf
#undef fprintf

#ifdef VERNER
#include <l4/log/l4log.h>
#define printf LOG
#endif

/* The H.264 processing workbench is highly configurable with the defines
 * below. Every possible combination should compile and work, although some
 * don't make much sense, but then you will get a compile-time warning.
 * Unfortunately, this leads to quite some preprocessor hell, but you'll
 * get used to it. */

/* toggle slice boundary and reference count dump */
#define SLICEREF_DUMP		0
/* toggle metrics dump */
#define METRICS_DUMP		0
/* toggle timings dump */
#define TIMINGS_DUMP		0
/* toggle extraction and storage of decoding time metrics */
#define METRICS_EXTRACT		0
/* toggle replacement calculation */
#define REPLACEMENT		0
/* toggle extraction of slice interdependency metrics */
#define PROPAGATION		0
/* toggle writing of sideband-data-enriched H.264 file */
#define SIDEBAND_WRITE		0
/* toggle parsing of sideband data from preprocessed H.264 file */
#define SIDEBAND_READ		0
/* toggle sideband data compression */
#define SIDEBAND_COMPRESSION	0
/* toggle slice skipping and replacement based on some scheduling */
#define SLICE_SKIPPING		0
/* toggle slice error tracking */
#define SLICE_TRACKING		0
/* toggle training the execution time prediction */
#define LLSP_TRAIN		0
/* toggle execution time prediction */
#define LLSP_PREDICT		0


/* configuration presets */
#if defined(SSIM_PRECISION_SWEEP) || \
    defined(SSIM_PRECISION_FINAL) || \
    defined(REPLACEMENT_VIS) || \
    defined(DECODING_TIME_MEASURE) || \
    defined(REPLACEMENT_TIME_MEASURE) || \
    defined(PROPAGATION_VIS) || \
    defined(PROPAGATION_MEASURE) || \
    defined(PROPAGATION_HISTOGRAM) || \
    defined(PROPAGATION_ACCUMULATE) || \
    defined(REFERENCE_LIFETIME) || \
    defined(SIDEBAND_REGRESSION1) || \
    defined(SIDEBAND_REGRESSION2) || \
    defined(SIDEBAND_REGRESSION3) || \
    defined(FINAL_PREPROCESSING) || \
    defined(FINAL_REPLACEMENT) || \
    defined(LLSP_COEFFICIENTS) || \
    defined(LLSP_PREDICTION) || \
    defined(SCHEDULING_DEPTH) || \
    defined(SCHEDULING_OVERHEAD) || \
    defined(FINAL_SCHEDULING) || \
    defined(SCHEDULE_EXECUTE) || \
    defined(VERNER)
#undef SLICEREF_DUMP
#undef METRICS_DUMP
#undef TIMINGS_DUMP
#undef METRICS_EXTRACT
#undef REPLACEMENT
#undef PROPAGATION
#undef SIDEBAND_WRITE
#undef SIDEBAND_READ
#undef SIDEBAND_COMPRESSION
#undef SLICE_SKIPPING
#undef SLICE_TRACKING
#undef LLSP_TRAIN
#undef LLSP_PREDICT
#endif
#ifdef SSIM_PRECISION_SWEEP
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		1
#define PROPAGATION		0
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		0
#define SIDEBAND_COMPRESSION	0
#define SLICE_SKIPPING		0
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef SSIM_PRECISION_FINAL
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		1
#define PROPAGATION		0
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		0
#define SIDEBAND_COMPRESSION	0
#define SLICE_SKIPPING		0
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef REPLACEMENT_VIS
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		1
#define PROPAGATION		0
#define SIDEBAND_WRITE		1
#define SIDEBAND_READ		0
#define SIDEBAND_COMPRESSION	0
#define SLICE_SKIPPING		0
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef DECODING_TIME_MEASURE
#define SLICEREF_DUMP		0
#define METRICS_DUMP		1
#define TIMINGS_DUMP		1
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		0
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		0
#define SIDEBAND_COMPRESSION	0
#define SLICE_SKIPPING		0
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef REPLACEMENT_TIME_MEASURE
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		0
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		1
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		0
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef PROPAGATION_VIS
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		1
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		1
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		1
#define SLICE_TRACKING		1
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef PROPAGATION_MEASURE
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		0
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		1
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		1
#define SLICE_TRACKING		1
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef PROPAGATION_HISTOGRAM
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		0
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		1
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		1
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef PROPAGATION_ACCUMULATE
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		0
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		1
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		1
#define SLICE_TRACKING		1
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef REFERENCE_LIFETIME
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		1
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		0
#define SIDEBAND_COMPRESSION	0
#define SLICE_SKIPPING		0
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef SIDEBAND_REGRESSION1
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		1
#define REPLACEMENT		1
#define PROPAGATION		1
#define SIDEBAND_WRITE		1
#define SIDEBAND_READ		0
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		0
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef SIDEBAND_REGRESSION2
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		1
#define REPLACEMENT		1
#define PROPAGATION		1
#define SIDEBAND_WRITE		1
#define SIDEBAND_READ		1
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		0
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef SIDEBAND_REGRESSION3
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		0
#define SIDEBAND_WRITE		1
#define SIDEBAND_READ		1
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		0
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef FINAL_PREPROCESSING
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		1
#define REPLACEMENT		1
#define PROPAGATION		1
#define SIDEBAND_WRITE		1
#define SIDEBAND_READ		0
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		0
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef FINAL_REPLACEMENT
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		0
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		1
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		0
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef LLSP_COEFFICIENTS
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		0
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		1
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		0
#define SLICE_TRACKING		0
#define LLSP_TRAIN		1
#define LLSP_PREDICT		0
#endif
#ifdef LLSP_PREDICTION
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		0
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		1
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		0
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		1
#endif
#ifdef SCHEDULING_DEPTH
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		0
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		1
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		1
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		1
#endif
#ifdef SCHEDULING_OVERHEAD
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		0
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		1
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		1
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		1
#endif
#ifdef FINAL_SCHEDULING
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		0
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		1
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		1
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		1
#endif
#ifdef SCHEDULE_EXECUTE
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		0
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		1
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		1
#define SLICE_TRACKING		0
#define LLSP_TRAIN		0
#define LLSP_PREDICT		0
#endif
#ifdef VERNER
#define SLICEREF_DUMP		0
#define METRICS_DUMP		0
#define TIMINGS_DUMP		0
#define METRICS_EXTRACT		0
#define REPLACEMENT		0
#define PROPAGATION		0
#define SIDEBAND_WRITE		0
#define SIDEBAND_READ		1
#define SIDEBAND_COMPRESSION	1
#define SLICE_SKIPPING		1
#define SLICE_TRACKING		0
#define LLSP_TRAIN		1
#define LLSP_PREDICT		1
#endif

/* configuration dependencies */
#if (TIMINGS_DUMP && !FFMPEG_TIME) || ((METRICS_DUMP || METRICS_EXTRACT) && !FFMPEG_METRICS)
#warning  FFmpeg is not configured correctly, check avcodec.h
#endif
#if SIDEBAND_WRITE && (!METRICS_EXTRACT || !REPLACEMENT || !PROPAGATION) && !defined(REPLACEMENT_VIS) && !defined(SIDEBAND_REGRESSION3)
#warning  extracted sideband data might be invalid
#endif
#if SIDEBAND_COMPRESSION && !SIDEBAND_WRITE && !SIDEBAND_READ
#warning  enabling sideband compression has no effect
#endif
#if SLICE_SKIPPING && !SIDEBAND_READ
#warning  slice skipping only works properly with sideband data available
#endif
#if LLSP_PREDICT && !SIDEBAND_READ
#warning  execution time prediction only works properly with sideband data available
#endif
#if LLSP_TRAIN && !(SIDEBAND_READ || (METRICS_EXTRACT && REPLACEMENT))
#warning  training the predictor requires metrics and slice boundaries
#endif
#if SLICE_SKIPPING && (METRICS_EXTRACT || REPLACEMENT || PROPAGATION) && !defined(PROPAGATION_VIS)
#warning  slices will not be skipped for real as this would scramble the extracted sideband data
#endif

/* maximum supported number of slices per frame, must be less than 256 */
#define SLICE_MAX 32
/* maximum supported number of references per frame, must be less than 128 */
#define REF_MAX 32

/* scheduling methods */
#define COST		1
#define DIRECT_ERROR	2
#define NO_SKIP		3
#define LIFETIME	4

#if LLSP_TRAIN || LLSP_PREDICT
/* number of decoding time metrics in use */
#define METRICS_COUNT 12
/* the LLSP solver IDs */
enum { LLSP_DECODE, LLSP_REPLACE };
#endif

typedef struct replacement_node_s replacement_node_t;
typedef struct frame_node_s frame_node_t;
typedef float propagation_t[SLICE_MAX];

/* storage for per-frame data */
struct frame_node_s {
#if PROPAGATION || SLICE_TRACKING
  /* avoids premature deletion */
  int reference_count;
#endif
  
  int slice_count;
  /* storage for per-slice data, slice[SLICE_MAX] is a pseudo-slice covering the whole frame */
  struct {
    /* decoding time metrics */
#if METRICS_EXTRACT || SIDEBAND_READ
    struct {
      int type, bytes;
      int intra_pcm, intra_4x4, intra_8x8, intra_16x16;
      int inter_4x4, inter_8x8, inter_16x16;
      int idct_4x4, idct_8x8;
      int deblock_edges;
    } metrics;
#endif
#if REPLACEMENT
    /* rectangle around the slice, min inclusive, max exclusive */
    change_rect_t rect;
#endif
#if REPLACEMENT || PROPAGATION || SIDEBAND_READ
    /* macroblock indices for the slice, start inclusive, end exclusive */
    int start_index, end_index;
#endif
#if REPLACEMENT || SIDEBAND_READ
    /* measured quality loss due to replacement */
    float direct_quality_loss;
#endif
#if PROPAGATION || SLICE_TRACKING || (SCHEDULING_METHOD == LIFETIME)
    /* how much do errors from reference slices propagate into this slice */
    propagation_t immission_base[2 * REF_MAX + 1];
    /* propagation will point to propagation_base[REF_MAX], so it can be indexed
     * from -REF_MAX to REF_MAX, which is the values range of reference numbers */
    propagation_t *immission;
#endif
#if PROPAGATION || SIDEBAND_READ
    /* how much does an error from this slice propagate into future slices */
    float emission_factor;
#endif
#if SLICE_TRACKING
    /* estimated quality loss due to error propagation */
    float estimated_quality_loss;
#endif
#if LLSP_PREDICT
    /* estimated execution times */
    float decoding_time, replacement_time;
    /* benefit when this slice is decoded */
    float benefit;
#endif
#if defined(SCHEDULING_DEPTH) || defined(SCHEDULING_OVERHEAD) || defined(FINAL_SCHEDULING) || defined(VERNER)
    /* is this slice to be skipped */
    int skip;
#endif
  } slice[SLICE_MAX + 1];
  
#if REPLACEMENT || SIDEBAND_READ
  /* the root of the replacement quadtree, NULL if no replacement is possible */
  replacement_node_t *replacement;
#endif
#if PROPAGATION
  /* a copy of the reference stack for this frame */
  frame_node_t *reference_base[2 * REF_MAX + 1];
  /* reference will point to reference_base[REF_MAX], so it can be indexed
   * from -REF_MAX to REF_MAX, which is the values range of reference numbers */
  frame_node_t **reference;
#endif
#if PROPAGATION || SLICE_TRACKING || (SCHEDULING_METHOD == LIFETIME)
  /* how many future frames will see this frame in the reference stacks */
  int reference_lifetime;
#endif
  /* the next frame in decoding order */
  frame_node_t *next;
};

extern struct proc_s {
  /* frames nodes are always kept from one IDR frame to the next, because
   * some processing like backwards-accumulation of error propagation can only
   * be performed at IDR frames; the layout of the frames list is:
   * 
   * node -> node -> node -> node -> node -> node -> node -> node -> node
   *  ^                       ^                                       ^
   * last_idr                frame                                 lookahead
   * 
   * If we are not currently reading sideband data, the list ends at the "frame"
   * node and lookahead does not exist. If we are reading sideband data, the
   * distance between "frame" and "lookahead" is constant (see frame_lookahead).
   * 
   * If the upcoming frame is an IDR, the list should be processed from last_idr
   * to frame->next. This will mark the end regardless of sideband reading. */
  
  /* anchor of the frame list */
  frame_node_t *last_idr;
  /* current frame */
  frame_node_t *frame;
#if SIDEBAND_READ
  /* sideband data already read */
  frame_node_t *lookahead;
#endif
  
  /* state variables for sideband reading/writing */
  struct {
#if SIDEBAND_WRITE
    FILE *from, *to;
    uint8_t buf[4096];
    uint8_t history[3];
#if SIDEBAND_COMPRESSION
    uint32_t remain;
    uint8_t remain_bits;
#endif
#endif
#if SIDEBAND_READ
    uint8_t * restrict nalu;
#if SIDEBAND_COMPRESSION
    uint8_t bits;
#endif
#endif
#if (SIDEBAND_WRITE && PROPAGATION) || (SIDEBAND_READ && !PROPAGATION && (SLICE_TRACKING || (SCHEDULING_METHOD == LIFETIME)))
    /* the immission factors do not belong to the sideband data, but some of our
     * visualizations and measurements need them, so store them in an extra file */
    FILE *propagation;
#endif
  } sideband;
  
  /* just some helpers for error propagation visualization */
  struct {
#if defined(PROPAGATION_VIS) || defined(PROPAGATION_MEASURE) || defined(PROPAGATION_HISTOGRAM) || \
    defined(PROPAGATION_ACCUMULATE) || defined(SCHEDULE_EXECUTE)
    AVPicture vis_frame;
#endif
#ifdef PROPAGATION_HISTOGRAM
    float *error_histogram;
#endif
#if defined(PROPAGATION_VIS) || defined(PROPAGATION_MEASURE) || defined(PROPAGATION_HISTOGRAM)
    /* the coded picture number of the first degraded frame,
     * -1 if no propagation is in progress */
    int first_picture;
#endif
#ifdef SCHEDULE_EXECUTE
    double total_error;
#endif
  } propagation;
  
#if LLSP_TRAIN || LLSP_PREDICT
  struct {
#if LLSP_TRAIN
    const char *train_coeffs;
#endif
#if LLSP_PREDICT
    const char *predict_coeffs;
#endif
    llsp_t *decode;
    llsp_t *replace;
#if LLSP_TRAIN || defined(LLSP_PREDICTION)
    double decoding_time;
#endif
  } llsp;
#endif
  
#ifdef SCHEDULE_EXECUTE
  struct {
    int conceal;
    int first_to_drop;
  } schedule;
#endif
  /* current video size */
  int mb_width, mb_height;
#if REPLACEMENT || LLSP_TRAIN || defined(REPLACEMENT_TIME_MEASURE) || defined(LLSP_PREDICTION)
  /* intermediary frame storage */
  AVPicture temp_frame;
#endif
#if REPLACEMENT || PROPAGATION
  /* translated reference numbers (slice-local to global) of the current frame */
  int8_t *ref_num[2];
#endif
} proc;

static const int mb_size_log = 4;	/* log2 of the edge length of a macroblock */
static const float ssim_precision = 0.05;
static const float subdivision_threshold = 0.01;
static const int frame_lookahead = 25;
static const int output_queue = 10;     /* length of simulated player's frame queue */
static const float safety_margin_decode  = 1.1;
static const float safety_margin_replace = 1.2;

/* process.c  --  this is where it all begins */
void process_init(AVCodecContext *c, char *file);
void process_finish(AVCodecContext *c);

/* metrics.c  --  handling of decoding time metrics */
#if SLICEREF_DUMP || METRICS_DUMP || TIMINGS_DUMP
void dump_metrics(const AVCodecContext *c);
#endif
#if METRICS_EXTRACT
void remember_metrics(const AVCodecContext *c);
#endif
#if SIDEBAND_WRITE && (METRICS_EXTRACT || SIDEBAND_READ)
void write_metrics(const frame_node_t *frame, int slice);
#endif
#if SIDEBAND_READ
void read_metrics(frame_node_t *frame, int slice);
#endif
#if LLSP_TRAIN || LLSP_PREDICT
const double *metrics_decode(const frame_node_t *frame, int slice);
const double *metrics_replace(const frame_node_t *frame, int slice);
#endif

/* schedule.c  --  the slice scheduler */
#if SLICE_SKIPPING
int schedule_skip(const AVCodecContext *c, int current_slice);
#endif
#if LLSP_TRAIN || defined(LLSP_PREDICTION) || defined(SCHEDULING_DEPTH) || defined(SCHEDULING_OVERHEAD) || defined(FINAL_SCHEDULING) || defined(VERNER)
double get_time(void);
#endif
#ifdef VERNER
void h264_prediction_files(const char *train, const char *predict);
void h264_machine_speed(float speed);
#endif

/* replace.c  --  the replacement quadtree */
#if REPLACEMENT
void search_replacements(const AVCodecContext *c, replacement_node_t *node);
#endif
#if REPLACEMENT || SIDEBAND_READ
void destroy_replacement_tree(replacement_node_t *node);
#endif
#if defined(REPLACEMENT_VIS) || defined(FINAL_REPLACEMENT)
int frame_storage_alloc(AVCodecContext *c, AVFrame *frame);
void frame_storage_destroy(AVCodecContext *c, AVFrame *frame);
void replacement_visualize(const AVCodecContext *c);
#endif
#if REPLACEMENT || PROPAGATION
void remember_slice_boundaries(const AVCodecContext *c);
void remember_reference_frames(const AVCodecContext *c);
#endif
#if LLSP_TRAIN || defined(REPLACEMENT_TIME_MEASURE) || defined(LLSP_PREDICTION)
void replacement_time(AVCodecContext *c);
#endif
#if SLICE_SKIPPING
int check_skip_next_slice(const AVCodecContext *c);
#endif
#if SIDEBAND_WRITE && (REPLACEMENT || SIDEBAND_READ)
void write_replacement_tree(const replacement_node_t *node);
#endif
#if SIDEBAND_READ
void read_replacement_tree(replacement_node_t *node);
#endif

/* propagate.c  --  error propagation */
#if PROPAGATION
void remember_dependencies(const AVCodecContext *c);
void accumulate_quality_loss(frame_node_t *frame);
#endif
#if PROPAGATION || SLICE_TRACKING
int frame_storage_alloc(AVCodecContext *c, AVFrame *frame);
void frame_storage_destroy(AVCodecContext *c, AVFrame *frame);
#endif
#if defined(PROPAGATION_VIS) || defined(PROPAGATION_MEASURE) || defined(PROPAGATION_HISTOGRAM) || \
    defined(PROPAGATION_ACCUMULATE) || defined(SCHEDULE_EXECUTE)
void propagation_visualize(const AVCodecContext *c);
#ifdef PROPAGATION_HISTOGRAM
void propagation_finalize(void);
#endif
#endif
#if SIDEBAND_WRITE && PROPAGATION && !SIDEBAND_READ
void write_immission(const frame_node_t *frame);
#endif
#if SIDEBAND_READ && !PROPAGATION && (SLICE_TRACKING || (SCHEDULING_METHOD == LIFETIME))
void read_immission(frame_node_t *frame);
#endif

/* nalu.c  --  sideband data read and write */
#if SIDEBAND_WRITE
int slice_start(void);
void copy_nalu(void);
void nalu_write_start(void);
void nalu_write_end(void);
void nalu_write_uint8(uint8_t value);
void nalu_write_uint16(uint16_t value);
void nalu_write_uint24(uint32_t value);
void nalu_write_uint32(uint32_t value);
void nalu_write_int8(int8_t value);
void nalu_write_int16(int16_t value);
void nalu_write_float(float value);
#endif
#if SIDEBAND_READ
void nalu_read_start(uint8_t *nalu);
uint8_t nalu_read_uint8(void);
uint16_t nalu_read_uint16(void);
uint32_t nalu_read_uint24(void);
uint32_t nalu_read_uint32(void);
int8_t nalu_read_int8(void);
int16_t nalu_read_int16(void);
float nalu_read_float(void);
#endif

/* for efficient handling of eight bytes at once */
typedef uint64_t byte_block_t;
static const byte_block_t byte_spread = 0x0101010101010101ULL;
#define BLOCK(base, offset) *cast_to_byte_block_pointer(base, offset)
static inline byte_block_t *cast_to_byte_block_pointer(const uint8_t *const restrict base, const int offset)
{
  return (byte_block_t *)(base + offset);
}

/* upcasting an AVFrame's opaque pointer */
#if defined(REPLACEMENT_VIS) || defined(FINAL_REPLACEMENT)
static inline uint8_t *private_data(const AVFrame *frame)
{
  return (uint8_t *)frame->opaque;
}
#endif
#if PROPAGATION || SLICE_TRACKING
static inline frame_node_t *private_data(const AVFrame *frame)
{
  return *(frame_node_t **)frame->opaque;
}
#endif
