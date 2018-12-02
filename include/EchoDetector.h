//
//    EchoDetector.h: Graves echo detector C++ wrapper
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

#ifndef GRAVES_H
#define GRAVES_H

#include <QObject>

#include <memory>
#include <vector>

#include <graves/graves.h>

namespace QStones {
  class EchoDetector: public QObject {
    Q_OBJECT

  private:
    std::unique_ptr<graves_det_t, void (*)(graves_det_t *)> instance;

    // FIXME: do we actually need to lock these?
    bool freq_changed = false;
    SUFLOAT new_freq;

    static bool registered;
    void assertTypeRegistration(void);

  public:
    struct Chirp;

    static SUBOOL OnChirpFunc(
        void *privdata,
        SUSCOUNT start,
        const SUCOMPLEX *data,
        SUSCOUNT len,
        SUFLOAT N0);

    void feed(const SUCOMPLEX *samples, SUSCOUNT len);

    // Lazy methods
    void setFreqLater(SUFLOAT new_freq);

    void emitChirp(const Chirp &);
    EchoDetector(QObject *, SUFLOAT, SUFLOAT);

  signals:
    void new_chirp(const QStones::EchoDetector::Chirp &);
  };

  // TODO: ADD SAMPLE RATE!!!!
  struct EchoDetector::Chirp {
    SUSCOUNT start;
    SUFLOAT N0;
    struct graves_det_params params;
    std::vector<SUCOMPLEX> samples;

    // Processed members
    bool processed = false;
    SUFLOAT SNR = 0;
    SUFLOAT mean_doppler = 0;
    SUFLOAT duration = 0;
    std::vector<SUFLOAT> doppler;

    // Methods
    void process(void);

    // Constructors
    Chirp(
        const struct graves_det_params *params,
        SUSCOUNT start,
        const SUCOMPLEX *data,
        SUSCOUNT len,
        SUFLOAT N0);
    Chirp(); // QT made me do this

    Chirp(const Chirp &);
    Chirp(Chirp &&);

    Chirp &operator=(const Chirp &);
    Chirp &operator=(Chirp &&);
  };
};

#endif // GRAVES_H
