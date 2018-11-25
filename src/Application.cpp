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

  connect(
        this->ui->actionCapture,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerCapture(bool)));

  connect(
        this->ui->actionStop_capture,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onTriggerStop(bool)));
}

void
Application::setProfile(Suscan::Source::Config profile)
{
  this->currProfile = profile;
  this->setWindowTitle(
        QString::fromStdString("GRAVES echo detector - " + profile.label()));
}

Application::Application(QWidget *parent) : QMainWindow(parent)
{
  // Initialize everything that does not depend on Suscan

  this->ui = new Ui_Main();
  this->ui->setupUi(this);

  // Add custom widgets
  this->plotter = new CPlotter(this);
  this->ui->verticalSplitter->insertWidget(0, this->plotter);
  this->plotter->setSampleRate(250000);

  // Setup UI state
  this->setUIState(HALTED);
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

void
Application::setUIState(State state)
{
  this->state = state;

  this->ui->actionCapture->setEnabled(state == HALTED);
  this->ui->actionStop_capture->setEnabled(state == RUNNING);
}

void
Application::onAnalyzerHalted(void)
{
  this->setUIState(HALTED);
  this->analyzer = nullptr;
}

void
Application::connectAnalyzer(void)
{
  connect(
        this->analyzer.get(),
        SIGNAL(halted(void)),
        this,
        SLOT(onAnalyzerHalted(void)));

  connect(
        this->analyzer.get(),
        SIGNAL(psd_message(const Suscan::PSDMessage &)),
        this,
        SLOT(onPSDMessage(const Suscan::PSDMessage &)));
}

void
Application::startCapture(void)
{
  struct suscan_analyzer_params default_params =
      suscan_analyzer_params_INITIALIZER;

  default_params.detector_params.window_size = 2048;

  try {
    if (this->state == HALTED) {
        this->analyzer = std::make_unique<Suscan::Analyzer>(default_params, this->currProfile);
        this->connectAnalyzer();
        this->setUIState(RUNNING);
      }
  } catch (Suscan::Exception &e) {
    (void)  QMessageBox::critical(
          this,
          "QStones error",
          QString::fromStdString("Failed to create analyzer object. Error was:\n\n"
            + std::string(e.what())),
          QMessageBox::Ok);
  }
}

void
Application::stopCapture(void)
{
  if (this->state == RUNNING) {
    this->analyzer.get()->halt();
    this->setUIState(HALTING);
  }
}

void
Application::onPSDMessage(const Suscan::PSDMessage &msg)
{
  this->plotter->setNewFftData((float *) msg.get(), (int) msg.size());
}

void
Application::onTriggerSetup(bool)
{
  this->configDialog->exec();
  this->setProfile(this->configDialog->getProfile());
}

void
Application::onTriggerCapture(bool)
{
  this->startCapture();
}

void
Application::onTriggerStop(bool)
{
  this->stopCapture();
}

Application::~Application()
{
  if (this->configDialog != nullptr)
    delete this->configDialog;

  delete this->ui;
}
