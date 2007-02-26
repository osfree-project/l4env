/*
 * Copyright (C) 2006 Michael Roitzsch <mroi@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 */

#include "process.h"
#include "mpegvideo.h"
#if defined(REPLACEMENT_VIS) || defined(FINAL_REPLACEMENT)
#include "digits.h"
#endif

#if REPLACEMENT || SIDEBAND_READ
struct replacement_node_s {
  /* position of this node in the tree */
  unsigned depth, index;
  /* block boundaries (start inclusive, end exclusive) in macroblock coordinates */
  unsigned start_x, start_y, end_x, end_y;
  /* positive: short term, negative: long term; invariant: never zero */
  int reference;
  /* offset of the replacement */
  int x, y;
  /* subnodes; invariant: either all four or none exist */
  replacement_node_t *node[4];
};

static int fill_coordinates(replacement_node_t *node);
#endif

#if REPLACEMENT || SLICE_SKIPPING || LLSP_TRAIN || defined(REPLACEMENT_TIME_MEASURE) || defined(FINAL_REPLACEMENT) || defined(LLSP_PREDICTION)
static inline const replacement_node_t *get_replacement_node(const replacement_node_t *node, const unsigned mb_x, const unsigned mb_y);
static float do_replacement(const AVCodecContext *c, const AVPicture *frame, int slice, const change_rect_t *rect);
static inline byte_block_t checkerboard(int mb_x, int mb_y)
{
  return ((mb_x & 1) ^ (mb_y & 1)) ? (byte_spread * 0xA0) : (byte_spread * 0x60);
}
#endif

#if REPLACEMENT

static int search_average_motion(const AVCodecContext *c, replacement_node_t *node);
static void cut_nodes(const AVCodecContext *c, replacement_node_t *node,
  const picture_t *original, const picture_t *replaced);

void search_replacements(const AVCodecContext *c, replacement_node_t *node)
{
  int i;
  
  if (!node) {
    /* this is the root node, which is empty on initial call; we need to set it up */
    node = proc.frame->replacement = (replacement_node_t *)av_malloc(sizeof(replacement_node_t));
    node->depth = 0;
    node->index = 0;
    node->node[0] = node->node[1] = node->node[2] = node->node[3] = NULL;
    if (!fill_coordinates(node) || !search_average_motion(c, node)) {
      av_free(node);
      proc.frame->replacement = NULL;
      return;
    }
  }
  
  /* now we subdivide the current area into four sub-areas */
  for (i = 0; i < 4; i++) {
    node->node[i] = (replacement_node_t *)av_malloc(sizeof(replacement_node_t));
    node->node[i]->depth = node->depth + 1;
    node->node[i]->index = 4 * node->index + i;
    node->node[i]->node[0] = node->node[i]->node[1] = node->node[i]->node[2] = node->node[i]->node[3] = NULL;
    if (!fill_coordinates(node->node[i]) || !search_average_motion(c, node->node[i])) {
      /* subdivision is not possible, revert and bail out */
      for (; i >= 0; i--)
	av_freep(&node->node[i]);
      break;
    }
  }
  if (node->node[0]) {
    search_replacements(c, node->node[0]);
    search_replacements(c, node->node[1]);
    search_replacements(c, node->node[2]);
    search_replacements(c, node->node[3]);
  }
  
  if (node->depth == 0) {
    picture_t original, replaced;
    
    original.Y  = c->frame.current->data[0];
    original.Cb = c->frame.current->data[1];
    original.Cr = c->frame.current->data[2];
    original.line_stride_Y  = c->frame.current->linesize[0];
    original.line_stride_Cb = c->frame.current->linesize[1];
    original.line_stride_Cr = c->frame.current->linesize[2];
    original.width  = c->width;
    original.height = c->height;
    
    replaced.Y  = proc.temp_frame.data[0];
    replaced.Cb = proc.temp_frame.data[1];
    replaced.Cr = proc.temp_frame.data[2];
    replaced.line_stride_Y  = proc.temp_frame.linesize[0];
    replaced.line_stride_Cb = proc.temp_frame.linesize[1];
    replaced.line_stride_Cr = proc.temp_frame.linesize[2];
    replaced.width  = c->width;
    replaced.height = c->height;
    
    /* the quadtree is now fully subdivided, let's cut off some of the nodes bottom-up */
    do_replacement(c, &proc.temp_frame, SLICE_MAX, NULL);
    cut_nodes(c, node, &original, &replaced);
    
    /* calculate the error for each slice individually */
#if defined(SSIM_PRECISION_SWEEP) || defined(SSIM_PRECISION_FINAL)
    printf("\n");
#endif
    for (i = 0; i < proc.frame->slice_count; i++) {
      img_copy(&proc.temp_frame, (const AVPicture *)c->frame.current, PIX_FMT_YUV420P,
	proc.mb_width << mb_size_log, proc.mb_height << mb_size_log);
      do_replacement(c, &proc.temp_frame, i, NULL);
#if !defined(SSIM_PRECISION_SWEEP) && !defined(SSIM_PRECISION_FINAL)
      proc.frame->slice[i].direct_quality_loss =
	ssim_quality_loss(&original, &replaced, &proc.frame->slice[i].rect, ssim_precision);
#else
#ifdef SSIM_PRECISION_SWEEP
      /* prints precision, Y and YCbCr SSIM values for a random precision */
      ssim_quality_loss(&original, &replaced, &proc.frame->slice[i].rect, (float)(random() % RAND_MAX) / (float)RAND_MAX);
#endif
#ifdef SSIM_PRECISION_FINAL
      /* prints precision, Y and YCbCr SSIM values for final precision */
      ssim_quality_loss(&original, &replaced, &proc.frame->slice[i].rect, ssim_precision);
#endif
      /* the accurate values for comparision */
      proc.frame->slice[i].direct_quality_loss =
	ssim_quality_loss(&original, &replaced, &proc.frame->slice[i].rect, 1.0);
      /* finish the line properly */
      printf("1\n");
#endif
    }
  }
}

