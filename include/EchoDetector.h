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

    std::unique_ptr<graves_det_t, void (*)(graves_det_t *)> instance;

  public:
    struct Chirp;

    void feed(const SUCOMPLEX *samples, SUSCOUNT len);
    void emitChirp(const Chirp &);
    EchoDetector(QObject *, SUFLOAT, SUFLOAT);

  signals:
    void new_chirp(const Chirp &);
  };

  struct EchoDetector::Chirp {
    SUSCOUNT start;
    SUFLOAT N0;
    std::vector<SUCOMPLEX> samples;

    // Constructors
    Chirp(SUSCOUNT start, const SUCOMPLEX *data, SUSCOUNT len, SUFLOAT N0);
    Chirp(); // QT made me do this

    Chirp(const Chirp &);
    Chirp(Chirp &&);

    Chirp &operator=(const Chirp &);
    Chirp &operator=(Chirp &&);
  };
};

#endif // GRAVES_H
