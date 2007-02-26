/*
 * Copyright (C) 2006 Michael Roitzsch <mroi@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 */

#include "process.h"


#if SLICEREF_DUMP || METRICS_DUMP || TIMINGS_DUMP
void dump_metrics(const AVCodecContext *c)
{
#if SLICEREF_DUMP
  if (c->metrics.type >= 0) 
    printf("slice start: %d, end: %d, references: %d + %d\n",
	   c->slice.start_index, c->slice.end_index,
	   c->reference.short_count, c->reference.long_count);
#endif
#if METRICS_DUMP
  printf("%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, ",
	 c->metrics.type,                              //  1
	 (c->slice.end_index - c->slice.start_index),  //  2
	 c->metrics.bytes,                             //  3
	 c->metrics.intra_pcm,                         //  4
	 c->metrics.intra_4x4,                         //  5
	 c->metrics.intra_8x8,                         //  6
	 c->metrics.intra_16x16,                       //  7
	 c->metrics.inter_4x4,                         //  8
	 c->metrics.inter_8x8,                         //  9
	 c->metrics.inter_16x16,                       // 10
	 c->metrics.idct_4x4,                          // 11
	 c->metrics.idct_8x8,                          // 12
	 c->metrics.deblock_edges);                    // 13
#endif
#if TIMINGS_DUMP
  printf("%llu, %llu, %llu, %llu, %llu, %llu, %llu, ",
	 c->timing.decoder_prep,                       // 14
	 c->timing.decompress_cabac,                   // 15
	 c->timing.decompress_cavlc,                   // 16
	 c->timing.spatial_pred,                       // 17
	 c->timing.temporal_pred,                      // 18
	 c->timing.idct,                               // 19
	 c->timing.post);                              // 20
#endif
#if METRICS_DUMP || TIMINGS_DUMP
  printf("%llu\n", c->timing.total);                   // 21
#endif
}
#endif

#if METRICS_EXTRACT
void remember_metrics(const AVCodecContext *c)
{
  proc.frame->slice[proc.frame->slice_count].metrics.type          = c->metrics.type;
  proc.frame->slice[proc.frame->slice_count].metrics.bytes         = c->metrics.bytes;
  proc.frame->slice[proc.frame->slice_count].metrics.intra_pcm     = c->metrics.intra_pcm;
  proc.frame->slice[proc.frame->slice_count].metrics.intra_4x4     = c->metrics.intra_4x4;
  proc.frame->slice[proc.frame->slice_count].metrics.intra_8x8     = c->metrics.intra_8x8;
  proc.frame->slice[proc.frame->slice_count].metrics.intra_16x16   = c->metrics.intra_16x16;
  proc.frame->slice[proc.frame->slice_count].metrics.inter_4x4     = c->metrics.inter_4x4;
  proc.frame->slice[proc.frame->slice_count].metrics.inter_8x8     = c->metrics.inter_8x8;
  proc.frame->slice[proc.frame->slice_count].metrics.inter_16x16   = c->metrics.inter_16x16;
  proc.frame->slice[proc.frame->slice_count].metrics.idct_4x4      = c->metrics.idct_4x4;
  proc.frame->slice[proc.frame->slice_count].metrics.idct_8x8      = c->metrics.idct_8x8;
  proc.frame->slice[proc.frame->slice_count].metrics.deblock_edges = c->metrics.deblock_edges;
}
#endif

#if SIDEBAND_WRITE && (METRICS_EXTRACT || SIDEBAND_READ)
void write_metrics(const frame_node_t *frame, int slice)
{
  nalu_write_uint8(frame->slice[slice].metrics.type);
  nalu_write_uint24(frame->slice[slice].metrics.bytes);
  nalu_write_uint24(frame->slice[slice].metrics.intra_pcm);
  nalu_write_uint24(frame->slice[slice].metrics.intra_4x4);
  nalu_write_uint24(frame->slice[slice].metrics.intra_8x8);
  nalu_write_uint24(frame->slice[slice].metrics.intra_16x16);
  nalu_write_uint24(frame->slice[slice].metrics.inter_4x4);
  nalu_write_uint24(frame->slice[slice].metrics.inter_8x8);
  nalu_write_uint24(frame->slice[slice].metrics.inter_16x16);
  nalu_write_uint24(frame->slice[slice].metrics.idct_4x4);
  nalu_write_uint24(frame->slice[slice].metrics.idct_8x8);
  nalu_write_uint24(frame->slice[slice].metrics.deblock_edges);
}
#endif

#if SIDEBAND_READ
void read_metrics(frame_node_t *frame, int slice)
{
  frame->slice[slice].metrics.type          = nalu_read_uint8();
  frame->slice[slice].metrics.bytes         = nalu_read_uint24();
  frame->slice[slice].metrics.intra_pcm     = nalu_read_uint24();
  frame->slice[slice].metrics.intra_4x4     = nalu_read_uint24();
  frame->slice[slice].metrics.intra_8x8     = nalu_read_uint24();
  frame->slice[slice].metrics.intra_16x16   = nalu_read_uint24();
  frame->slice[slice].metrics.inter_4x4     = nalu_read_uint24();
  frame->slice[slice].metrics.inter_8x8     = nalu_read_uint24();
  frame->slice[slice].metrics.inter_16x16   = nalu_read_uint24();
  frame->slice[slice].metrics.idct_4x4      = nalu_read_uint24();
  frame->slice[slice].metrics.idct_8x8      = nalu_read_uint24();
  frame->slice[slice].metrics.deblock_edges = nalu_read_uint24();
}
#endif

#if LLSP_TRAIN || LLSP_PREDICT
const double *metrics_decode(const frame_node_t *frame, int slice)
{
  static double metrics[METRICS_COUNT];
  metrics[ 0] = frame->slice[slice].end_index - frame->slice[slice].start_index;
  metrics[ 1] = frame->slice[slice].metrics.bytes;
  metrics[ 2] = frame->slice[slice].metrics.intra_pcm;
  metrics[ 3] = frame->slice[slice].metrics.intra_4x4;
  metrics[ 4] = frame->slice[slice].metrics.intra_8x8;
  metrics[ 5] = frame->slice[slice].metrics.intra_16x16;
  metrics[ 6] = frame->slice[slice].metrics.inter_4x4;
  metrics[ 7] = frame->slice[slice].metrics.inter_8x8;
  metrics[ 8] = frame->slice[slice].metrics.inter_16x16;
  metrics[ 9] = frame->slice[slice].metrics.idct_4x4;
  metrics[10] = frame->slice[slice].metrics.idct_8x8;
  metrics[11] = frame->slice[slice].metrics.deblock_edges;
  return metrics;
}
const double *metrics_replace(const frame_node_t *frame, int slice)
{
  static double metrics[1];
  metrics[0] = frame->slice[slice].end_index - frame->slice[slice].start_index;
  return metrics;
}
#endif