static int search_average_motion(const AVCodecContext *c, replacement_node_t *node)
{
  int i;
  unsigned mb_x, mb_y, list, count;
  int ref_count_base[2 * REF_MAX + 1] = { 0 };
  /* ref_count can be indexed from -REF_MAX to REF_MAX, which is the values range of ref_num */
  int *ref_count = ref_count_base + REF_MAX;
  int64_t x, y;
    
  /* Step 1: select the reference used most often */
  for (mb_y = node->start_y; mb_y < node->end_y; mb_y++) {
    for (mb_x = node->start_x; mb_x < node->end_x; mb_x++) {
      const int ref_stride = 2 * proc.mb_width;
      const int ref_index_base = 2*mb_x + 2*mb_y * ref_stride;
      for (list = 0; list < 2; list++) {
	for (i = 0; i < 4; i++) {
	  const int ref_index = ref_index_base + (i&1) + (i>>1) * ref_stride;
	  const int ref_num = proc.ref_num[list][ref_index];
	  ref_count[ref_num]++;
	}
      }
    }
  }
  count = 0;
  node->reference = 0;
  for (i = -REF_MAX; i <= REF_MAX; i++) {
    if (i && ref_count[i] > count) {
      count = ref_count[i];
      node->reference = i;
    }
  }
  if (!node->reference)
    /* no reference found */
    return 0;
  
  /* Step 2: calculate average motion vector */
  count = 0;
  x = y = 0;
  for (mb_y = node->start_y; mb_y < node->end_y; mb_y++) {
    for (mb_x = node->start_x; mb_x < node->end_x; mb_x++) {
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
	  if (proc.ref_num[list][ref_index] == node->reference) {
	    if ((IS_16X16(mb_type) && (i&15) == 0) ||
		(IS_16X8(mb_type) && (i&(15-8)) == 0) ||
		(IS_8X16(mb_type) && (i&(15-2)) == 0) ||
		(IS_8X8(mb_type) && (i&(15-8-2)) == 0)) {
	      /* FIXME: we cannot distinguish the 8x8, 8x4, 4x8 and 4x4 subtypes */
	      x += c->frame.current->motion_val[list][mv_index][0];
	      y += c->frame.current->motion_val[list][mv_index][1];
	      count++;
	    }
	  }
	}
      }
    }
  }
  /* finish averaging, the additional factor 4 is due to motion being quarter-pixel */
  node->x = x / (4 * count);
  node->y = y / (4 * count);
  
  return 1;
}

static void cut_nodes(const AVCodecContext *c, replacement_node_t *node,
  const picture_t *original, const picture_t *replaced)
{
  change_rect_t rect;
  float quality_loss1, quality_loss2;
  replacement_node_t *subnode[4];
  
  if (!node || !node->node[0])
    /* no subnodes -> nothing to cut here */
    return;
  /* try cutting subnodes first */
  cut_nodes(c, node->node[0], original, replaced);
  cut_nodes(c, node->node[1], original, replaced);
  cut_nodes(c, node->node[2], original, replaced);
  cut_nodes(c, node->node[3], original, replaced);
  if (node->node[0]->node[0] ||
      node->node[1]->node[0] ||
      node->node[2]->node[0] ||
      node->node[3]->node[0])
    /* subnodes were not fully cut off, don't cut this one either */
    return;
  
  /* we are in a node with subnodes that have already been cut,
   * now we try to cut off those subnodes and see what happens quality-wise */
  
  rect.min_x = node->start_x << mb_size_log;
  rect.min_y = node->start_y << mb_size_log;
  rect.max_x = node->end_x << mb_size_log;
  rect.max_y = node->end_y << mb_size_log;
  
  /* this is the current quality loss within the current node's area */
  quality_loss1 = ssim_quality_loss(original, replaced, &rect, ssim_precision);
  
  /* now cut off the subnodes */
  memcpy(subnode, node->node, sizeof(node->node));
  memset(node->node, 0, sizeof(node->node));
  do_replacement(c, &proc.temp_frame, SLICE_MAX, &rect);
  
  /* this is the quality loss with the subnodes removed */
  quality_loss2 = ssim_quality_loss(original, replaced, &rect, ssim_precision);
  
#ifdef REPLACEMENT_VIS
  printf("%d, %lf, %lf\n", node->depth, quality_loss1, quality_loss2);
#ifdef REPLACEMENT_VIS_THRESHOLD
#define subdivision_threshold REPLACEMENT_VIS_THRESHOLD
#endif
#endif
  if (quality_loss2 - quality_loss1 <= subdivision_threshold / (1 << (2 * node->depth))) {
    /* the increase in quality loss is adequate, we can delete the subnodes */
    destroy_replacement_tree(subnode[0]);
    destroy_replacement_tree(subnode[1]);
    destroy_replacement_tree(subnode[2]);
    destroy_replacement_tree(subnode[3]);
  } else {
    /* the quality dropped too much, let's reattach the subnodes */
    memcpy(node->node, subnode, sizeof(node->node));
    do_replacement(c, &proc.temp_frame, SLICE_MAX, &rect);
  }
}

