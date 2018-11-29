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

#include <QMessageBox>
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

  connect(
        this->ui->sbRXFreq,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onFreqChanged(int)));

  connect(
        this->ui->sWRatio,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onSwPropChanged(int)));

  connect(
        this->ui->sbIFFreq,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onIFFreqChanged(int)));

  connect(
        this->plotter,
        SIGNAL(newCenterFreq(qint64)),
        this,
        SLOT(onPlotterNewTunerFreq(qint64)));

  connect(
        this->plotter,
        SIGNAL(newDemodFreq(qint64, qint64)),
        this,
        SLOT(onPlotterNewIfFreq(qint64, qint64)));

  connect(
        this->ui->cbLockPandapter,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onToggleLockPandapter(int)));

  connect(
        this->ui->eventTable->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        this,
        SLOT(onChirpSelected(const QItemSelection &, const QItemSelection &)));
}

void
Application::setProfile(Suscan::Source::Config profile)
{
  this->currProfile = profile;
  this->setWindowTitle(
        QString::fromStdString(
          "QStones - " + profile.label() + " - " + std::to_string(profile.getSampleRate())));

  this->plotter->setSampleRate(profile.getSampleRate());
  this->setTunerFrequency(profile.getFreq());
}

void
Application::updateChirpCharts(const EchoDetector::Chirp &chirp)
{
  QLineSeries *series;
  unsigned int n = 0;
  SUFLOAT limits = 0;

  this->chirpChart->removeAllSeries();

  // Add real component
  series = new QLineSeries(this->chirpChart);
  for (auto &p : chirp.samples) {
    series->append(
          qreal(n++) / qreal(this->currSampleRate), // FIXME
          qreal(SU_C_REAL(p)));

    if (SU_ABS(SU_C_REAL(p)) > limits)
      limits = SU_ABS(SU_C_REAL(p));
  }

  series->setColor(QColor(0, 0, 200));
  series->setName("Real part");
  this->chirpChart->addSeries(series);

  // Add imaginary component
  n = 0;
  series = new QLineSeries(this->chirpChart);
  for (auto &p : chirp.samples) {
    series->append(
          qreal(n++) / qreal(this->currSampleRate), // FIXME
          qreal(SU_C_IMAG(p)));

    if (SU_ABS(SU_C_IMAG(p)) > limits)
      limits = SU_ABS(SU_C_IMAG(p));
  }

  series->setColor(QColor(200, 0, 0));
  series->setName("Imaginary part");
  this->chirpChart->addSeries(series);
  this->chirpChart->createDefaultAxes();
  this->chirpChart->axisX()->setTitleText("Time (s)");
  this->chirpChart->axisY()->setMin(-limits);
  this->chirpChart->axisY()->setMax(limits);

  // Paint Doppler
  this->dopplerChart->removeAllSeries();
  n = 0;
  series = new QLineSeries(this->dopplerChart);
  for (auto &p : chirp.doppler)
    series->append(
          qreal(n++) / qreal(this->currSampleRate), // FIXME
          qreal(p));

  series->setColor(QColor(1, 1, 1));
  series->setName("Doppler shift");
  this->dopplerChart->addSeries(series);
  this->dopplerChart->createDefaultAxes();
  this->dopplerChart->axisX()->setTitleText("Time (s)");
  this->dopplerChart->axisY()->setTitleText("Relative speed (m/s)");
  this->dopplerChart->axisY()->setMin(-SU_ABS(2 * chirp.mean_doppler));
  this->dopplerChart->axisY()->setMax(SU_ABS(2 * chirp.mean_doppler));

}

