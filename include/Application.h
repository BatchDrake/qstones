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

#define QSTONES_DEFAULT_TUNER_FREQ 143049000
#define QSTONES_DEFAULT_IF_FREQ    1000
#define QSTONES_DEFAULT_PEAK_HOLD  false
#define QSTONES_DEFAULT_LOCK       true
#define QSTONES_DEFAULT_WF_PROP    SU_ADDSFX(.5)
#define QSTONES_DEFAULT_SNR_BW     SU_ADDSFX(.16)

#define QSTONES_FFT_WINDOW_SIZE    2048
#define QSTONES_MAX_SAMPLE_RATE    100000

namespace QStones {
  struct ApplicationProperties {
    SUFREQ  tunFreq       = QSTONES_DEFAULT_TUNER_FREQ;
    SUFREQ  ifFreq        = QSTONES_DEFAULT_IF_FREQ;
    bool    peakHold      = QSTONES_DEFAULT_PEAK_HOLD;
    bool    lockPandapter = QSTONES_DEFAULT_LOCK;
    SUFLOAT snrBw         = QSTONES_DEFAULT_SNR_BW;
    SUFLOAT swProp        = QSTONES_DEFAULT_WF_PROP;
  };

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
    SUSCOUNT currSampleRate;
    struct ApplicationProperties prop;

    // UI
    Ui_Main *ui;
    ConfigDialog *configDialog;

    // Custom widgets
    CPlotter *plotter; // Deleted by parent

    void setProfile(Suscan::Source::Config);
    void connectAll(void);
    void setUIState(State state);
    void connectAnalyzer(void);
    void syncPlotter(void);
    void setSampleRate(SUSCOUNT rate);

  public:
    void run(void);

    void startCapture(void);
    void stopCapture(void);

    void setPandapterLocked(bool value, bool updateUi = true);
    void setTunerFrequency(SUFREQ freq, bool updateUi = true);
    void setIfFrequency(SUFREQ freq, bool updateUi = true);
    void setSpectrumWaterfallProportion(SUFLOAT prop, bool updateUi = true);
    explicit Application(QWidget *parent = nullptr);
    ~Application();

  public slots:
    void onTriggerSetup(bool);
    void onTriggerCapture(bool);
    void onTriggerStop(bool);
    void onAnalyzerHalted(void);
    void onPSDMessage(const Suscan::PSDMessage &message);
    void onFreqChanged(int);
    void onIFFreqChanged(int);
    void onSwPropChanged(int);
    void onPlotterNewTunerFreq(qint64 freq);
    void onPlotterNewIfFreq(qint64 freq, qint64 delta);
    void onToggleLockPandapter(int state);
  };
};

#endif // QSTONES_APPLICATION_H