#endif

#if REPLACEMENT || SIDEBAND_READ
void destroy_replacement_tree(replacement_node_t *node)
{
  if (!node) return;
  destroy_replacement_tree(node->node[0]);
  destroy_replacement_tree(node->node[1]);
  destroy_replacement_tree(node->node[2]);
  destroy_replacement_tree(node->node[3]);
  av_free(node);
}
#endif

#if defined(REPLACEMENT_VIS) || defined(FINAL_REPLACEMENT)

int frame_storage_alloc(AVCodecContext *c, AVFrame *frame)
{
  /* round width and height to integer macroblock multiples */
  const int width  = (2 * (c->width  + ((1 << mb_size_log) - 1)) >> mb_size_log) << mb_size_log;
  const int height = (2 * (c->height + ((1 << mb_size_log) - 1)) >> mb_size_log) << mb_size_log;
  frame->opaque = av_malloc(avpicture_get_size(c->pix_fmt, width, height));
  return avcodec_default_get_buffer(c, frame);
}

void frame_storage_destroy(AVCodecContext *c, AVFrame *frame)
{
  av_freep(&frame->opaque);
  avcodec_default_release_buffer(c, frame);
}

static void draw_slice_error(AVPicture *frame)
{
  int slice, mb, y, x;
  
  for (slice = 0; slice < proc.frame->slice_count; slice++) {
    for (mb = proc.frame->slice[slice].start_index; mb < proc.frame->slice[slice].end_index; mb++) {
      const int mb_x = mb % proc.mb_width;
      const int mb_y = mb / proc.mb_width;
      const replacement_node_t *const node = get_replacement_node(proc.frame->replacement, mb_x, mb_y);
      const byte_block_t error = byte_spread *
	((5 * proc.frame->slice_count * proc.frame->slice[slice].direct_quality_loss < 1.0) ?
	  (uint8_t)(0xFF * 5 * proc.frame->slice_count * proc.frame->slice[slice].direct_quality_loss) :
	  0xFF);
      int start_x = mb_x << mb_size_log;
      int start_y = mb_y << mb_size_log;
      int end_x = (mb_x + 1) << mb_size_log;
      int end_y = (mb_y + 1) << mb_size_log;
      /* Y */
      if (node)
	for (y = start_y; y < end_y; y++)
	  for (x = start_x; x < end_x; x += sizeof(byte_block_t))
	    BLOCK(frame->data[0], x + y * frame->linesize[0]) = error;
      else
	for (y = start_y; y < end_y; y++)
	  for (x = start_x; x < end_x; x += sizeof(byte_block_t))
	    BLOCK(frame->data[0], x + y * frame->linesize[0]) = checkerboard(mb_x, mb_y);
      start_x = mb_x << (mb_size_log - 1);
      start_y = mb_y << (mb_size_log - 1);
      end_x = (mb_x + 1) << (mb_size_log - 1);
      end_y = (mb_y + 1) << (mb_size_log - 1);
      /* Cb */
      for (y = start_y; y < end_y; y++)
	for (x = start_x; x < end_x; x += sizeof(byte_block_t))
	  BLOCK(frame->data[1], x + y * frame->linesize[1]) = byte_spread * 0x80;
      /* Cr */
      for (y = start_y; y < end_y; y++)
	for (x = start_x; x < end_x; x += sizeof(byte_block_t))
	  BLOCK(frame->data[2], x + y * frame->linesize[2]) = byte_spread * 0x80;
    }
    
    /* write slice number onto the image */
    print_number(frame, proc.frame->slice[slice].start_index, proc.frame->slice[slice].end_index);
  }
}

static void draw_border(const replacement_node_t *node, AVPicture *frame)
{
  int x, y;
  if (!node) return;
  if (node->node[0]) {
    draw_border(node->node[0], frame);
    draw_border(node->node[1], frame);
    draw_border(node->node[2], frame);
    draw_border(node->node[3], frame);
  } else {
    for (x = (node->start_x << mb_size_log); x < (node->end_x << mb_size_log); x++)
      frame->data[0][x + (node->start_y << mb_size_log) * frame->linesize[0]] ^= 0x80;
    for (y = (node->start_y << mb_size_log) + 1; y < (node->end_y << mb_size_log); y++)
      frame->data[0][(node->start_x << mb_size_log) + y * frame->linesize[0]] ^= 0x80;
  }
}

