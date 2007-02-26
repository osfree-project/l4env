/*
 * Copyright (C) 2006 Michael Roitzsch <mroi@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 */

#include "process.h"

#if defined(PROPAGATION_VIS) || defined(PROPAGATION_ACCUMULATE)
#include "digits.h"
#endif

#if defined(PROPAGATION_VIS) || defined(PROPAGATION_MEASURE) || defined(PROPAGATION_ACCUMULATE)
static void draw_slice_error(const AVCodecContext *c, AVPicture *quad);
#endif
#ifdef SCHEDULE_EXECUTE
static void update_total_error(const AVCodecContext *c, const AVPicture *quad);
#endif


#if PROPAGATION
#include "mpegvideo.h"

void remember_dependencies(const AVCodecContext *c)
{
  int mb, list, i, x, y, ref, slice;
  
  if (proc.frame->slice_count == 0) {
    proc.frame->reference_lifetime = 0;
    /* first copy the current reference stacks */
    for (ref = -REF_MAX; ref <= REF_MAX; ref++) {
      if (ref > 0 && ref - 1 < c->reference.short_count)
	proc.frame->reference[ref] = private_data(c->reference.short_list[ref - 1]);
      else if (ref < 0 && -ref - 1 < c->reference.long_count)
	proc.frame->reference[ref] = private_data(c->reference.long_list[-ref - 1]);
      else
	proc.frame->reference[ref] = NULL;
      /* increment the lifetime of the references we can still see */
      if (proc.frame->reference[ref]) proc.frame->reference[ref]->reference_lifetime++;
    }
  }
  
  /* spatial prediction dependencies are ignored, because I have not observed them */
  
  for (mb = proc.frame->slice[proc.frame->slice_count].start_index;
       mb < proc.frame->slice[proc.frame->slice_count].end_index; mb++) {
    const int mb_y = mb / proc.mb_width;
    const int mb_x = mb % proc.mb_width;
    const int mb_stride = proc.mb_width + 1;
    const int mb_index = mb_x + mb_y * mb_stride;
    const int ref_stride = 2 * proc.mb_width;
    const int ref_index_base = 2*mb_x + 2*mb_y * ref_stride;
    const int mv_sample_log2 = 4 - c->frame.current->motion_subsample_log2;
    const int mv_stride = proc.mb_width << mv_sample_log2;
    const int mv_index_base = (mb_x << mv_sample_log2) + (mb_y << mv_sample_log2) * mv_stride;
    const int mb_type = c->frame.current->mb_type[mb_index];
    for (list = 0; list < 2; list++) {
      for (i = 0; i < 16; i++) {
	const int ref_index = ref_index_base + ((i>>1)&1) + (i>>3) * ref_stride;
	const int mv_index = mv_index_base + (i&3) + (i>>2) * mv_stride;
	if (proc.ref_num[list][ref_index]) {
	  const frame_node_t *frame = proc.frame->reference[proc.ref_num[list][ref_index]];
	  if ((IS_16X16(mb_type) && (i&15) == 0) ||
	      (IS_16X8(mb_type) && (i&(15-8)) == 0) ||
	      (IS_8X16(mb_type) && (i&(15-2)) == 0) ||
	      (IS_8X8(mb_type) && (i&(15-8-2)) == 0)) {
	    /* FIXME: we cannot distinguish the 8x8, 8x4, 4x8 and 4x4 subtypes */
	    const int start_x = (mb_x << mb_size_log) + c->frame.current->motion_val[list][mv_index][0] / 4;
	    const int start_y = (mb_y << mb_size_log) + c->frame.current->motion_val[list][mv_index][1] / 4;
	    const int size_x = (IS_16X16(mb_type) || IS_16X8(mb_type)) ? 16 : 8;
	    const int size_y = (IS_16X16(mb_type) || IS_8X16(mb_type)) ? 16 : 8;
	    /* check every individual pixel of this motion block */
	    for (y = start_y; y < start_y + size_y; y++) {
	      for (x = start_x; x < start_x + size_x; x++) {
		int clip_x = x >> mb_size_log;
		int clip_y = y >> mb_size_log;
		if (clip_x < 0) x = 0;
		if (clip_y < 0) y = 0;
		if (clip_x >= proc.mb_width) clip_x = proc.mb_width - 1;
		if (clip_y >= proc.mb_height) clip_y = proc.mb_height - 1;
		const int mb_index = clip_x + clip_y * proc.mb_width;
		/* search for the slice this pixel came from */
		for (slice = 0; slice < frame->slice_count; slice++) {
		  if (mb_index >= frame->slice[slice].start_index && mb_index < frame->slice[slice].end_index) {
		    /* add .5 for bi-prediction, 1 for normal prediction */
		    float contrib = .5 + .5 * (float)!proc.ref_num[list ^ 1][ref_index];
		    proc.frame->slice[proc.frame->slice_count].immission[proc.ref_num[list][ref_index]][slice] += contrib;
		    break;
		  }
		}
	      }
	    }
	  }
	}
      }
    }
  }
  
  for (ref = -REF_MAX; ref <= REF_MAX; ref++) {
    const frame_node_t *reference = proc.frame->reference[ref];
    if (!reference) {
      for (slice = 0; slice < SLICE_MAX; slice++)
	assert(proc.frame->slice[proc.frame->slice_count].immission[ref][slice] == 0.0);
      continue;
    }
    for (slice = 0; slice < SLICE_MAX; slice++) {
      /* scale the propagation factors by the size of the reference slice,
       * so that copying an entire reference-slice completely will result in factor 1 for that slice */
      const int slice_mbs = reference->slice[slice].end_index - reference->slice[slice].start_index;
      const int slice_pixels = (1 << (2 * mb_size_log)) * slice_mbs;
      proc.frame->slice[proc.frame->slice_count].immission[ref][slice] /= slice_pixels;
    }
  }
}

