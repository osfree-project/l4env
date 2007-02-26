/*
 * Copyright (C) 2006 Michael Roitzsch <mroi@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 */

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>

#include "ssim.h"

#if defined(__SSE__)
#include <xmmintrin.h>
#elif defined(__ALTIVEC__)
#include <altivec.h>
#endif
#ifdef __GNUC__
#define PREFETCH(x) __builtin_prefetch(x);
#endif
#define HORIZ_ADD(x)  (x[0] + x[1] + x[2] + x[3])

#define BLOCK_WIDTH  8
#define BLOCK_HEIGHT 8
#define PIXEL_STRIDE 1

#define N (BLOCK_WIDTH * BLOCK_HEIGHT)

#define K1 0.01
#define K2 0.03
#define L 255.0

#define C1 ((K1 * L) * (K1 * L))
#define C2 ((K2 * L) * (K2 * L))

#define WEIGHT_Y  0.8
#define WEIGHT_CB 0.1
#define WEIGHT_CR 0.1


static double ssim_block(const uint8_t * restrict x, const uint8_t * restrict y,
  const unsigned line_stride_x, const unsigned line_stride_y)
{
#if defined(__SSE__)
  __m128 vec_mean_x   = { 0.0, 0.0, 0.0, 0.0 };
  __m128 vec_mean_y   = { 0.0, 0.0, 0.0, 0.0 };
  __m128 vec_var_x    = { 0.0, 0.0, 0.0, 0.0 };
  __m128 vec_var_y    = { 0.0, 0.0, 0.0, 0.0 };
  __m128 vec_covar_xy = { 0.0, 0.0, 0.0, 0.0 };
  __m128 vec_x;
  __m128 vec_y;
  float unpack[4] __attribute__((aligned(16)));
#elif defined(__ALTIVEC__)
  vector float vec_mean_x   = { 0.0, 0.0, 0.0, 0.0 };
  vector float vec_mean_y   = { 0.0, 0.0, 0.0, 0.0 };
  vector float vec_var_x    = { 0.0, 0.0, 0.0, 0.0 };
  vector float vec_var_y    = { 0.0, 0.0, 0.0, 0.0 };
  vector float vec_covar_xy = { 0.0, 0.0, 0.0, 0.0 };
  vector float vec_x;
  vector float vec_y;
  float unpack[4] __attribute__((aligned(16)));
#endif
  double mean_x   = 0.0;
  double mean_y   = 0.0;
  double var_x    = 0.0;
  double var_y    = 0.0;
  double covar_xy = 0.0;
  unsigned i, j;
  
#if defined(__SSE__)
  assert(BLOCK_WIDTH % 4 == 0);
  
  for (i = 0; i < BLOCK_HEIGHT; i++) {
    PREFETCH(x + i * line_stride_x);
    PREFETCH(y + i * line_stride_y);
    for (j = 0; j < BLOCK_WIDTH; j += 4) {
      unpack[0] = (float)x[i * line_stride_x + (j + 0) * PIXEL_STRIDE];
      unpack[1] = (float)x[i * line_stride_x + (j + 1) * PIXEL_STRIDE];
      unpack[2] = (float)x[i * line_stride_x + (j + 2) * PIXEL_STRIDE];
      unpack[3] = (float)x[i * line_stride_x + (j + 3) * PIXEL_STRIDE];
      vec_x = _mm_load_ps(unpack);
      unpack[0] = (float)y[i * line_stride_y + (j + 0) * PIXEL_STRIDE];
      unpack[1] = (float)y[i * line_stride_y + (j + 1) * PIXEL_STRIDE];
      unpack[2] = (float)y[i * line_stride_y + (j + 2) * PIXEL_STRIDE];
      unpack[3] = (float)y[i * line_stride_y + (j + 3) * PIXEL_STRIDE];
      vec_y = _mm_load_ps(unpack);
      vec_mean_x   = _mm_add_ps(vec_x, vec_mean_x);
      vec_mean_y   = _mm_add_ps(vec_y, vec_mean_y);
      vec_var_x    = _mm_add_ps(_mm_mul_ps(vec_x, vec_x), vec_var_x);
      vec_var_y    = _mm_add_ps(_mm_mul_ps(vec_y, vec_y), vec_var_y);
      vec_covar_xy = _mm_add_ps(_mm_mul_ps(vec_x, vec_y), vec_covar_xy);
    }
  }
  _mm_store_ps(unpack, vec_mean_x);
  mean_x = HORIZ_ADD(unpack);
  _mm_store_ps(unpack, vec_mean_y);
  mean_y = HORIZ_ADD(unpack);
  _mm_store_ps(unpack, vec_var_x);
  var_x = HORIZ_ADD(unpack);
  _mm_store_ps(unpack, vec_var_y);
  var_y = HORIZ_ADD(unpack);
  _mm_store_ps(unpack, vec_covar_xy);
  covar_xy = HORIZ_ADD(unpack);
#elif defined(__ALTIVEC__)
  assert(BLOCK_WIDTH % 4 == 0);
  
  for (i = 0; i < BLOCK_HEIGHT; i++) {
    PREFETCH(x + i * line_stride_x);
    PREFETCH(y + i * line_stride_y);
    for (j = 0; j < BLOCK_WIDTH; j += 4) {
      unpack[0] = (float)x[i * line_stride_x + (j + 0) * PIXEL_STRIDE];
      unpack[1] = (float)x[i * line_stride_x + (j + 1) * PIXEL_STRIDE];
      unpack[2] = (float)x[i * line_stride_x + (j + 2) * PIXEL_STRIDE];
      unpack[3] = (float)x[i * line_stride_x + (j + 3) * PIXEL_STRIDE];
      vec_x = vec_ld(0, unpack);
      unpack[0] = (float)y[i * line_stride_y + (j + 0) * PIXEL_STRIDE];
      unpack[1] = (float)y[i * line_stride_y + (j + 1) * PIXEL_STRIDE];
      unpack[2] = (float)y[i * line_stride_y + (j + 2) * PIXEL_STRIDE];
      unpack[3] = (float)y[i * line_stride_y + (j + 3) * PIXEL_STRIDE];
      vec_y = vec_ld(0, unpack);
      vec_mean_x   = vec_add(vec_x, vec_mean_x);
      vec_mean_y   = vec_add(vec_y, vec_mean_y);
      vec_var_x    = vec_madd(vec_x, vec_x, vec_var_x);
      vec_var_y    = vec_madd(vec_y, vec_y, vec_var_y);
      vec_covar_xy = vec_madd(vec_x, vec_y, vec_covar_xy);
    }
  }
  vec_st(vec_mean_x, 0, unpack);
  mean_x = HORIZ_ADD(unpack);
  vec_st(vec_mean_y, 0, unpack);
  mean_y = HORIZ_ADD(unpack);
  vec_st(vec_var_x, 0, unpack);
  var_x = HORIZ_ADD(unpack);
  vec_st(vec_var_y, 0, unpack);
  var_y = HORIZ_ADD(unpack);
  vec_st(vec_covar_xy, 0, unpack);
  covar_xy = HORIZ_ADD(unpack);
#else
  for (i = 0; i < BLOCK_HEIGHT; i++) {
    PREFETCH(x + i * line_stride_x);
    PREFETCH(y + i * line_stride_y);
    for (j = 0; j < BLOCK_WIDTH; j++) {
      const float x_ij = (float)x[i * line_stride_x + j * PIXEL_STRIDE];
      const float y_ij = (float)y[i * line_stride_y + j * PIXEL_STRIDE];
      mean_x   += x_ij;
      mean_y   += y_ij;
      var_x    += x_ij * x_ij;
      var_y    += y_ij * y_ij;
      covar_xy += x_ij * y_ij;
    }
  }
#endif
  
  mean_x /= N;
  mean_y /= N;
  var_x    = (var_x    / (N-1)) - (N / (N-1) * mean_x * mean_x);
  var_y    = (var_y    / (N-1)) - (N / (N-1) * mean_y * mean_y);
  covar_xy = (covar_xy / (N-1)) - (N / (N-1) * mean_x * mean_y);
  
  return ((2 * mean_x * mean_y + C1) * (2 * covar_xy + C2)) /
    ((mean_x*mean_x + mean_y*mean_y + C1) * (var_x + var_y + C2));
}