void replacement_visualize(const AVCodecContext *c)
{
  /* round width and height to integer macroblock multiples */
  const int width  = (2 * (c->width  + ((1 << mb_size_log) - 1)) >> mb_size_log) << mb_size_log;
  const int height = (2 * (c->height + ((1 << mb_size_log) - 1)) >> mb_size_log) << mb_size_log;
  AVPicture quad;
  
  if (c->metrics.type >= 0) {
    /* use per-frame storage to keep the replaced image */
    avpicture_fill(&quad, private_data(c->frame.current), c->pix_fmt, width, height);
    /* left upper quadrant: original */
    img_copy(&quad, (const AVPicture *)c->frame.current, PIX_FMT_YUV420P, c->width, c->height);
    /* right upper quadrant: replacement */
    quad.data[0] += c->width;
    quad.data[1] += c->width / 2;
    quad.data[2] += c->width / 2;
    do_replacement(c, &quad, SLICE_MAX, NULL);
    /* left lower quadrant: slice error map */
    quad.data[0] += (c->height    ) * quad.linesize[0] - (c->width    );
    quad.data[1] += (c->height / 2) * quad.linesize[1] - (c->width / 2);
    quad.data[2] += (c->height / 2) * quad.linesize[2] - (c->width / 2);
    draw_slice_error(&quad);
    /* right lower quadrant: replacement with borders */
    quad.data[0] += c->width;
    quad.data[1] += c->width / 2;
    quad.data[2] += c->width / 2;
    do_replacement(c, &quad, SLICE_MAX, NULL);
    draw_border(proc.frame->replacement, &quad);
  }
  if (c->metrics.type == PSEUDO_SLICE_FRAME_END && c->frame.display) {
    /* dump the current display image and its stored replacement to disk */
    int y;
    static FILE *vis = NULL;
    if (!vis) vis = fopen("Visualization.yuv", "w");
    avpicture_fill(&quad, private_data(c->frame.display), c->pix_fmt, width, height);
    /* Y */
    for (y = 0; y < 2 * c->height; y++)
      fwrite(quad.data[0] + y * quad.linesize[0], 1, 2 * c->width, vis);
    /* Cb */
    for (y = 0; y < c->height; y++)
      fwrite(quad.data[1] + y * quad.linesize[1], 1, c->width, vis);
    /* Cr */
    for (y = 0; y < c->height; y++)
      fwrite(quad.data[2] + y * quad.linesize[2], 1, c->width, vis);
  }
}

#endif

#if REPLACEMENT || PROPAGATION
void remember_slice_boundaries(const AVCodecContext *c)
{
  proc.frame->slice[proc.frame->slice_count].start_index = c->slice.start_index;
  proc.frame->slice[proc.frame->slice_count].end_index   = c->slice.end_index;
#if REPLACEMENT
  proc.frame->slice[proc.frame->slice_count].rect.min_y = ((c->slice.start_index) / proc.mb_width) << mb_size_log;
  proc.frame->slice[proc.frame->slice_count].rect.max_y = ((c->slice.end_index - 1) / proc.mb_width + 1) << mb_size_log;
  if (c->slice.start_index / proc.mb_width == (c->slice.end_index - 1) / proc.mb_width) {
    proc.frame->slice[proc.frame->slice_count].rect.min_x = ((c->slice.start_index) % proc.mb_width) << mb_size_log;
    proc.frame->slice[proc.frame->slice_count].rect.max_x = ((c->slice.end_index - 1) % proc.mb_width + 1) << mb_size_log;
  } else {
    proc.frame->slice[proc.frame->slice_count].rect.min_x = 0;
    proc.frame->slice[proc.frame->slice_count].rect.max_x = c->width;
  }
#endif
}

void remember_reference_frames(const AVCodecContext *c)
{
  int mb, list, i, j;
  int8_t translate[2][REF_MAX];
  
  /* prepare translation table from slice-local reference numbers to frame-global reference numbers
   * using the coded picture number to identify the frames */
  for (list = 0; list < 2; list++) {
    for (i = 0; i < REF_MAX; i++) {
      translate[list][i] = 0;
      if (i >= c->reference.count[list]) continue;
      for (j = 0; j < REF_MAX; j++) {
	if (j < c->reference.long_count &&
	    c->reference.list[list][i]->coded_picture_number == c->reference.long_list[j]->coded_picture_number)
	  translate[list][i] = -(j + 1);
	if (j < c->reference.short_count &&
	    c->reference.list[list][i]->coded_picture_number == c->reference.short_list[j]->coded_picture_number)
	  translate[list][i] = j + 1;
      }
    }
  }
  
  /* now use the translation table to convert from slice-local reference
   * numbers to global ones */
  for (mb = c->slice.start_index; mb < c->slice.end_index; mb++) {
    const int mb_x = mb % proc.mb_width;
    const int mb_y = mb / proc.mb_width;
    const int mb_stride = proc.mb_width + 1;
    const int mb_index = mb_x + mb_y * mb_stride;
    const int ref_stride = 2 * proc.mb_width;
    const int ref_index_base = 2*mb_x + 2*mb_y * ref_stride;
    for (list = 0; list < 2; list++) {
      for (i = 0; i < 4; i++) {
	const int ref_index = ref_index_base + (i&1) + (i>>1) * ref_stride;
	const int ref_num = c->frame.current->ref_index[list][ref_index];
	if (USES_LIST(c->frame.current->mb_type[mb_index], list) && ref_num >= 0)
	  proc.ref_num[list][ref_index] = translate[list][ref_num];
	else
	  proc.ref_num[list][ref_index] = 0;
      }
    }
  }
}
#endif

