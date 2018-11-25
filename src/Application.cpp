//
//    Application.cpp: Application window implementation
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

#include "Application.h"

using namespace QStones;

void
Application::connectAll(void)
{
  connect(
        this->ui->actionSetup,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerSetup(bool)));
}

void
Application::setProfile(Suscan::Source::Config profile)
{
  this->currProfile = profile;
  this->setWindowTitle(
        QString::fromStdString("GRAVES echo detector - " + profile.label()));
}

void
Application::onTriggerSetup(bool)
{
  this->configDialog->exec();
  this->setProfile(this->configDialog->getProfile());
}

Application::Application(QWidget *parent) : QMainWindow(parent)
{
  // Initialize everything that does not depend on Suscan

  this->ui = new Ui_Main();
  this->ui->setupUi(this);
}

void
Application::run(void)
{
  // Create profile dialog
  this->configDialog = new ConfigDialog(this);

  // Get current profile
  this->setProfile(this->configDialog->getProfile());

  // Connect signals
  this->connectAll();

  // Go!
  this->show();
}

Application::~Application()
{
  if (this->configDialog != nullptr)
    delete this->configDialog;

  delete this->ui;
}