Application::Application(QWidget *parent) : QMainWindow(parent)
{
  QGridLayout *layout;

  // Initialize everything that does not depend on Suscan
  this->ui = new Ui_Main();
  this->ui->setupUi(this);

  // Create chirp model
  this->chirpModel = new ChirpModel(this, *this);
  this->ui->eventTable->setModel(this->chirpModel);

  // Add custom widgets
  this->plotter = new CPlotter(this);
  this->ui->verticalSplitter->insertWidget(0, this->plotter);
  this->setSampleRate(44100); // Dummy sample rate

  // Add chirp chart
  this->chirpChart = new QChart();
  this->chirpChart->setTitle("Chirp signal over time");
  this->chirpView = new QChartView(this->chirpChart);
  this->chirpView->setRenderHint(QPainter::Antialiasing);

  layout = new QGridLayout(this->ui->chirpFrame);
  layout->setSpacing(6);
  layout->setContentsMargins(11, 11, 11, 11);
  layout->addWidget(this->chirpView, 0, 0, 0, 0);

  // Add Doppler chart
  this->dopplerChart = new QChart();
  this->dopplerChart->setTitle("Doppler shift over time");
  this->dopplerChart->legend()->hide();
  this->dopplerView = new QChartView(this->dopplerChart);
  this->dopplerView->setRenderHint(QPainter::Antialiasing);

  layout = new QGridLayout(this->ui->dopplerFrame);
  layout->setSpacing(6);
  layout->setContentsMargins(11, 11, 11, 11);
  layout->addWidget(this->dopplerView, 0, 0, 0, 0);

  // Setup UI state
  this->setUIState(HALTED);

  this->setTunerFrequency(this->prop.tunFreq);
  this->setIfFrequency(this->prop.ifFreq);
  this->setSpectrumWaterfallProportion(this->prop.swProp);
  this->setPandapterLocked(this->prop.lockPandapter);
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
Application::connectDetector(void)
{
  connect(
        this->detector.get(),
        SIGNAL(new_chirp(const QStones::EchoDetector::Chirp &)),
        this,
        SLOT(onChirp(const QStones::EchoDetector::Chirp &)));
}

SUPRIVATE SUBOOL
onBaseBandData(
    void *privdata,
    suscan_analyzer_t *,
    const SUCOMPLEX *samples,
    SUSCOUNT length)
{
  EchoDetector *det = static_cast<EchoDetector *>(privdata);

  det->feed(samples, length);

  return SU_TRUE;
}

void
Application::startCapture(void)
{
  struct suscan_analyzer_params default_params =
      suscan_analyzer_params_INITIALIZER;

  default_params.detector_params.window_size = QSTONES_FFT_WINDOW_SIZE;

  try {
    if (this->state == HALTED) {
      if (this->currProfile.getType() == SUSCAN_SOURCE_TYPE_SDR
          && this->currProfile.getSampleRate() > QSTONES_MAX_SAMPLE_RATE) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(
              this,
              "Sample rate too high",
              "The sample rate of profile \""
              + QString::fromStdString(this->currProfile.label())
              + "\" is unusually big ("
              + QString::number(this->currProfile.getSampleRate())
              + "). Temporarily reduce it to "
              + QString::number(QSTONES_MAX_SAMPLE_RATE)
              + "?",
              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes)
          this->currProfile.setSampleRate(QSTONES_MAX_SAMPLE_RATE);
        else if (reply == QMessageBox::Cancel)
          return;
      }

      this->analyzer = std::make_unique<Suscan::Analyzer>(default_params, this->currProfile);
      this->detector = std::make_unique<EchoDetector>(
            this,
            this->currProfile.getSampleRate(),
            this->prop.ifFreq);

      // Add baseband filter to feed echo detector
      this->analyzer.get()->registerBaseBandFilter(onBaseBandData, this->detector.get());

      this->connectDetector();
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
Application::setSampleRate(SUSCOUNT rate)
{
  if (this->currSampleRate != rate) {
    this->plotter->setSampleRate(rate);
    this->plotter->resetHorizontalZoom();
    this->currSampleRate = rate;
  }
}

void
Application::onPSDMessage(const Suscan::PSDMessage &msg)
{
  this->setSampleRate(msg.getSampleRate());
  this->plotter->setNewFftData((float *) msg.get(), (int) msg.size());
}

void
Application::syncPlotter(void)
{
  this->plotter->setCenterFreq(static_cast<quint64>(this->prop.tunFreq));
  this->plotter->setFilterOffset(static_cast<qint64>(this->prop.ifFreq));
}

void
Application::setPandapterLocked(bool value, bool updateUi)
{
  this->prop.lockPandapter = value;
  this->plotter->setLocked(value);

  if (updateUi)
    this->ui->cbLockPandapter->setCheckState(
        value
        ? Qt::Checked
        : Qt::Unchecked);
}

void
Application::setTunerFrequency(SUFREQ freq, bool updateUi)
{
  this->prop.tunFreq = freq;

  if (this->state == RUNNING)
    this->analyzer.get()->setFrequency(freq);

  if (updateUi) {
    this->syncPlotter();
    this->ui->sbRXFreq->setValue(static_cast<int>(freq));
  }
}

void
Application::setSpectrumWaterfallProportion(SUFLOAT prop, bool updateUi)
{
  this->prop.swProp = prop;

  this->plotter->setPercent2DScreen(static_cast<int>(prop * 100));

  if (updateUi)
    this->ui->sWRatio->setValue(static_cast<int>(prop * 100));
}

void
Application::setIfFrequency(SUFREQ freq, bool updateUi)
{
  this->prop.ifFreq = freq;

  // TODO: Change demodulator frequency

  if (updateUi) {
    this->syncPlotter();
    this->ui->sbIFFreq->setValue(static_cast<int>(freq));
  }
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

void
Application::onFreqChanged(int freq)
{
  this->setTunerFrequency(freq, false);
  this->syncPlotter();
}

void
Application::onIFFreqChanged(int freq)
{
  this->setIfFrequency(freq, false);
  this->syncPlotter();
}

void
Application::onPlotterNewTunerFreq(qint64 freq)
{
  this->setTunerFrequency(freq);
}

void
Application::onPlotterNewIfFreq(qint64 freq, qint64 delta)
{
  this->setTunerFrequency(freq - delta);
  this->setIfFrequency(delta);
}

void
Application::onSwPropChanged(int prop)
{
  this->setSpectrumWaterfallProportion(
        static_cast<float>(prop) / 100.f,
        false);
}

void
Application::onToggleLockPandapter(int state)
{
  this->setPandapterLocked(state == Qt::Checked, false);
}

void
Application::onChirp(const EchoDetector::Chirp &chirp)
{
  this->chirpModel->pushChirp(chirp);
}

void
Application::onChirpSelected(const QItemSelection &, const QItemSelection &)
{
  QModelIndexList selected =
      this->ui->eventTable->selectionModel()->selectedRows();

  if (selected.count() == 1) {
    const EchoDetector::Chirp &chirp =
        this->chirpModel->at(
          static_cast<unsigned long>(selected.at(0).row()));
    this->updateChirpCharts(chirp);
  }
}

Application::~Application()
{
  delete this->ui;
}
