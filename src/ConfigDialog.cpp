//
//    ConfigDialog.cpp: Configuration dialog window
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
#include <iostream>
#include <Suscan/Library.h>

#include "ConfigDialog.h"

using namespace QStones;

Q_DECLARE_METATYPE(Suscan::Source::Config); // Unicorns

void
ConfigDialog::populateProfiles(void)
{
  Suscan::Singleton *sus = Suscan::Singleton::get_instance();

  for (auto i = sus->getFirstProfile(); i != sus->getLastProfile(); ++i)
      this->ui->profileCombo->addItem(
            QString::fromStdString(i->label()),
            QVariant::fromValue(*i));
}

void
ConfigDialog::connectAll(void)
{
  // Stub
}

Suscan::Source::Config
ConfigDialog::getProfile(void)
{
  QVariant data = this->ui->profileCombo->itemData(this->ui->profileCombo->currentIndex());

  return data.value<Suscan::Source::Config>();
}

ConfigDialog::ConfigDialog(QWidget *parent) : QDialog(parent)
{
  this->ui = new Ui_Config();
  this->ui->setupUi(this);

  this->setWindowTitle("Signal source configuration");

  // Populate this dialog with profiles
  this->populateProfiles();
}

ConfigDialog::~ConfigDialog()
{
    delete this->ui;
}
