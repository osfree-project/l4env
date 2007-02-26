/*
 * Copyright (C) 2005 Michael Roitzsch <mroi@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef PREDICT_H
#define PREDICT_H

/* 
 * The operation of the llsp decoding time predictor is divided in two
 * phases: First, the predictor has to be trained with a mix of typical
 * videos. After that, predicted decoding times can be retrieved. The
 * same llsp handle can be used for both, even in parallel (that is:
 * predicting decoding times with previous coefficients and accumulating
 * samples for calculating new coefficients at the same time).
 */

/* an opaque handle for the llsp solver/predictor */
typedef struct llsp_s llsp_t;

/* Allocated a new llsp handle with the given id. The id is meant to
 * distinguish several prediction contexts and is typically used to
 * handle the different codecs or different frame types (I,P,B,S,...)
 * within the same codec, so use some unique codec/frame type number.
 * Count denotes the number of metrics values used in this context. */
llsp_t *llsp_new(int id, unsigned count);

/* This function is called during the training phase and adds another
 * tuple of (metrics, measured decoder time) to the training matrix
 * inside the given llsp solver. The metrics array must have as many
 * values as stated in count on llsp_new(). */
void llsp_accumulate(llsp_t *llsp, double *metrics, double time);

/* Finalizes the training phase for the given context and returns a
 * pointer to the resulting coefficients for inspection by the client
 * or NULL if the training phase could not be successfully finalized.
 * The pointer is valid until the llsp context is freed. */
/* FIXME: It should be possible to run this from a low priority
 * background thread. See FIXME's in the code. */
double *llsp_finalize(llsp_t *llsp);

/* Predicts the decoding time from the given metrics. The context has
 * to be populated with a set of prediction coefficients, either by
 * running llsp_load() or by a training phase with a successfully
 * finished llsp_finalize(). Otherwise, the resulting prediction is
 * undefined. */
double llsp_predict(llsp_t *llsp, double *metrics);

/* Populates the llsp context with previously stored prediction
 * coefficients. Returns a pointer to the coefficients for inspection
 * by the client or NULL, if no stored coefficients for the current
 * id and metrics count have been found. */
double *llsp_load(llsp_t *llsp, const char *filename);

/* Stores the current prediction coefficients. The context has to be
 * populated by either llsp_load() or by a training phase with a
 * successfully finished llsp_finalize(). Otherwise, the stored
 * coefficients are undefined. */
int llsp_store(llsp_t *llsp, const char *filename);

/* Frees the llsp context. */
void llsp_dispose(llsp_t *llsp);


/* 
 * The actual prediction functions for specific codecs follow.
 * Six functions per codec are used: The first expects the filenames
 * for the llsp_store() and llsp_load() functions (see above)
 * respectively. You can pass NULL or the empty string to disable
 * either feature.
 *
 * The actual prediction function takes a chunk of the compressed video
 * and returns a wallclock time prediction for the actual decoding
 * function until it emits its next frame (which is not necessarily the
 * next frame inside the stream, keyword: frame reordering) or a
 * negative value if no prediction is possible. The given length and
 * data pointer will be modified to reflect the amount of consumed bytes.
 *
 * The learning function assigns a measured decoding time for the chunk
 * of compressed video last passed to the prediction function. This is
 * only useful for the learning phase, if the previous call to the
 * prediction function did not handle more than one frame, so consider
 * this when using codecs with heavy frame reordering.
 *
 * The eval function calculates the prediction coefficients and stores
 * them on disk, if the respective file name has been given. The
 * predictor can still be used afterwards.
 *
 * The discontinue function should be called in between different videos
 * and will reset the internal metrics extraction, but will not delete
 * the accumulated metrics and decoding times. Use this to learn
 * decoding times from more than one video.
 *
 * The dispose function frees the prediction handle.
 *
 * A typical usage of the prediction functions looks like this:
 *
 * predictor_t *predictor = predict_<codec>_new(learn_file, predict_file);
 * [...]
 * while (!end_of_data) {
 *   [...]
 *   get_video_block(&data, &length);
 *   data_predict = data;
 *   length_predict = length;
 *   time = 0.0;
 *   // this will predict decoding time for the first frame inside the data chunk
 *   prediction = predict_<codec>(predictor, &data_predict, &length_predict);
 *   time -= current_time();
 *   frame = really_decode(&data, &length);
 *   time += current_time();
 *   predict_<codec>_learn(predictor, time);
 *   [...]
 * }
 * predict_<codec>_eval(predictor);
 * predict_<codec>_dispose(predictor);
 */

/* an opaque handle for the predictor */
typedef struct predictor_s predictor_t;

/* the various prediction context IDs */
enum {
  PREDICT_XVID_I, PREDICT_XVID_P, PREDICT_XVID_B, PREDICT_XVID_S, PREDICT_XVID_N,
  PREDICT_MPEG_I, PREDICT_MPEG_P, PREDICT_MPEG_B, PREDICT_MPEG_D
};

enum {
  PREDICT_XVID_BASE = PREDICT_XVID_I,
  PREDICT_MPEG_BASE = PREDICT_MPEG_I
};

/* XviD (MPEG-4 part 2) prediction */
predictor_t *predict_xvid_new(const char *learn_file, const char *predict_file);
double predict_xvid(predictor_t *predictor, unsigned char **data, unsigned *length);
void predict_xvid_learn(predictor_t *predictor, double time);
void predict_xvid_eval(predictor_t *predictor);
void predict_xvid_discontinue(predictor_t *predictor);
void predict_xvid_dispose(predictor_t *predictor);

/* MPEG-1/2 prediction */
predictor_t *predict_mpeg_new(const char *learn_file, const char *predict_file);
double predict_mpeg(predictor_t *predictor, unsigned char **data, unsigned *length);
void predict_mpeg_learn(predictor_t *predictor, double time);
void predict_mpeg_eval(predictor_t *predictor);
void predict_mpeg_discontinue(predictor_t *predictor);
void predict_mpeg_dispose(predictor_t *predictor);

#endif