void accumulate_quality_loss(frame_node_t *frame)
{
  frame_node_t *future;
  int slice_here, slice_there, ref;
  
  if (!proc.frame || frame == proc.frame->next) return;
  
#ifdef REFERENCE_LIFETIME
  printf("%d\n", frame->reference_lifetime);
#endif
  
  /* accumulate backwards, so do recursion first */
  accumulate_quality_loss(frame->next);
  
  /* initialize with ones */
  for (slice_here = 0; slice_here < frame->slice_count; slice_here++)
    frame->slice[slice_here].emission_factor = 1.0;
  
  /* search all future frames for direct references to this one */
  for (future = frame->next; future != proc.frame->next; future = future->next) {
    for (ref = -REF_MAX; ref <= REF_MAX; ref++) {
      if (future->reference[ref] == frame) {
	/* this future frame references our frame with reference number ref */
	for (slice_there = 0; slice_there < future->slice_count; slice_there++) {
	  for (slice_here = 0; slice_here < frame->slice_count; slice_here++) {
	    /* this is the factor, by which errors in slice_here are propagated into slice_there directly */
	    const float propagation_level1 = future->slice[slice_there].immission[ref][slice_here];
	    /* this is the factor, by which errors in slice_there are propagated further ahead,
	     * since the recursion has already worked on this frame, we can use this value here */
	    const float propagation_level2 = future->slice[slice_there].emission_factor;
	    /* the complete quality loss factor along this propagation path */
	    const float propagation_path = propagation_level1 * propagation_level2;
	    /* accumulate */
	    frame->slice[slice_here].emission_factor += propagation_path;
	  }
	}
      }
    }
  }
}
#endif

#if PROPAGATION || SLICE_TRACKING
int frame_storage_alloc(AVCodecContext *c, AVFrame *frame)
{
  frame->opaque = av_malloc(sizeof(frame_node_t *));
  *(void **)frame->opaque = NULL;
  return avcodec_default_get_buffer(c, frame);
}
void frame_storage_destroy(AVCodecContext *c, AVFrame *frame)
{
  frame_node_t *node = private_data(frame);
  av_freep(&frame->opaque);
  if (node && !--node->reference_count)
    av_free(node);
  avcodec_default_release_buffer(c, frame);
}
#endif

#if defined(PROPAGATION_VIS) || defined(PROPAGATION_MEASURE) || defined(PROPAGATION_HISTOGRAM) || \
    defined(PROPAGATION_ACCUMULATE) || defined(SCHEDULE_EXECUTE)

