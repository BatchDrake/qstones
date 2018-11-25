//
//    ConfigDialog.h: Configuration dialog window
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
#ifndef CONFIG_H
#define CONFIG_H

#include <QDialog>
#include <ui_config.h>

#include <Suscan/Source.h>

namespace QStones {
  class ConfigDialog : public QDialog
  {
    Q_OBJECT

  public:
    Suscan::Source::Config getProfile(void);

    explicit ConfigDialog(QWidget *parent = nullptr);
    ~ConfigDialog();

  private:
    Ui_Config *ui;

    void connectAll(void);
    void populateProfiles(void);
  };
};

#endif // CONFIG_H
