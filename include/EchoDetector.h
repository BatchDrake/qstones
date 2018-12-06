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

#define QSTONES_MAX_SNR SU_ADDSFX(100.)

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
        const struct graves_chirp_info *info);

    void feed(const SUCOMPLEX *samples, SUSCOUNT len);

    // Lazy methods
    void setFreqLater(SUFLOAT new_freq);

    void emitChirp(const Chirp &);
    EchoDetector(QObject *, SUSCOUNT, SUFLOAT);
    EchoDetector(QObject *, SUSCOUNT, SUFLOAT, SUFLOAT, SUFLOAT);

  signals:
    void new_chirp(const QStones::EchoDetector::Chirp &);
  };

  // TODO: ADD SAMPLE RATE!!!!
  struct EchoDetector::Chirp {
    enum MemberType {
      SCALARS = 1,
      SAMPLES = 2,
      POWER_NARROW = 4,
      POWER_WIDE = 8,
      SNR = 16,
      DOPPLER = 32,
      SOFT_DOPPLER = 64
    };

    SUSCOUNT start;
    SUFLOAT  startDecimal;

    SUSCOUNT fs;
    SUFLOAT  Rbw;

    std::vector<SUCOMPLEX> samples;
    std::vector<SUFLOAT> pN; // Noise power in the narrow channel
    std::vector<SUFLOAT> pW; // Noise power in the wide channel
    std::vector<SUFLOAT> snr;

    // Processed members
    bool processed = false;

    SUFLOAT meanSNR = 0; // This is a mean
    SUFLOAT meanDoppler = 0; // Also  mear
    SUFLOAT duration = 0;

    std::vector<SUFLOAT> doppler;
    std::vector<SUFLOAT> softDoppler;

    // Methods
    void process(void);

    std::string serialize(int what) const;

    // Constructors
    Chirp(const struct graves_chirp_info *info);
    Chirp(); // QT made me do this

    Chirp(const Chirp &);
    Chirp(Chirp &&);

    Chirp &operator=(const Chirp &);
    Chirp &operator=(Chirp &&);
  };
};

#endif // GRAVES_H
