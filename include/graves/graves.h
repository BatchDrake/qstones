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

#define GRAVES_CENTER_FREQ 143050000.
#define SPEED_OF_LIGHT     3e8

#define LPF1_CUTOFF SU_ADDSFX(300.) /* In Hz */
#define LPF2_CUTOFF SU_ADDSFX(50.)  /* In Hz */
#define GRAVES_POWER_RATIO (LPF2_CUTOFF / LPF1_CUTOFF)
#define GRAVES_POWER_THRESHOLD (SU_ASFLOAT(2.) * GRAVES_POWER_RATIO)

#define MIN_CHIRP_DURATION SU_ADDSFX(0.07)

typedef SUBOOL (*graves_chirp_cb_t) (
    void *privdata,
    SUFLOAT fs,
    SUSCOUNT start,
    const SUCOMPLEX *data,
    SUSCOUNT len,
    SUFLOAT N0);

struct graves_det {
  SUFLOAT fs;          /* Sample rate */
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

void graves_det_destroy(graves_det_t *detect);

SUBOOL graves_det_feed(graves_det_t *md, SUCOMPLEX x);

graves_det_t *graves_det_new(
    SUFLOAT fs,
    SUFLOAT fc,
    graves_chirp_cb_t chrp_fn,
    void *privdata);

#ifdef __cplusplus
}
#endif


#endif /* _GRAVES_H */

