//
//    Application.h: QStones application object
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

#ifndef QSTONES_APPLICATION_H
#define QSTONES_APPLICATION_H

#include <QMainWindow>
#include <Gqrx/CPlotter.h>

#include <ConfigDialog.h>
#include <ui_mainui.h>

#include <Suscan/Source.h>
#include <Suscan/Analyzer.h>

namespace QStones {
  class Application : public QMainWindow
  {
    Q_OBJECT

  private:
    enum State {
      HALTED,
      HALTING,
      RUNNING
    };

    // Analyzer object
    Suscan::Source::Config currProfile;
    std::unique_ptr<Suscan::Analyzer> analyzer;
    State state;

    // UI
    Ui_Main *ui;
    ConfigDialog *configDialog;

    // Custom widgets
    CPlotter *plotter; // Deleted by parent

    void setProfile(Suscan::Source::Config);
    void connectAll(void);
    void setUIState(State state);
    void connectAnalyzer(void);

  public:
    void run(void);

    void startCapture(void);
    void stopCapture(void);

    explicit Application(QWidget *parent = nullptr);
    ~Application();

  public slots:
    void onTriggerSetup(bool);
    void onTriggerCapture(bool);
    void onTriggerStop(bool);
    void onAnalyzerHalted(void);
    void onPSDMessage(const Suscan::PSDMessage &message);
  };
};

#endif // QSTONES_APPLICATION_H