void ssim_map(const uint8_t * restrict x, const uint8_t * restrict y,
  uint8_t * restrict map, float * restrict hist,
  const unsigned width, const unsigned height, const unsigned line_stride)
{
  double ssim;
  unsigned i, j;

  for (i = 0; i < height - BLOCK_HEIGHT; i++) {
    for (j = 0; j < width - BLOCK_WIDTH; j++) {
      ssim = 1 - ssim_block(&x[i * line_stride + j * PIXEL_STRIDE], &y[i * line_stride + j * PIXEL_STRIDE], line_stride, line_stride);
      if (hist)
	hist[(i + (BLOCK_HEIGHT / 2)) * width + (j + (BLOCK_WIDTH / 2)) * PIXEL_STRIDE] += ssim;
      if (ssim <= 0.0)
        map[(i + (BLOCK_HEIGHT / 2)) * line_stride + (j + (BLOCK_WIDTH / 2)) * PIXEL_STRIDE] = 0;
      else if (ssim >= 1.0)
        map[(i + (BLOCK_HEIGHT / 2)) * line_stride + (j + (BLOCK_WIDTH / 2)) * PIXEL_STRIDE] = 255;
      else
        map[(i + (BLOCK_HEIGHT / 2)) * line_stride + (j + (BLOCK_WIDTH / 2)) * PIXEL_STRIDE] = 255 * ssim;
    }
  }
}