#if LLSP_TRAIN || defined(REPLACEMENT_TIME_MEASURE) || defined(LLSP_PREDICTION)
void replacement_time(AVCodecContext *c)
{
  int slice;
  
  for (slice = 0; slice < proc.frame->slice_count; slice++) {
#if LLSP_TRAIN || defined(LLSP_PREDICTION)
    double time;
#endif
    if (proc.frame->replacement) {
      img_copy(&proc.temp_frame, (const AVPicture *)c->frame.current, PIX_FMT_YUV420P, c->width, c->height);
#if LLSP_TRAIN || defined(LLSP_PREDICTION)
      time = -get_time();
#endif
#ifdef REPLACEMENT_TIME_MEASURE
      c->timing.total = 0;
      FFMPEG_TIME_START(c, total);
#endif
      do_replacement(c, &proc.temp_frame, slice, NULL);
#ifdef REPLACEMENT_TIME_MEASURE
      FFMPEG_TIME_STOP(c, total);
#endif
#if LLSP_TRAIN || defined(LLSP_PREDICTION)
      time += get_time();
#endif
#if LLSP_TRAIN
      llsp_accumulate(proc.llsp.replace, metrics_replace(proc.frame, slice), time);
#endif
#ifdef LLSP_PREDICTION
      printf("R: %lf, %lf\n", proc.frame->slice[slice].replacement_time, time);
#endif
#ifdef REPLACEMENT_TIME_MEASURE
      printf("%llu\n", c->timing.total);
#endif
    }
  }
}
#endif

#if REPLACEMENT || SLICE_SKIPPING || LLSP_TRAIN || defined(REPLACEMENT_TIME_MEASURE) || defined(FINAL_REPLACEMENT) || defined(LLSP_PREDICTION)

static inline const replacement_node_t *get_replacement_node(const replacement_node_t *node, const unsigned mb_x, const unsigned mb_y)
{
  int decision_x, decision_y;
  
  if (!node || !node->node[0]) return node;
  decision_x = (mb_x >= node->node[1]->start_x);
  decision_y = (mb_y >= node->node[2]->start_y);
  return get_replacement_node(node->node[(decision_y << 1) | decision_x], mb_x, mb_y);
}