void propagation_visualize(const AVCodecContext *c)
{
  int y;
  static FILE *original = NULL;
  if (!original) original = fopen("Original.yuv", "r");
  
  if (!c->frame.display) return;
  
#if defined(PROPAGATION_VIS) || defined(PROPAGATION_MEASURE) || defined(PROPAGATION_HISTOGRAM)
  if (c->frame.display->coded_picture_number >= (unsigned)proc.propagation.first_picture) {
#else
  if (1) {
#endif
    AVPicture quad = proc.propagation.vis_frame;
#if defined(PROPAGATION_VIS) || defined(PROPAGATION_ACCUMULATE) || defined(SCHEDULE_EXECUTE)
    static FILE *vis = NULL;
#endif
    
    /* left upper quadrant: original */
    /* Y */
    for (y = 0; y < c->height; y++)
      fread(quad.data[0] + y * quad.linesize[0], sizeof(uint8_t), c->width, original);
    /* Cb */
    for (y = 0; y < c->height / 2; y++)
      fread(quad.data[1] + y * quad.linesize[1], sizeof(uint8_t), c->width / 2, original);
    /* Cr */
    for (y = 0; y < c->height / 2; y++)
      fread(quad.data[2] + y * quad.linesize[2], sizeof(uint8_t), c->width / 2, original);
    /* right upper quadrant: with propagated error */
    quad.data[0] += c->width;
    quad.data[1] += c->width / 2;
    quad.data[2] += c->width / 2;
#ifdef SCHEDULE_EXECUTE
    if (c->frame.display->coded_picture_number < (unsigned)proc.schedule.first_to_drop)
      img_copy(&quad, (const AVPicture *)c->frame.display, PIX_FMT_YUV420P, c->width, c->height);
    update_total_error(c, &quad);
#else
    img_copy(&quad, (const AVPicture *)c->frame.display, PIX_FMT_YUV420P, c->width, c->height);
    /* left lower quadrant: slice error map */
    quad.data[0] += (c->height    ) * quad.linesize[0] - (c->width    );
    quad.data[1] += (c->height / 2) * quad.linesize[1] - (c->width / 2);
    quad.data[2] += (c->height / 2) * quad.linesize[2] - (c->width / 2);
#if defined(PROPAGATION_VIS) || defined(PROPAGATION_MEASURE) || defined(PROPAGATION_ACCUMULATE)
    draw_slice_error(c, &quad);
#endif
    /* right lower quadrant: SSIM error map */
    quad.data[0] += c->width;
    quad.data[1] += c->width / 2;
    quad.data[2] += c->width / 2;
#if defined(PROPAGATION_VIS) || defined(PROPAGATION_HISTOGRAM) || defined(PROPAGATION_ACCUMULATE)
    ssim_map(proc.propagation.vis_frame.data[0], proc.propagation.vis_frame.data[0] + c->width, quad.data[0],
#ifdef PROPAGATION_HISTOGRAM
      proc.propagation.error_histogram,
#else
      NULL,
#endif
      c->width, c->height, quad.linesize[0]);
#endif
#endif
    
#if defined(PROPAGATION_VIS) || defined(PROPAGATION_ACCUMULATE)
    if (!vis) vis = fopen("Visualization.yuv", "w");
    /* Y */
    for (y = 0; y < 2 * c->height; y++)
      fwrite(proc.propagation.vis_frame.data[0] + y * proc.propagation.vis_frame.linesize[0], sizeof(uint8_t), 2 * c->width, vis);
    /* Cb */
    for (y = 0; y < c->height; y++)
      fwrite(proc.propagation.vis_frame.data[1] + y * proc.propagation.vis_frame.linesize[1], sizeof(uint8_t), c->width, vis);
    /* Cr */
    for (y = 0; y < c->height; y++)
      fwrite(proc.propagation.vis_frame.data[2] + y * proc.propagation.vis_frame.linesize[2], sizeof(uint8_t), c->width, vis);
#endif
#ifdef SCHEDULE_EXECUTE
    if (!vis) vis = fopen("Visualization.yuv", "w");
    /* Y */
    for (y = 0; y < c->height; y++)
      fwrite(quad.data[0] + y * quad.linesize[0], sizeof(uint8_t), c->width, vis);
    /* Cb */
    for (y = 0; y < c->height / 2; y++)
      fwrite(quad.data[1] + y * quad.linesize[1], sizeof(uint8_t), c->width / 2, vis);
    /* Cr */
    for (y = 0; y < c->height / 2; y++)
      fwrite(quad.data[2] + y * quad.linesize[2], sizeof(uint8_t), c->width / 2, vis);
#endif
  } else {
    /* skip one frame in the original */
    /* Y */
    for (y = 0; y < c->height; y++)
      fread(proc.propagation.vis_frame.data[0] + y * proc.propagation.vis_frame.linesize[0], sizeof(uint8_t), c->width, original);
    /* Cb */
    for (y = 0; y < c->height / 2; y++)
      fread(proc.propagation.vis_frame.data[1] + y * proc.propagation.vis_frame.linesize[1], sizeof(uint8_t), c->width / 2, original);
    /* Cr */
    for (y = 0; y < c->height / 2; y++)
      fread(proc.propagation.vis_frame.data[2] + y * proc.propagation.vis_frame.linesize[2], sizeof(uint8_t), c->width / 2, original);
  }
}

#if defined(PROPAGATION_VIS) || defined(PROPAGATION_MEASURE) || defined(PROPAGATION_ACCUMULATE)
static void draw_slice_error(const AVCodecContext *c, AVPicture *quad)
{
  picture_t original, degraded;
  frame_node_t *frame = private_data(c->frame.display);
  int slice, mb, y, x;
  float measured_error;
#if defined(PROPAGATION_VIS) || defined(PROPAGATION_ACCUMULATE)
  /* holds per-slice measured and predicted error */
  byte_block_t error[SLICE_MAX][2];
#endif
  
  /* the original frame is in the upper left quadrant */
  original.Y  = proc.propagation.vis_frame.data[0];
  original.Cb = proc.propagation.vis_frame.data[1];
  original.Cr = proc.propagation.vis_frame.data[2];
  original.line_stride_Y  = proc.propagation.vis_frame.linesize[0];
  original.line_stride_Cb = proc.propagation.vis_frame.linesize[1];
  original.line_stride_Cr = proc.propagation.vis_frame.linesize[2];
  original.width  = c->width;
  original.height = c->height;
  
  /* the degraded version uses current quadrant as temporary storage */
  degraded.Y  = quad->data[0];
  degraded.Cb = quad->data[1];
  degraded.Cr = quad->data[2];
  degraded.line_stride_Y  = quad->linesize[0];
  degraded.line_stride_Cb = quad->linesize[1];
  degraded.line_stride_Cr = quad->linesize[2];
  degraded.width  = c->width;
  degraded.height = c->height;
  
  for (slice = 0; slice < frame->slice_count; slice++) {
    /* copy the original frame to temp */
    img_copy(quad, &proc.propagation.vis_frame, PIX_FMT_YUV420P, c->width, c->height);
    /* replace slice with the degraded version */
    for (mb = frame->slice[slice].start_index; mb < frame->slice[slice].end_index; mb++) {
      const int mb_x = mb % proc.mb_width;
      const int mb_y = mb / proc.mb_width;
      int start_x = mb_x << mb_size_log;
      int start_y = mb_y << mb_size_log;
      int end_x = (mb_x + 1) << mb_size_log;
      int end_y = (mb_y + 1) << mb_size_log;
      for (y = start_y; y < end_y; y++)
	for (x = start_x; x < end_x; x += sizeof(byte_block_t))
	  BLOCK(quad->data[0], x + y * quad->linesize[0]) =
	    BLOCK(c->frame.display->data[0], x + y * c->frame.display->linesize[0]);
      start_x = mb_x << (mb_size_log - 1);
      start_y = mb_y << (mb_size_log - 1);
      end_x = (mb_x + 1) << (mb_size_log - 1);
      end_y = (mb_y + 1) << (mb_size_log - 1);
      /* Cb */
      for (y = start_y; y < end_y; y++)
	for (x = start_x; x < end_x; x += sizeof(byte_block_t))
	  BLOCK(quad->data[1], x + y * quad->linesize[1]) =
	    BLOCK(c->frame.display->data[1], x + y * c->frame.display->linesize[1]);
      /* Cr */
      for (y = start_y; y < end_y; y++)
	for (x = start_x; x < end_x; x += sizeof(byte_block_t))
	  BLOCK(quad->data[2], x + y * quad->linesize[2]) =
	    BLOCK(c->frame.display->data[2], x + y * c->frame.display->linesize[2]);
    }
    /* calculate the error */
    measured_error = ssim_quality_loss(&original, &degraded, NULL, ssim_precision);
#if defined(PROPAGATION_VIS) || defined(PROPAGATION_ACCUMULATE)
    error[slice][0] = byte_spread *
      ((5 * frame->slice_count * measured_error < 1.0) ?
	(uint8_t)(0xFF * 5 * frame->slice_count * measured_error) :
	0xFF);
#if SLICE_TRACKING
    error[slice][1] = byte_spread *
      ((5 * frame->slice_count * frame->slice[slice].estimated_quality_loss < 1.0) ?
       (uint8_t)(0xFF * 5 * frame->slice_count * frame->slice[slice].estimated_quality_loss) :
       0xFF);
#else
    error[slice][1] = error[slice][0];
#endif
#endif
#if SLICE_TRACKING
    /* print estimated and measured error */
    printf("%f, %f\n", frame->slice[slice].estimated_quality_loss, measured_error);
#endif
  }
  
#if defined(PROPAGATION_VIS) || defined(PROPAGATION_ACCUMULATE)
  /* clear the entire lower half, because it has been polluted with temp */
  memset(quad->data[0], 0x00, quad->linesize[0] * (proc.mb_height << (mb_size_log+0)));
  memset(quad->data[1], 0x80, quad->linesize[1] * (proc.mb_height << (mb_size_log-1)));
  memset(quad->data[2], 0x80, quad->linesize[2] * (proc.mb_height << (mb_size_log-1)));
  
  /* draw the error */
  for (slice = 0; slice < frame->slice_count; slice++) {
    for (mb = frame->slice[slice].start_index; mb < frame->slice[slice].end_index; mb++) {
      const int mb_x = mb % proc.mb_width;
      const int mb_y = mb / proc.mb_width;
      const int start_x = mb_x << mb_size_log;
      const int start_y = mb_y << mb_size_log;
      const int end_x = (mb_x + 1) << mb_size_log;
      const int end_y = (mb_y + 1) << mb_size_log;
	for (y = start_y; y < end_y; y++)
	  for (x = start_x; x < end_x; x += sizeof(byte_block_t))
	    BLOCK(quad->data[0], x + y * quad->linesize[0]) =
	      error[slice][y < start_y + (1 << (mb_size_log - 1))];
    }
    /* print slice number */
    print_number(quad, frame->slice[slice].start_index, frame->slice[slice].end_index);
  }
#endif
}
#endif

#ifdef SCHEDULE_EXECUTE
static void update_total_error(const AVCodecContext *c, const AVPicture *quad)
{
  picture_t original, degraded;
  
  /* the original frame is in the upper left quadrant */
  original.Y  = proc.propagation.vis_frame.data[0];
  original.Cb = proc.propagation.vis_frame.data[1];
  original.Cr = proc.propagation.vis_frame.data[2];
  original.line_stride_Y  = proc.propagation.vis_frame.linesize[0];
  original.line_stride_Cb = proc.propagation.vis_frame.linesize[1];
  original.line_stride_Cr = proc.propagation.vis_frame.linesize[2];
  original.width  = c->width;
  original.height = c->height;
  
  /* the degraded version is in the current quadrant */
  degraded.Y  = quad->data[0];
  degraded.Cb = quad->data[1];
  degraded.Cr = quad->data[2];
  degraded.line_stride_Y  = quad->linesize[0];
  degraded.line_stride_Cb = quad->linesize[1];
  degraded.line_stride_Cr = quad->linesize[2];
  degraded.width  = c->width;
  degraded.height = c->height;
  
  proc.propagation.total_error += ssim_quality_loss(&original, &degraded, NULL, ssim_precision);
}
#endif

#ifdef PROPAGATION_HISTOGRAM
void propagation_finalize(void)
{
  uint32_t width, height;
  int x, y;
  FILE *hist;
  
  /* read and add a previously saved histogram */
  hist = fopen("Histogram.dat", "r");
  if (hist) {
    fread(&width,  sizeof(width),  1, hist);
    fread(&height, sizeof(height), 1, hist);
    if (width == (proc.mb_width << mb_size_log) && height == (proc.mb_height << mb_size_log)) {
      float buffer[width];
      for (y = 0; y < height; y++) {
	fread(buffer, sizeof(buffer[0]), width, hist);
	for (x = 0; x < width; x++)
	  proc.propagation.error_histogram[y * width + x] += buffer[x];
      }
    }
    fclose(hist);
  }
  
  /* store the new histogram */
  hist = fopen("Histogram.dat", "w");
  width  = proc.mb_width  << mb_size_log;
  height = proc.mb_height << mb_size_log;
  fwrite(&width , sizeof(width),  1, hist);
  fwrite(&height, sizeof(height), 1, hist);
  for (y = 0; y < height; y++)
    fwrite(proc.propagation.error_histogram + y * width, sizeof(proc.propagation.error_histogram[0]), width, hist);
  fclose(hist);
}
#endif

#endif

#if SIDEBAND_WRITE && PROPAGATION && !SIDEBAND_READ
void write_immission(const frame_node_t *frame)
{
  uint8_t buf[sizeof(uint32_t)];
  int slice_here, slice_there, ref;
  
  if (!proc.sideband.propagation) return;
  
  for (slice_here = 0; slice_here < frame->slice_count; slice_here++) {
    for (ref = -REF_MAX; ref <= REF_MAX; ref++) {
      for (slice_there = 0; slice_there < SLICE_MAX; slice_there++) {
        if (frame->slice[slice_here].immission[ref][slice_there] > 0.0) {
	  uint32_t out;
	  float *in = (float *)(void *)&out;
	  
	  assert(sizeof(float) == sizeof(uint32_t));
	  *in = frame->slice[slice_here].immission[ref][slice_there];
	  buf[0] = (out >> 24) & 0xFF;
	  buf[1] = (out >> 16) & 0xFF;
	  buf[2] = (out >>  8) & 0xFF;
	  buf[3] = (out >>  0) & 0xFF;
	  fwrite(buf, sizeof(buf[0]), 4, proc.sideband.propagation);
	  buf[0] = (uint8_t)ref;
	  buf[1] = slice_there;
	  fwrite(buf, sizeof(buf[0]), 2, proc.sideband.propagation);
        }
      }
    }
    /* end marker */
    buf[0] = buf[1] = buf[2] = buf[3] = 0;
    fwrite(buf, sizeof(buf[0]), 4, proc.sideband.propagation);
  }
  buf[0] = (frame->reference_lifetime >> 24) & 0xFF;
  buf[1] = (frame->reference_lifetime >> 16) & 0xFF;
  buf[2] = (frame->reference_lifetime >>  8) & 0xFF;
  buf[3] = (frame->reference_lifetime >>  0) & 0xFF;
  fwrite(buf, sizeof(buf[0]), 4, proc.sideband.propagation);
}
#endif

#if SIDEBAND_READ && !PROPAGATION && (SLICE_TRACKING || (SCHEDULING_METHOD == LIFETIME))
void read_immission(frame_node_t *frame)
{
  uint8_t buf[sizeof(uint32_t)];
  int slice_here, slice_there, ref;
  
  /* clear and init the dependency storage */
  for (slice_here = 0; slice_here < SLICE_MAX + 1; slice_here++) {
    memset(frame->slice[slice_here].immission_base, 0, sizeof(frame->slice[0].immission_base));
    frame->slice[slice_here].immission = frame->slice[slice_here].immission_base + REF_MAX;
  }
  
  if (!proc.sideband.propagation) return;
  
  for (slice_here = 0; slice_here < frame->slice_count; slice_here++) {
    while (1) {
      uint32_t in;
      float *out = (float *)(void *)&in;
      
      assert(sizeof(float) == sizeof(uint32_t));
      fread(buf, sizeof(buf[0]), 4, proc.sideband.propagation);
      in = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | ((uint32_t)buf[3] << 0);
      if (*out == 0.0) break;
      fread(buf, sizeof(buf[0]), 2, proc.sideband.propagation);
      ref = (int8_t)buf[0];
      slice_there = buf[1];
      frame->slice[slice_here].immission[ref][slice_there] = *out;
    }
  }
  fread(buf, sizeof(buf[0]), 4, proc.sideband.propagation);
  frame->reference_lifetime = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | ((uint32_t)buf[3] << 0);
}
#endif
