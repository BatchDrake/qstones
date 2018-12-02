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
  SUFLOAT offset, this_doppler;
  SUFLOAT doppler_sum = 0;
  SUFLOAT E = 0;
  SUFLOAT A2;
  SUFLOAT ratio;

  ratio = this->params.lpf2 / this->params.lpf1;

  len = this->samples.size();
  this->doppler.resize(len);

  for (i = 0; i < len; ++i) {
    // Compute square of the amplitude
    A2 = SU_C_REAL(this->samples[i] * SU_C_CONJ(this->samples[i]));
    E += A2; // Energy integration

    // Get immediate offset
    offset = SU_C_ARG(this->samples[i] * SU_C_CONJ(prev))
        / (2 * SU_ADDSFX(M_PI)) * this->params.fs;

    // Compute Doppler shift
    this_doppler = SU_ADDSFX(.5) * SPEED_OF_LIGHT * offset / GRAVES_CENTER_FREQ;

    doppler_sum += A2 * this_doppler; // Weight by A2
    doppler[i] = this_doppler;

    prev = this->samples[i];
  }

  this->SNR = SU_POWER_DB_RAW(E / (ratio * len * N0));
  this->mean_doppler = doppler_sum / E;
  this->duration = len / this->params.fs;

  this->processed = true;
}

EchoDetector::Chirp::Chirp(void) { } // Dumb constructor

// C API constructor
EchoDetector::Chirp::Chirp(
    const struct graves_det_params *params,
    SUSCOUNT start,
    const SUCOMPLEX *data,
    SUSCOUNT len,
    SUFLOAT N0)
{
  this->start = start;
  this->params = *params;
  this->samples.assign(data, data + len);
  this->N0 = N0;
}

// Copy constructor
EchoDetector::Chirp::Chirp(const Chirp &prev)
{
  this->start = prev.start;
  this->N0 = prev.N0;
  this->samples = prev.samples;
  this->params = prev.params;

  this->processed = prev.processed;
  if (this->processed) {
    this->SNR = prev.SNR;
    this->mean_doppler = prev.mean_doppler;
    this->duration = prev.duration;
    this->doppler = prev.doppler;
  }
}

// Move constructor
EchoDetector::Chirp::Chirp(Chirp &&prev)
{
  this->start = prev.start;
  this->N0 = prev.N0;
  this->samples = std::move(prev.samples);
  this->params = prev.params;

  this->processed = prev.processed;
  if (this->processed) {
    this->SNR = prev.SNR;
    this->mean_doppler = prev.mean_doppler;
    this->duration = prev.duration;
    this->doppler = std::move(prev.doppler);
  }
}

// Copy assign
EchoDetector::Chirp &
EchoDetector::Chirp::operator=(const Chirp &rhs)
{
  this->start = rhs.start;
  this->N0 = rhs.N0;
  this->samples = rhs.samples;
  this->params = rhs.params;

  this->processed = rhs.processed;
  if (this->processed) {
    this->SNR = rhs.SNR;
    this->mean_doppler = rhs.mean_doppler;
    this->duration = rhs.duration;
    this->doppler = rhs.doppler;
  }

  return *this;
}

// Move assign
EchoDetector::Chirp &
EchoDetector::Chirp::operator=(Chirp &&rhs)
{
  this->start = rhs.start;
  this->N0 = rhs.N0;
  this->samples = std::move(rhs.samples);
  this->params = rhs.params;

  this->processed = rhs.processed;
  if (this->processed) {
    this->SNR = rhs.SNR;
    this->mean_doppler = rhs.mean_doppler;
    this->duration = rhs.duration;
    this->doppler = std::move(rhs.doppler);
  }

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
    SUSCOUNT start,
    const SUCOMPLEX *data,
    SUSCOUNT len,
    SUFLOAT N0)
{
  EchoDetector *detector = static_cast<EchoDetector *>(privdata);

  detector->emitChirp(
        EchoDetector::Chirp(
        graves_det_get_params(detector->instance.get()),
        start,
        data,
        len,
        N0));

  return SU_TRUE;
}

EchoDetector::EchoDetector(
    QObject *parent,
    SUFLOAT fs,
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

EchoDetector::EchoDetector(QObject *parent, SUFLOAT fs, SUFLOAT fc) :
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