static float do_replacement(const AVCodecContext *c, const AVPicture *frame, int slice, const change_rect_t *rect)
{
  int mb, x, y;
  double error = 0.0;
  const int width = proc.mb_width << mb_size_log;
  const int height = proc.mb_height << mb_size_log;
#if !REPLACEMENT
  int mb_add;
#endif
#if SLICE_SKIPPING || LLSP_TRAIN || defined(REPLACEMENT_TIME_MEASURE) || defined(LLSP_PREDICTION)
  int ref;
  typedef struct {
    uint8_t list;
    uint8_t num;
  } translate_t;
  translate_t translate_base[2 * REF_MAX + 1];
  translate_t *translate = translate_base + REF_MAX;
#endif
  
#if !REPLACEMENT
  assert(rect == NULL);
#endif
#if SLICE_SKIPPING || LLSP_TRAIN || defined(REPLACEMENT_TIME_MEASURE) || defined(LLSP_PREDICTION)
  /* create a backwards translation table from frame-global reference numbers to
   * slice-local reference numbers, again using the coded picture number to match frames */
  for (ref = -REF_MAX; ref <= REF_MAX; ref++) {
    int list, i, coded_picture_number = -1;
    translate[ref].list = 0;
    translate[ref].num  = 0;
    if (ref < 0 && -ref - 1 < c->reference.long_count)
      coded_picture_number = c->reference.long_list[-ref - 1]->coded_picture_number;
    if (ref > 0 && ref - 1 < c->reference.short_count)
      coded_picture_number = c->reference.short_list[ref - 1]->coded_picture_number;
    if (coded_picture_number < 0) continue;
    for (list = 0; list < 2; list++) {
      for (i = 0; i < c->reference.count[list]; i++) {
	if (coded_picture_number == c->reference.list[list][i]->coded_picture_number) {
	  translate[ref].list = list;
	  translate[ref].num  = i;
	  break;
	}
      }
      if (i < c->reference.count[list]) break;
    }
  }
#endif
  
  for (mb = proc.frame->slice[slice].start_index; mb < proc.frame->slice[slice].end_index; mb++) {
    const int mb_x = mb % proc.mb_width;
    const int mb_y = mb / proc.mb_width;
    int start_x = mb_x << mb_size_log;
    int start_y = mb_y << mb_size_log;
    int end_x = (mb_x + 1) << mb_size_log;
    int end_y = (mb_y + 1) << mb_size_log;
    
#if REPLACEMENT
    /* area of interest */
    if (!rect ||
	(end_x > rect->min_x && start_x < rect->max_x &&
	 end_y > rect->min_y && start_y < rect->max_y)) {
#endif
      const replacement_node_t *const restrict node = get_replacement_node(proc.frame->replacement, mb_x, mb_y);
      const AVFrame *const restrict replace =
	node ?
	  (((node->reference < 0) ? c->reference.long_list[-node->reference - 1] :
	  ((node->reference == 0) ? NULL :
	  ((node->reference > 0) ? c->reference.short_list[node->reference - 1] : NULL)))) :
	NULL;
      uint8_t * restrict target = frame->data[0];
      const uint8_t * restrict source = replace ? replace->data[0] : NULL;
      int stride1 = frame->linesize[0];
      int stride2 = replace ? replace->linesize[0] : 0;
      int dx = node ? node->x : 0;
      int dy = node ? node->y : 0;
      
#if !REPLACEMENT
      /* add macroblocks to this loop iteration as long as copying can happen en-bloc with no clipping */
      for (mb_add = 1; node; mb_add++) {
	if (mb_x + mb_add == node->end_x)
	  /* the node ends here */
	  break;
	if (start_x + dx < 0)
	  /* motion vector clipping in progress */
	  break;
	if (end_x + (mb_add << mb_size_log) + dx > width)
	  /* motion vector clipping starts here */
	  break;
	if (mb + mb_add == proc.frame->slice[slice].end_index)
	  /* slice ends here */
	  break;
      }
      /* when the loop ends, we have hit one of the conditions so we went one too far */
      mb_add--;
      end_x += (mb_add << mb_size_log);
      mb    += mb_add;
#endif
      
      /* motion clipping */
      if (start_x + dx < 0) dx = -start_x;
      if (start_y + dy < 0) dy = -start_y;
      if (end_x + dx > width ) dx = width  - end_x;
      if (end_y + dy > height) dy = height - end_y;
      
      /* Y */
      if (replace)
	for (y = start_y; y < end_y; y++)
	  for (x = start_x; x < end_x; x += sizeof(byte_block_t))
	    BLOCK(target, x + y * stride1) = BLOCK(source, (x+dx) + (y+dy) * stride2);
      else
	for (y = start_y; y < end_y; y++)
	  for (x = start_x; x < end_x; x += sizeof(byte_block_t))
	    BLOCK(target, x + y * stride1) = checkerboard(mb_x, mb_y);
      
#if SLICE_TRACKING
      /* now, before we modify the values for Cb and Cr copying,
       * we iterate over all macroblocks to calculate error immission from the reference in use;
       * we should iterate over all pixels, but for speed reasons, we use a center pixel of each macroblock only */
      for (y = start_y + (1 << (mb_size_log - 1)); y < end_y; y += (1 << mb_size_log)) {
	const frame_node_t *reference = private_data(replace);
	for (x = start_x + (1 << (mb_size_log - 1)); x < end_x; x += (1 << mb_size_log)) {
	  const int mb_index = ((x+dx) >> mb_size_log) + ((y+dy) >> mb_size_log) * proc.mb_width;
	  int slice;
	  for (slice = 0; slice < reference->slice_count; slice++) {
	    if (mb_index >= reference->slice[slice].start_index && mb_index < reference->slice[slice].end_index) {
	      /* account the error of the reference weighted by its size:
	       * we are copying one macroblock out of end_index-start_index */
	      const int slice_mbs = reference->slice[slice].end_index - reference->slice[slice].start_index;
	      error += reference->slice[slice].estimated_quality_loss / slice_mbs;
	      break;
	    }
	  }
	}
      }
#endif
      
      start_x = mb_x << (mb_size_log - 1);
      start_y = mb_y << (mb_size_log - 1);
#if REPLACEMENT
      end_x = (mb_x + 1) << (mb_size_log - 1);
#else
      end_x = (mb_x + mb_add + 1) << (mb_size_log - 1);
#endif
      end_y = (mb_y + 1) << (mb_size_log - 1);
      target = frame->data[1];
      source = replace ? replace->data[1] : NULL;
      stride1 = frame->linesize[1];
      stride2 = replace ? replace->linesize[1] : 0;
      dx >>= 1;
      dy >>= 1;
      
      /* Cb */
      if (replace)
	for (y = start_y; y < end_y; y++)
	  for (x = start_x; x < end_x; x += sizeof(byte_block_t))
	    BLOCK(target, x + y * stride1) = BLOCK(source, (x+dx) + (y+dy) * stride2);
      else
	for (y = start_y; y < end_y; y++)
	  for (x = start_x; x < end_x; x += sizeof(byte_block_t))
	    BLOCK(target, x + y * stride1) = byte_spread * 0x80;
      
      target = frame->data[2];
      source = replace ? replace->data[2] : NULL;
      stride1 = frame->linesize[2];
      stride2 = replace ? replace->linesize[2] : 0;
      
      /* Cr */
      if (replace)
	for (y = start_y; y < end_y; y++)
	  for (x = start_x; x < end_x; x += sizeof(byte_block_t))
	    BLOCK(target, x + y * stride1) = BLOCK(source, (x+dx) + (y+dy) * stride2);
      else
	for (y = start_y; y < end_y; y++)
	  for (x = start_x; x < end_x; x += sizeof(byte_block_t))
	    BLOCK(target, x + y * stride1) = byte_spread * 0x80;
#if REPLACEMENT
    }
#endif
    
#if SLICE_SKIPPING || LLSP_TRAIN || defined(REPLACEMENT_TIME_MEASURE) || defined(LLSP_PREDICTION)
    /* in addition to replacing the actual content of the slice, we also need to
     * synthesize some metadata (namely macroblock type, reference index and motion vector)
     * which FFmpeg might use for direct coded macroblocks of future frames */
    for (x = mb_x; x < mb_x + mb_add + 1; x++) {
      const int mb_stride = proc.mb_width + 1;
      const int mb_index = x + mb_y * mb_stride;
      const int ref_stride = 2 * proc.mb_width;
      const int ref_index_base = 2*x + 2*mb_y * ref_stride;
      const int mv_sample_log2 = 4 - c->frame.current->motion_subsample_log2;
      const int mv_stride = proc.mb_width << mv_sample_log2;
      const int mv_index_base = (x << mv_sample_log2) + (mb_y << mv_sample_log2) * mv_stride;
      const int list = translate[node->reference].list;
      int i;
#if SLICE_SKIPPING
      c->frame.current->mb_type[mb_index] = MB_TYPE_16x16 | (list ? MB_TYPE_L1 : MB_TYPE_L0);
#else
      /* dummy operation to get the time measurements somewhat correct */
      c->frame.current->error[mb_index % 4] += MB_TYPE_16x16 | (list ? MB_TYPE_L1 : MB_TYPE_L0);
#endif
      for (i = 0; i < 4; i++) {
	const int ref_index = ref_index_base + (i&1) + (i>>1) * ref_stride;
#if SLICE_SKIPPING
	c->frame.current->ref_index[list][ref_index] = translate[node->reference].num;
#else
	/* dummy operation to get the time measurements somewhat correct */
	c->frame.current->error[ref_index % 4] += translate[node->reference].num;
#endif
      }
      for (i = 0; i < 16; i++) {
	const int mv_index = mv_index_base + (i&3) + (i>>2) * mv_stride;
#if SLICE_SKIPPING
	c->frame.current->motion_val[list][mv_index][0] = node->x * 4;
	c->frame.current->motion_val[list][mv_index][1] = node->y * 4;
#else
	/* dummy operation to get the time measurements somewhat correct */
	c->frame.current->error[mv_index % 4] += node->x * 4 + node->y * 4;
#endif
      }
    }
#endif
  }
  
  return error;
}