float ssim_quality_loss(const picture_t * restrict x, const picture_t * restrict y,
  const change_rect_t * restrict rect, const float precision)
{
  double ssim = 0.0;
  unsigned i, j;
  
#ifdef SSIM_PRECISION_SWEEP
  if (precision == 0.0) return 0.0;
#endif
  
  /* Y */
  for (i = 0; i < x->height - BLOCK_HEIGHT; i++)
    if (!rect || (i + BLOCK_HEIGHT > rect->min_y && i < rect->max_y))
      for (j = 0; j < x->width - BLOCK_WIDTH; j++)
	if (!rect || (j + BLOCK_WIDTH  > rect->min_x && j < rect->max_x))
	  if (random() % RAND_MAX < precision * RAND_MAX)
	    ssim += WEIGHT_Y * (1.0 - ssim_block(&x->Y[i * x->line_stride_Y + j * PIXEL_STRIDE], &y->Y[i * y->line_stride_Y + j * PIXEL_STRIDE], x->line_stride_Y, y->line_stride_Y));
  
#if defined(SSIM_PRECISION_SWEEP) || defined(SSIM_PRECISION_FINAL)
  printf("%lf, %lf, ", precision, ssim / (WEIGHT_Y * x->width * x->height * precision));
#endif
  
  /* Cb / Cr */
  for (i = 0; i < x->height / 2 - BLOCK_HEIGHT; i++)
    if (!rect || (i + BLOCK_HEIGHT > rect->min_y / 2 && i <= rect->max_y / 2))
      for (j = 0; j < x->width / 2 - BLOCK_WIDTH; j++)
	if (!rect || (j + BLOCK_WIDTH > rect->min_x / 2 && j <= rect->max_x / 2))
	  if (random() % RAND_MAX < precision * RAND_MAX) {
	    /* due to subsampling, one chroma window is worth 4 luma windows, hence the factor 4 */
	    ssim += 4 * WEIGHT_CB * (1.0 - ssim_block(&x->Cb[i * x->line_stride_Cb + j * PIXEL_STRIDE], &y->Cb[i * y->line_stride_Cb + j * PIXEL_STRIDE], x->line_stride_Cb, y->line_stride_Cb));
	    ssim += 4 * WEIGHT_CR * (1.0 - ssim_block(&x->Cr[i * x->line_stride_Cr + j * PIXEL_STRIDE], &y->Cr[i * y->line_stride_Cr + j * PIXEL_STRIDE], x->line_stride_Cr, y->line_stride_Cr));
	  }
  
#if defined(SSIM_PRECISION_SWEEP) || defined(SSIM_PRECISION_FINAL)
  printf("%lf, ", ssim / (x->width * x->height * precision));
#endif
  
  return ssim / (x->width * x->height * precision);
}
