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

using namespace QStones;

EchoDetector::Chirp::Chirp(void) { } // Dumb constructor

// C API constructor
EchoDetector::Chirp::Chirp(SUSCOUNT start, const SUCOMPLEX *data, SUSCOUNT len, SUFLOAT N0)
{
  this->start = start;
  this->samples.assign(data, data + len);
  this->N0 = N0;
}

// Copy constructor
EchoDetector::Chirp::Chirp(const Chirp &prev)
{
  this->start = prev.start;
  this->N0 = prev.N0;
  this->samples = prev.samples;
}

// Move constructor
EchoDetector::Chirp::Chirp(Chirp &&prev)
{
  this->start = prev.start;
  this->N0 = prev.N0;
  this->samples = std::move(prev.samples);
}

// Copy assign
EchoDetector::Chirp &
EchoDetector::Chirp::operator=(const Chirp &rhs)
{
  this->start = rhs.start;
  this->N0 = rhs.N0;
  this->samples = rhs.samples;

  return *this;
}

// Move assign
EchoDetector::Chirp &
EchoDetector::Chirp::operator=(Chirp &&rhs)
{
  this->start = rhs.start;
  this->N0 = rhs.N0;
  this->samples = std::move(rhs.samples);

  return *this;
}

/////////////////////////// EchoDetector implementation //////////////////////

SUBOOL
OnChirpFunc(void *privdata,
            SUFLOAT,
            SUSCOUNT start,
            const SUCOMPLEX *data,
            SUSCOUNT len,
            SUFLOAT N0)
{
  EchoDetector *detector = static_cast<EchoDetector *>(privdata);

  detector->emitChirp(EchoDetector::Chirp(start, data, len, N0));

  return SU_TRUE;
}

EchoDetector::EchoDetector(QObject *parent, SUFLOAT fs, SUFLOAT fc) :
  QObject(parent), instance(nullptr, graves_det_destroy)
{
  graves_det_t *ptr;

  SU_ATTEMPT(ptr = graves_det_new(fs, fc, OnChirpFunc, this));

  this->instance = std::unique_ptr<graves_det_t, void (*)(graves_det_t *)>(ptr, graves_det_destroy);
}

void
EchoDetector::feed(const SUCOMPLEX *samples, SUSCOUNT len)
{
  for (unsigned int i = 0; i < len; ++i)
    SU_ATTEMPT(graves_det_feed(this->instance.get(), samples[i]));
}

void
EchoDetector::emitChirp(const Chirp &chirp)
{
  emit new_chirp(chirp);
}
