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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <graves/graves.h>

void
graves_det_destroy(graves_det_t *detect)
{
  if (detect->snr_hist != NULL)
    free(detect->snr_hist);

  if (detect->samp_hist != NULL)
    free(detect->samp_hist);

  su_iir_filt_finalize(&detect->lpf1);
  su_iir_filt_finalize(&detect->lpf2);

  grow_buf_clear(&detect->chirp);

  free(detect);
}

SUBOOL
graves_det_feed(graves_det_t *md, SUCOMPLEX x)
{
  SUCOMPLEX y;
  SUFLOAT   snr;
  SUFLOAT   energy;
  SUSCOUNT  start = 0;

  unsigned int i;

  y = su_iir_filt_feed(&md->lpf1, x * SU_C_CONJ(su_ncqo_read(&md->lo)));
  md->n0 += md->alpha * (SU_C_REAL(y * SU_C_CONJ(y)) - md->n0);

  y = su_iir_filt_feed(&md->lpf2, y);
  md->s0 += md->alpha * (SU_C_REAL(y * SU_C_CONJ(y)) - md->s0);

  snr = md->s0 / md->n0;

  /* Put SNR value */
  md->snr_hist[md->p] = snr;

  /* Keep demodulated sample in delay line */
  md->samp_hist[md->p] = y;

  if (++md->p == md->hist_len)
    md->p = 0;

  /* md->p now points to the OLDEST sample */

  /* Compute cross-correlation */
  energy = 0;
  for (i = 0; i < md->hist_len; ++i)
    energy += md->snr_hist[i];

  /* Detect chirp limits */
  if (md->in_chirp) {
    if (energy < md->energy_thres) {
      /* DETECTED: CHIRP END */
      md->in_chirp = SU_FALSE;
      start   = (SUSCOUNT) (SU_FLOOR(SU_ASFLOAT(md->n - start) / md->fs));

      SU_TRYCATCH(
          (md->on_chirp) (
              md->privdata,
              md->fs, start,
              (const SUCOMPLEX *) grow_buf_get_buffer(&md->chirp),
              grow_buf_get_size(&md->chirp) / sizeof(SUCOMPLEX),
              md->n0_start),
          return SU_FALSE);

#ifdef DEBUG
      printf(
          "Chirp of length %5d detected (at %02d:%02d:%02d)\n",
          grow_buf_get_size(&md->chirp) / sizeof(SUCOMPLEX),
          start / 3600,
          (start / 60) % 60,
          start % 60);
#endif
    } else {
      /* Sample belongs to chirp. Save it for later processing */
      SU_TRYCATCH(
          grow_buf_append(&md->chirp, &y, sizeof(SUCOMPLEX)) != -1,
          return SU_FALSE);
    }
  } else {
    if (energy >= md->energy_thres) {
      /* DETECTED: CHIRP START */
      md->in_chirp = SU_TRUE;
      md->n0_start = md->n0;

      /* Save all samples in the delay line to the grow buffer */
      grow_buf_shrink(&md->chirp);
      for (i = 0; i < md->hist_len; ++i) {
        SU_TRYCATCH(
            grow_buf_append(
                &md->chirp,
                md->samp_hist + (i + md->p) % md->hist_len,
                sizeof(SUCOMPLEX)) != -1,
            return SU_FALSE);
      }
    }
  }

  ++md->n;
  return SU_TRUE;
}

void
graves_det_set_center_freq(graves_det_t *md, SUFLOAT fc)
{
  su_ncqo_set_freq(&md->lo, SU_ABS2NORM_FREQ(md->fs, fc));
}

graves_det_t *
graves_det_new(SUFLOAT fs, SUFLOAT fc, graves_chirp_cb_t chrp_fn, void *privdata)
{
  graves_det_t *new = NULL;

  SU_TRYCATCH(new = calloc(1, sizeof (graves_det_t)), goto fail);

  new->fs = fs;
  new->alpha = 1 - SU_EXP(-SU_ADDSFX(1.) / (fs * MIN_CHIRP_DURATION));
  new->on_chirp = chrp_fn;
  new->privdata = privdata;

  su_ncqo_init(&new->lo, SU_ABS2NORM_FREQ(fs, fc));

  SU_TRYCATCH(
      su_iir_bwlpf_init(&new->lpf1, 5, SU_ABS2NORM_FREQ(fs, LPF1_CUTOFF)),
      goto fail);

  SU_TRYCATCH(
      su_iir_bwlpf_init(&new->lpf2, 4, SU_ABS2NORM_FREQ(fs, LPF2_CUTOFF)),
      goto fail);

  new->hist_len = (SUSCOUNT) (SU_CEIL(fs * MIN_CHIRP_DURATION));
  new->energy_thres = GRAVES_POWER_THRESHOLD * new->hist_len;

  SU_TRYCATCH(
      new->snr_hist   = calloc(sizeof(SUFLOAT), new->hist_len),
      goto fail);

  SU_TRYCATCH(
      new->samp_hist = calloc(sizeof(SUCOMPLEX), new->hist_len),
      goto fail);

  return new;

fail:
  if (new != NULL)
    graves_det_destroy(new);

  return new;
}