#endif

#if SLICE_SKIPPING
int check_skip_next_slice(const AVCodecContext *c)
{
  static int local_slice_count = 0;
  static int skip = 0;
  
  /* handle replacement and error propagation of the PREVIOUS slice */
  if (c->metrics.type != PSEUDO_SLICE_FRAME_START) {
    /* skipping cannot have happened before the first slice */
    if (skip) {
      /* the last slice has been skipped, let's do the replacement */
      float immission;
      immission = do_replacement(c, (const AVPicture *)c->frame.current, local_slice_count, NULL);
      /* replacement finished */
      skip = 0;
#if SLICE_TRACKING
      proc.frame->slice[local_slice_count].estimated_quality_loss =
        immission + proc.frame->slice[local_slice_count].direct_quality_loss;
    } else {
      proc.frame->slice[local_slice_count].estimated_quality_loss = 0.0;
      /* scan remembered propagation paths and accumulate error */
      int ref, slice;
      for (ref = -REF_MAX; ref <= REF_MAX; ref++) {
	for (slice = 0; slice < SLICE_MAX; slice++) {
	  const float immission_factor = proc.frame->slice[local_slice_count].immission[ref][slice];
	  if (immission_factor > 0.0) {
	    const frame_node_t *node = private_data(
	      ((ref >  0) ? c->reference.short_list[ref - 1] :
	      ((ref == 0) ? c->frame.current :
	      ((ref <  0) ? c->reference.long_list[-ref - 1] : NULL))));
	    const float immitted_quality_loss = node->slice[slice].estimated_quality_loss;
	    proc.frame->slice[local_slice_count].estimated_quality_loss +=
	      immitted_quality_loss * immission_factor;
	  }
	}
      }
#endif
    }
  }
  
  if (c->metrics.type != PSEUDO_SLICE_FRAME_START) {
    /* update local_slice_count to the number of the NEXT slice */
    if (c->slice.flag_last)
      local_slice_count = 0;
    else
      local_slice_count++;
  }
  
  /* check for skipping of the NEXT slice */
  if (!c->slice.flag_last) {
    /* there is nothing to skip after the last slice */
    if (schedule_skip(c, local_slice_count))
      /* we skip the next slice, the replacement will be done when we meet here again before the next slice */
      /* skip that many macroblocks */
      skip = proc.frame->slice[local_slice_count].end_index - proc.frame->slice[local_slice_count].start_index;
  }
  
#if METRICS_EXTRACT || REPLACEMENT || PROPAGATION
  /* do not skip for real yet, just don't keep and cross-slice state */
  return 0;
#else
  /* actually advise FFmpeg to drop the decoding of the upcoming slice */
  return skip;
#endif
}
#endif

