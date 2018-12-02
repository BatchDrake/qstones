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
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

#include <ConfigDialog.h>
#include <ui_mainui.h>

#include <Suscan/Source.h>
#include <Suscan/Analyzer.h>

#include "EchoDetector.h"

#define QSTONES_DEFAULT_TUNER_FREQ 143049000
#define QSTONES_DEFAULT_IF_FREQ    SU_ADDSFX(1000.)
#define QSTONES_DEFAULT_PEAK_HOLD  false
#define QSTONES_DEFAULT_LOCK       true
#define QSTONES_DEFAULT_WF_PROP    SU_ADDSFX(.5)
#define QSTONES_DEFAULT_SNR_BW     SU_ADDSFX(.16)
#define QSTONES_DEFAULT_MIN_DB     -140
#define QSTONES_DEFAULT_MAX_DB     0
#define QSTONES_FFT_WINDOW_SIZE    2048
#define QSTONES_MAX_SAMPLE_RATE    100000

QT_CHARTS_USE_NAMESPACE

namespace QStones {
  class Application;

  class ChirpModel: public QAbstractTableModel {
    Q_OBJECT

  private:
    std::vector<EchoDetector::Chirp> chirps;
    Application &app;

    static QString secsToTime(SUSCOUNT sec);

  public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void pushChirp(const EchoDetector::Chirp &chirp);
    const EchoDetector::Chirp &at(unsigned long index) const;

    ChirpModel(QObject *parent, Application &app);
  };

  struct ApplicationProperties {
    SUFREQ  tunFreq       = QSTONES_DEFAULT_TUNER_FREQ;
    SUFLOAT ifFreq        = QSTONES_DEFAULT_IF_FREQ;
    bool    peakHold      = QSTONES_DEFAULT_PEAK_HOLD;
    bool    lockPandapter = QSTONES_DEFAULT_LOCK;
    SUFLOAT snrBw         = QSTONES_DEFAULT_SNR_BW;
    SUFLOAT swProp        = QSTONES_DEFAULT_WF_PROP;
    int     minDb         = QSTONES_DEFAULT_MIN_DB;
    int     maxDb         = QSTONES_DEFAULT_MAX_DB;
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
    std::unique_ptr<EchoDetector> detector;

    State state;
    SUSCOUNT currSampleRate;
    struct ApplicationProperties prop;

    // UI
    Ui_Main *ui;
    ConfigDialog *configDialog;

    // Custom widgets
    ChirpModel *chirpModel;
    CPlotter *plotter; // Deleted by parent
    QChart *chirpChart;
    QChart *dopplerChart;
    QChartView *chirpView;
    QChartView *dopplerView;

    void setProfile(Suscan::Source::Config);
    void connectAll(void);
    void setUIState(State state);
    void connectAnalyzer(void);
    void connectDetector(void);
    void syncPlotter(void);
    void setSampleRate(SUSCOUNT rate);
    void updateChirpCharts(const EchoDetector::Chirp &);

    friend class ChirpModel;

  public:
    static void flushLog(void);
    static QString getLogText(void);
    void run(void);

    void startCapture(void);
    void stopCapture(void);

    void setPandapterLocked(bool value, bool updateUi = true);
    void setPeakHold(bool value, bool updateUi = true);
    void setTunerFrequency(SUFREQ freq, bool updateUi = true);
    void setIfFrequency(SUFLOAT freq, bool updateUi = true);
    void setSpectrumWaterfallProportion(SUFLOAT prop, bool updateUi = true);
    void setSpectrumMinDb(int min, bool updateUi = true);
    void setSpectrumMaxDb(int max, bool updateUi = true);

    explicit Application(QWidget *parent = nullptr);
    ~Application();

  public slots:
    void onTriggerSetup(bool);
    void onTriggerCapture(bool);
    void onTriggerStop(bool);
    void onAnalyzerHalted(void);
    void onAnalyzerReadError(void);
    void onAnalyzerEos(void);
    void onPSDMessage(const Suscan::PSDMessage &message);
    void onFreqChanged(int);
    void onIFFreqChanged(int);
    void onSwPropChanged(int);
    void onPlotterNewTunerFreq(qint64 freq);
    void onPlotterNewIfFreq(qint64 freq, qint64 delta);
    void onToggleLockPandapter(int state);
    void onTogglePeakHold(int state);
    void onChirp(const QStones::EchoDetector::Chirp &);
    void onChirpSelected(const QItemSelection &, const QItemSelection &);
  };
};

#endif // QSTONES_APPLICATION_H
