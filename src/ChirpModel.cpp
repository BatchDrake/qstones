//
//    ChirpModel.cpp: QAbstractTableModel for the chirp list
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

#include <sstream>
#include <iomanip>

#include "Application.h"
#include <iostream>

#include <vector>

using namespace QStones;

ChirpModel::ChirpModel(QObject *parent, Application &app) :
  QAbstractTableModel(parent), app(app)
{ }


int
ChirpModel::rowCount(const QModelIndex &) const
{
  return static_cast<int>(this->chirps.size());
}

int
ChirpModel::columnCount(const QModelIndex &) const
{
  return 4;
}

QVariant
ChirpModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole) {
    if (orientation == Qt::Horizontal) {
      switch (section)
      {
        case 0:
          return QString("Time");

        case 1:
          return QString("Duration");

        case 2:
          return QString("SNR");

        case 3:
          return QString("Doppler");
      }
    } else {
      return section + 1;
    }
  }

  return QVariant();
}

QString
ChirpModel::secsToTime(SUSCOUNT seconds)
{
  QString time;
  SUSCOUNT days;
  std::stringstream ss;

  days     = seconds / 86400;
  seconds %= 86400;

  if (days > 0) {
    QString date;

    if (days % 31 > 0)
      date += QString::number(days % 31) + "d ";
    days /= 31;

    if (days % 12 > 0)
      date += QString::number(days % 12) + "m ";

    days /= 12;
    if (days > 0)
      date += QString::number(days) + "y ";

    time += date;
  }

  ss << std::setw(2) << std::setfill('0') << seconds / 3600 << ":";
  seconds %= 3600;

  ss << std::setw(2) << std::setfill('0') << seconds / 60 << ":";
  seconds %= 60;

  ss << std::setw(2) << std::setfill('0') << seconds;
  time += QString::fromStdString(ss.str());

  return time;
}

QVariant
ChirpModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::DisplayRole) {
    unsigned long row = static_cast<unsigned long>(index.row());
    unsigned long col = static_cast<unsigned long>(index.column());
    const EchoDetector::Chirp &chirp = this->chirps[row];

    switch (col) {
      case 0:
        return secsToTime(chirp.start);

      case 1:
        return QString::number
            (static_cast<double>(chirp.samples.size()) /
             static_cast<double>(this->app.currSampleRate));

      case 2:
        return QString::number(static_cast<double>(chirp.SNR)) + " dB";

      case 3:
        return QString::number(static_cast<double>(chirp.mean_doppler)) + " m/s";
    }
  }

  return QVariant();
}

const EchoDetector::Chirp &
ChirpModel::at(unsigned long index) const
{
  return this->chirps[index];
}

void
ChirpModel::pushChirp(const EchoDetector::Chirp &chirp)
{
  this->chirps.push_back(chirp);

  // Process chirp data
  (this->chirps.end() - 1)->process();

  emit layoutChanged();
}
