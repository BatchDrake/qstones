//
//    EchoDetector.cpp: Graves echo detector C++ wrapper
//    Copyright (C) 2018 Gonzalo José Carracedo Carballal
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Lesser General Public License as
//    published by the Free Software Foundation, either version 3 of the
//    License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public
//    License along with this program.  If not, see
//    <http://www.gnu.org/licenses/>
//

#include <Suscan/Compat.h>

#include "EchoDetector.h"

Q_DECLARE_METATYPE(QStones::EchoDetector::Chirp);

using namespace QStones;

// Chirp processing
void
EchoDetector::Chirp::process(void)
{
  unsigned long i, len;
  SUCOMPLEX prev = 0;
  SUFLOAT offset, thisDoppler;
  SUFLOAT dopplerSum = 0;
  SUFLOAT snrSum = 0;
  SUFLOAT K;

  len = this->samples.size();

  this->doppler.resize(len);
  this->softDoppler.resize(len);

  K = this->fs * SU_ADDSFX(.25) * SPEED_OF_LIGHT /
      (GRAVES_CENTER_FREQ * SU_ADDSFX(M_PI));

  for (i = 0; i < len; ++i) {
    // Get immediate offset
    offset = SU_C_ARG(this->samples[i] * SU_C_CONJ(prev));

    // Compute Doppler shift
    thisDoppler = K * offset;
    this->doppler[i]  = thisDoppler;

    dopplerSum += this->snr[i] * thisDoppler; // Weight by the SNR
    snrSum     += this->snr[i]; // Compute accumulated SNR

    prev = this->samples[i];
  }

  this->meanSNR     = snrSum / len;
  this->meanDoppler = dopplerSum / snrSum;
  this->duration    = len / this->fs;

  this->processed   = true;
}

EchoDetector::Chirp::Chirp(void) { } // Dumb constructor

static void
copyCommonToChirp(EchoDetector::Chirp *dest, const EchoDetector::Chirp &prev)
{
  dest->start         = prev.start;
  dest->startDecimal  = prev.startDecimal;
  dest->Rbw           = prev.Rbw;
  dest->fs            = prev.fs;

  // Processed members
  dest->processed     = prev.processed;
  if (dest->processed) {
    dest->meanSNR     = prev.meanSNR;
    dest->meanDoppler = prev.meanDoppler;
    dest->duration    = prev.duration;
  }
}

static void
copyToChirp(EchoDetector::Chirp *dest, const EchoDetector::Chirp &prev)
{
  copyCommonToChirp(dest, prev);

  dest->samples    = prev.samples;
  dest->snr        = prev.snr;
  dest->pN         = prev.pN;

  if (dest->processed) {
    dest->doppler     = prev.doppler;
    dest->softDoppler = prev.softDoppler;
  }
}

static void
moveToChirp(EchoDetector::Chirp *dest, EchoDetector::Chirp &&prev)
{
  dest->samples    = std::move(prev.samples);
  dest->snr        = std::move(prev.snr);
  dest->pN         = std::move(prev.pN);

  if (dest->processed) {
    dest->doppler     = std::move(prev.doppler);
    dest->softDoppler = std::move(prev.softDoppler);
  }
}

// C API constructor
EchoDetector::Chirp::Chirp(const struct graves_chirp_info *info)
{
  unsigned int i;

  this->start        = info->t0;
  this->startDecimal = info->t0f;
  this->Rbw          = info->rbw;
  this->fs           = info->fs;

  this->samples.assign(info->x, info->x + info->length);
  this->snr.resize(info->length);
  this->pN.resize(info->length);


  for (i = 0; i < info->length; ++i) {
    // We compute the SNR here directly
    this->snr[i] = graves_det_q_to_snr(this->Rbw, info->q[i]);

    // Compute noise power
    this->pN[i] =
        this->Rbw * graves_det_get_N0(this->Rbw, info->p_n[i], snr[i]);

  }
}

// Copy constructor
EchoDetector::Chirp::Chirp(const Chirp &prev)
{
  copyToChirp(this, prev);
}

// Move constructor
EchoDetector::Chirp::Chirp(Chirp &&prev)
{
  moveToChirp(this, std::move(prev));
}

// Copy assign
EchoDetector::Chirp &
EchoDetector::Chirp::operator=(const Chirp &rhs)
{
  copyToChirp(this, rhs);

  return *this;
}

// Move assign
EchoDetector::Chirp &
EchoDetector::Chirp::operator=(Chirp &&rhs)
{
  moveToChirp(this, std::move(rhs));

  return *this;
}

/////////////////////////// EchoDetector implementation //////////////////////
bool EchoDetector::registered = false;

void
EchoDetector::assertTypeRegistration(void)
{
  if (!EchoDetector::registered) {
    qRegisterMetaType<QStones::EchoDetector::Chirp>();
    EchoDetector::registered = true;
  }
}

SUBOOL
EchoDetector::OnChirpFunc(
    void *privdata,
    const struct graves_chirp_info *info)
{
  EchoDetector *detector = static_cast<EchoDetector *>(privdata);

  detector->emitChirp(EchoDetector::Chirp(info));

  return SU_TRUE;
}

EchoDetector::EchoDetector(
    QObject *parent,
    SUSCOUNT fs,
    SUFLOAT fc,
    SUFLOAT lpf1,
    SUFLOAT lpf2) :
  QObject(parent), instance(nullptr, graves_det_destroy)
{
  graves_det_t *ptr;
  struct graves_det_params params = graves_det_params_INITIALIZER;
  assertTypeRegistration();

  params.fs = fs;
  params.fc = fc;
  params.lpf1 = lpf1;
  params.lpf2 = lpf2;

  SU_ATTEMPT(ptr = graves_det_new(&params, OnChirpFunc, this));

  this->instance = std::unique_ptr<graves_det_t, void (*)(graves_det_t *)>(ptr, graves_det_destroy);
}

EchoDetector::EchoDetector(QObject *parent, SUSCOUNT fs, SUFLOAT fc) :
  EchoDetector(parent, fs, fc, 300, 50)
{

}

void
EchoDetector::feed(const SUCOMPLEX *samples, SUSCOUNT len)
{
  // Apply changes lazily
  if (this->freq_changed) {
    graves_det_set_center_freq(this->instance.get(), this->new_freq);
    this->freq_changed = false;
  }

  for (unsigned int i = 0; i < len; ++i)
    SU_ATTEMPT(graves_det_feed(this->instance.get(), samples[i]));
}

void
EchoDetector::setFreqLater(SUFLOAT freq)
{
  this->new_freq = freq;
  this->freq_changed = true;
}

void
EchoDetector::emitChirp(const Chirp &chirp)
{
  emit new_chirp(chirp);
}
