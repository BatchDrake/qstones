/*

  Copyright (C) 2018 Gonzalo José Carracedo Carballal

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

#ifndef GRAVES_GRAVES_H
#define GRAVES_GRAVES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <util.h>

#include <sigutils/iir.h>
#include <sigutils/ncqo.h>
#include <sigutils/log.h>
#include <sigutils/sampling.h>

#define GRAVES_CENTER_FREQ SU_ADDSFX(143050000.)
#define SPEED_OF_LIGHT     SU_ADDSFX(3e8)

#if 0
#define LPF1_CUTOFF SU_ADDSFX(300.) /* In Hz */
#define LPF2_CUTOFF SU_ADDSFX(50.)  /* In Hz */
#define GRAVES_POWER_RATIO (LPF2_CUTOFF / LPF1_CUTOFF)
#define GRAVES_POWER_THRESHOLD (SU_ASFLOAT(2.) * GRAVES_POWER_RATIO)
#endif

#define GRAVES_MIN_LPF_CUTOFF SU_ABS2NORM_FREQ(8000, 50)

#define MIN_CHIRP_DURATION SU_ADDSFX(0.07)

typedef SUBOOL (*graves_chirp_cb_t) (
    void *privdata,
    SUSCOUNT start,
    const SUCOMPLEX *data,
    SUSCOUNT len,
    SUFLOAT N0);

struct graves_det_params {
  SUFLOAT fs;
  SUFLOAT fc;
  SUFLOAT lpf1;
  SUFLOAT lpf2;
  SUFLOAT threshold;
};

#define graves_det_params_INITIALIZER \
{                                     \
  SU_ADDSFX(8000.), /* fs */          \
  SU_ADDSFX(1000.), /* fc */          \
  SU_ADDSFX(300.),  /* lpf1 */        \
  SU_ADDSFX(50.),   /* lpf2 */        \
  SU_ADDSFX(2.),    /* threshoid */   \
}

struct graves_det {
  struct graves_det_params params;
  SUFLOAT ratio;
  SUSCOUNT n;          /* Samples consumed */
  su_iir_filt_t lpf1; /* Low pass filter 1. Used to detect noise power */
  su_iir_filt_t lpf2; /* Low pass filter 2. Used to isolate chirps */
  su_ncqo_t lo;
  SUFLOAT alpha;
  SUFLOAT n0; /* Noise power */
  SUFLOAT s0; /* Signal power */

  SUSCOUNT hist_len;
  SUSCOUNT p;
  SUFLOAT *snr_hist;
  SUCOMPLEX *samp_hist;

  SUFLOAT   n0_start;
  SUFLOAT   energy_thres;
  SUBOOL    in_chirp;

  grow_buf_t chirp;

  void *privdata;

  graves_chirp_cb_t on_chirp;
};

typedef struct graves_det graves_det_t;

SUINLINE SUFLOAT
graves_det_get_ratio(const graves_det_t *det)
{
  return det->ratio;
}

SUINLINE const struct graves_det_params *
graves_det_get_params(const graves_det_t *det)
{
  return &det->params;
}

void graves_det_destroy(graves_det_t *detect);

void graves_det_set_center_freq(graves_det_t *md, SUFLOAT fc);

SUBOOL graves_det_feed(graves_det_t *md, SUCOMPLEX x);

graves_det_t *
graves_det_new(
    const struct graves_det_params *params,
    graves_chirp_cb_t chrp_fn,
    void *privdata);

#ifdef __cplusplus
}
#endif


#endif /* _GRAVES_H */