#if SIDEBAND_WRITE && (REPLACEMENT || SIDEBAND_READ)
void write_replacement_tree(const replacement_node_t *node)
{
  if (!node) {
    /* this must be the root node, so treat as a special case, since we must write something */
    nalu_write_uint16(0);
    return;
  }
  if (!node->node[0]) {
    nalu_write_uint8(node->depth);
    nalu_write_int8(node->reference);
    nalu_write_int16(node->x);
    nalu_write_int16(node->y);
  } else {
    write_replacement_tree(node->node[0]);
    write_replacement_tree(node->node[1]);
    write_replacement_tree(node->node[2]);
    write_replacement_tree(node->node[3]);
  }
}
#endif

#if REPLACEMENT || SIDEBAND_READ
/* quadtree coordinate calculation, actually it's more like bit shuffling */
static inline unsigned index_to_x(unsigned i)
{
  return
  ((i & (1 <<  0)) >>  0) |
  ((i & (1 <<  2)) >>  1) |
  ((i & (1 <<  4)) >>  2) |
  ((i & (1 <<  6)) >>  3) |
  ((i & (1 <<  8)) >>  4) |
  ((i & (1 << 10)) >>  5) |
  ((i & (1 << 12)) >>  6) |
  ((i & (1 << 14)) >>  7) |
  ((i & (1 << 16)) >>  8) |
  ((i & (1 << 18)) >>  9) |
  ((i & (1 << 20)) >> 10) |
  ((i & (1 << 22)) >> 11) |
  ((i & (1 << 24)) >> 12) |
  ((i & (1 << 26)) >> 13) |
  ((i & (1 << 28)) >> 14) |
  ((i & (1 << 30)) >> 15);
}
static inline unsigned index_to_y(unsigned i)
{
  return
  ((i & (1 <<  1)) >>  1) |
  ((i & (1 <<  3)) >>  2) |
  ((i & (1 <<  5)) >>  3) |
  ((i & (1 <<  7)) >>  4) |
  ((i & (1 <<  9)) >>  5) |
  ((i & (1 << 11)) >>  6) |
  ((i & (1 << 13)) >>  7) |
  ((i & (1 << 15)) >>  8) |
  ((i & (1 << 17)) >>  9) |
  ((i & (1 << 19)) >> 10) |
  ((i & (1 << 21)) >> 11) |
  ((i & (1 << 23)) >> 12) |
  ((i & (1 << 25)) >> 13) |
  ((i & (1 << 27)) >> 14) |
  ((i & (1 << 29)) >> 15) |
  ((i & (1 << 31)) >> 16);
}
static inline unsigned block_to_mb(unsigned block_coord, unsigned edge_length, unsigned depth)
{
  return (edge_length * block_coord) / (1 << depth);
}
static int fill_coordinates(replacement_node_t *node)
{
  const unsigned x = index_to_x(node->index);
  const unsigned y = index_to_y(node->index);
  node->start_x = block_to_mb(x    , proc.mb_width , node->depth);
  node->start_y = block_to_mb(y    , proc.mb_height, node->depth);
  node->end_x   = block_to_mb(x + 1, proc.mb_width , node->depth);
  node->end_y   = block_to_mb(y + 1, proc.mb_height, node->depth);
  return (node->start_x < node->end_x && node->start_y < node->end_y);
}
#endif

#if SIDEBAND_READ
void read_replacement_tree(replacement_node_t *node)
{
  static const int read_next_depth = -1;
  static int depth;
  
  if (!node) {
    /* initialize the root node */
    node = (replacement_node_t *)av_malloc(sizeof(replacement_node_t));
    proc.lookahead->replacement = node;
    node->depth = 0;
    node->index = 0;
    node->node[0] = node->node[1] = node->node[2] = node->node[3] = NULL;
    fill_coordinates(node);
    depth = read_next_depth;
  }
  
  while (1) {
    if (node->node[0] && node->node[1] && node->node[2] && node->node[3])
      /* nothing left to do here, this subtree is finished */
      return;
    if (depth == read_next_depth)
      depth = nalu_read_uint8();
    if (depth == node->depth) {
      /* this is our node */
      node->reference = nalu_read_int8();
      if (!node->reference) {
	/* special case: empty root node */
	av_free(node);
	proc.lookahead->replacement = NULL;
	return;
      }
      node->x = nalu_read_int16();
      node->y = nalu_read_int16();
      depth = read_next_depth;
      return;
    }
    if (depth > node->depth) {
      /* the next node is a subnode of the current one */
      int i;
      for (i = 0; i < 4; i++) {
	if (!node->node[i]) {
	  node->node[i] = (replacement_node_t *)av_malloc(sizeof(replacement_node_t));
	  node->node[i]->depth = node->depth + 1;
	  node->node[i]->index = node->index * 4 + i;
	  node->node[i]->node[0] = node->node[i]->node[1] = node->node[i]->node[2] = node->node[i]->node[3] = NULL;
	  fill_coordinates(node->node[i]);
	  read_replacement_tree(node->node[i]);
	  break;
	}
      }
    } else {
      return;
    }
  }
}
#endif
