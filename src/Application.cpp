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

#include <QFileDialog>
#include <QOpenGLWidget>
#include <QMessageBox>
#include "Application.h"

#include <fstream>

using namespace QStones;

bool
Application::saveChartView(QChartView *chartView, const QString &path)
{
  QPixmap p = chartView->grab();
  QOpenGLWidget *glWidget  = chartView->findChild<QOpenGLWidget*>();

  if (glWidget){
      QPainter painter(&p);
      QPoint d =
             glWidget->mapToGlobal(QPoint())
          - chartView->mapToGlobal(QPoint());
      painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
      painter.drawImage(d, glWidget->grabFramebuffer());
      painter.end();
  }

  return p.save(path, "PNG");
}

bool
Application::saveChirpData(
    const EchoDetector::Chirp *chirp,
    const QString &str,
    int what)
{
  std::ofstream fs;
  bool ok;

  fs.open(str.toStdString().c_str());
  ok = fs.is_open();

  if (ok)
    fs << chirp->serialize(what);

  return ok;
}

void
Application::flushLog(void)
{
  Suscan::Logger::getInstance()->flush();
}

QString
Application::getLogText(void)
{
  QString text = "";
  std::lock_guard<Suscan::Logger> guard(*Suscan::Logger::getInstance());

  for (const auto &p : *Suscan::Logger::getInstance()) {
    switch (p.severity) {
      case SU_LOG_SEVERITY_CRITICAL:
        text += "critical: ";
        break;

      case SU_LOG_SEVERITY_DEBUG:
        text += "debug: ";
        break;

      case SU_LOG_SEVERITY_ERROR:
        text += "error: ";
        break;

      case SU_LOG_SEVERITY_INFO:
        text += "info: ";
        break;

      case SU_LOG_SEVERITY_WARNING:
        text += "warning: ";
        break;
    }

    text += p.message.c_str();
  }

  return text;
}

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
        this->ui->sRange,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onSwPropChanged(int)));

  connect(
        this->ui->sFloor,
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
        this->ui->cbPeakHold,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onTogglePeakHold(int)));

  connect(
        this->ui->eventTable->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        this,
        SLOT(onChirpSelected(const QItemSelection &, const QItemSelection &)));

  connect(
        this->ui->cbThrottle,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(onThrottleChanged(void)));

  connect(
        this->ui->sbThrottleValue,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(onThrottleChanged(void)));

  connect(
        this->ui->actionQuit,
        SIGNAL(triggered(bool)),
        QApplication::instance(),
        SLOT(quit()));

  connect(
        this->ui->actionClear_all,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onClearEventTable(void)));

  connect(
        this->ui->pbSaveChirpPlot,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSaveChirpPlot(void)));

  connect(
        this->ui->pbSaveDopplerPlot,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSaveDopplerPlot(void)));

  connect(
        this->ui->pbSavePowerPlot,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSavePowerPlot(void)));

  connect(
        this->ui->pbSaveChirp,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSaveChirp(void)));

  connect(
        this->ui->pbSaveDoppler,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSaveDoppler(void)));

  connect(
        this->ui->pbSavePower,
        SIGNAL(clicked(bool)),
        this,
        SLOT(onSavePower(void)));

  connect(
        this->ui->actionSave,
        SIGNAL(triggered(bool)),
        this,
        SLOT(onSaveFullChirpData(void)));
}

void
Application::setProfile(Suscan::Source::Config profile)
{
  this->currProfile = profile;
  this->setWindowTitle(
        QString::fromStdString(
          "QStones - " + this->currProfile.label() + " - " + std::to_string(this->currProfile.getSampleRate())));

  this->plotter->setSampleRate(profile.getSampleRate());
  this->plotter->setFreqUnits(1e3);
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

  series->setColor(QColor(80, 80, 255));
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

  series->setColor(QColor(255, 80, 80));
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

  series->setColor(QColor(230, 200, 0));
  series->setName("Doppler shift");
  this->dopplerChart->addSeries(series);
  this->dopplerChart->createDefaultAxes();
  this->dopplerChart->axisX()->setTitleText("Time (s)");
  this->dopplerChart->axisY()->setTitleText("Relative speed (m/s)");
  this->dopplerChart->axisY()->setMin(-SU_ABS(2 * chirp.meanDoppler));
  this->dopplerChart->axisY()->setMax(SU_ABS(2 * chirp.meanDoppler));

  // Paint power plot
  this->pwpChart->removeAllSeries();

  // Paint narrow channel
  limits = 0;
  n = 0;
  series = new QLineSeries(this->pwpChart);
  for (auto &p : chirp.pN) {
    series->append(
          qreal(n++) / qreal(this->currSampleRate), // FIXME
          qreal(p));
    if (p > limits)
      limits = p;
  }

  series->setColor(QColor(100, 255, 100));
  series->setName("Narrow Channel");
  this->pwpChart->addSeries(series);

  // Paint wide channel
  n = 0;
  series = new QLineSeries(this->pwpChart);
  for (auto &p : chirp.pW) {
    series->append(
          qreal(n++) / qreal(this->currSampleRate), // FIXME
          qreal(p));
    if (p > limits)
      limits = p;
  }

  series->setColor(QColor(180, 100, 255));
  series->setName("Wide Channel");
  this->pwpChart->addSeries(series);

  this->pwpChart->createDefaultAxes();
  this->pwpChart->axisX()->setTitleText("Time (s)");
  this->pwpChart->axisY()->setTitleText("Power (full scale)");
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
  this->chirpChart->setTheme(QChart::ChartThemeDark);
  this->chirpView = new QChartView(this->chirpChart, this);
  this->chirpView->setRenderHint(QPainter::Antialiasing);

  layout = new QGridLayout(this->ui->chirpFrame);
  layout->setSpacing(6);
  layout->setContentsMargins(11, 11, 11, 11);
  layout->addWidget(this->chirpView, 0, 0, 0, 0);

  // Add Doppler chart
  this->dopplerChart = new QChart();
  this->dopplerChart->setTitle("Doppler shift over time");
  this->dopplerChart->legend()->hide();
  this->dopplerChart->setTheme(QChart::ChartThemeDark);
  this->dopplerView = new QChartView(this->dopplerChart, this);
  this->dopplerView->setRenderHint(QPainter::Antialiasing);

  layout = new QGridLayout(this->ui->dopplerFrame);
  layout->setSpacing(6);
  layout->setContentsMargins(11, 11, 11, 11);
  layout->addWidget(this->dopplerView, 0, 0, 0, 0);

  // Add SNR chart
  this->pwpChart = new QChart();
  this->pwpChart->setTitle("Signal power per channel");
  this->pwpChart->setTheme(QChart::ChartThemeDark);
  this->pwpView = new QChartView(this->pwpChart, this);
  this->pwpView->setRenderHint(QPainter::Antialiasing);

  layout = new QGridLayout(this->ui->pwpFrame);
  layout->setSpacing(6);
  layout->setContentsMargins(11, 11, 11, 11);
  layout->addWidget(this->pwpView, 0, 0, 0, 0);

  // Setup UI state
  this->setUIState(HALTED);

  this->setTunerFrequency(this->prop.tunFreq);
  this->setIfFrequency(this->prop.ifFreq);
  this->setSpectrumWaterfallProportion(this->prop.swProp);
  this->setPandapterLocked(this->prop.lockPandapter);
  this->setSpectrumMinDb(this->prop.minDb);
  this->setSpectrumMaxDb(this->prop.maxDb);
  this->setThrottleEnabled(this->prop.throttle);
  this->setThrottleValue(this->prop.efSampRate);
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
  printf("Analyzer halted\n");
  this->setUIState(HALTED);
  this->analyzer = nullptr;
}

void
Application::onAnalyzerEos(void)
{
  printf("Analyzer EOS\n");
  this->setUIState(HALTED);
  this->analyzer = nullptr;
}

void
Application::onAnalyzerReadError(void)
{
  printf("Analyzer read error\n");
  (void)  QMessageBox::critical(
        this,
        "Source error",
        "Capture stopped due to source read error. Last errors were:<p /><pre>"
        + getLogText()
        + "</pre>",
        QMessageBox::Ok);
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
        SIGNAL(eos(void)),
        this,
        SLOT(onAnalyzerEos(void)));

  connect(
        this->analyzer.get(),
        SIGNAL(read_error(void)),
        this,
        SLOT(onAnalyzerReadError(void)));

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
  Suscan::AnalyzerParams default_params;

  default_params.windowSize = QSTONES_FFT_WINDOW_SIZE;
  default_params.mode = Suscan::AnalyzerParams::Mode::CHANNEL;

  try {
    if (this->state == HALTED) {
      std::unique_ptr<Suscan::Analyzer> analyzer;
      std::unique_ptr<EchoDetector> detector;
      SUFLOAT lpf1, lpf2;
      SUFLOAT oldIfFreq = this->prop.ifFreq;
      int maxIfFreq;

      this->firstPSDrecv = false;

      if (this->currProfile.getType() == SUSCAN_SOURCE_TYPE_SDR) {
        const suscan_source_device_t *dev = this->currProfile.getDevice().getInstance();
        for (int i = 0; i < dev->args->size; ++i)
          printf("%s = %s\n", dev->args->keys[i], dev->args->vals[i]);
        this->setThrottleEnabled(false);
        if (this->currProfile.getSampleRate() > QSTONES_MAX_SAMPLE_RATE) {
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
      }

      // Set filter cutoffs
      lpf1 = SU_NORM2ABS_FREQ(
            this->currProfile.getSampleRate(),
            10 * GRAVES_MIN_LPF_CUTOFF);

      lpf2 = SU_NORM2ABS_FREQ(
            this->currProfile.getSampleRate(),
            GRAVES_MIN_LPF_CUTOFF);

      this->ui->sbLPF1->setValue(static_cast<double>(lpf1));
      this->ui->sbLPF2->setValue(static_cast<double>(lpf2));

      // Throttle is only enabled for file source
      this->ui->cbThrottle->setEnabled(
            this->currProfile.getType() == SUSCAN_SOURCE_TYPE_FILE);

      maxIfFreq = this->currProfile.getSampleRate() / 2;

      this->ui->sbIFFreq->setMaximum(+maxIfFreq);
      this->ui->sbIFFreq->setMinimum(-maxIfFreq);
      this->setIfFrequency(oldIfFreq);

      if (SU_ABS(oldIfFreq) > maxIfFreq) {
        this->setIfFrequency(0);

        QMessageBox::information(
              this,
              "IF out of bounds",
              "Detector IF was set to 0 Hz to fit sample rate constraints.",
              QMessageBox::Ok);
      }

      // Flush log messages from here
      flushLog();

      // Allocate objects
      analyzer = std::make_unique<Suscan::Analyzer>(
            default_params,
            this->currProfile);

      detector = std::make_unique<EchoDetector>(
            this,
            this->currProfile.getSampleRate(),
            this->prop.ifFreq,
            lpf1,
            lpf2);

      // Add baseband filter to feed echo detector
      analyzer.get()->registerBaseBandFilter(
            onBaseBandData,
            detector.get());

      // All set, move to application
      this->analyzer = std::move(analyzer);
      this->detector = std::move(detector);

      this->connectDetector();
      this->connectAnalyzer();

      this->setUIState(RUNNING);
    }
  } catch (Suscan::Exception &) {
    (void)  QMessageBox::critical(
          this,
          "QStones error",
          "Failed to start capture due to errors:<p /><pre>"
          + getLogText().toHtmlEscaped()
          + "</pre>",
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
Application::setSampleRate(unsigned int rate)
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
  if (!this->firstPSDrecv) {
    if (this->prop.throttle)
      this->setThrottleValue(this->prop.efSampRate);
    this->firstPSDrecv = true;
  }
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
Application::setPeakHold(bool value, bool updateUi)
{
  this->prop.lockPandapter = value;
  this->plotter->setPeakHold(value);

  if (updateUi)
    this->ui->cbPeakHold->setCheckState(
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
Application::setSpectrumMinDb(int min, bool updateUi)
{
  this->prop.minDb = min;

  this->plotter->setFftRange(this->prop.minDb, this->prop.maxDb);

  if (updateUi)
    this->ui->sFloor->setValue(min);
}

void
Application::setSpectrumMaxDb(int max, bool updateUi)
{
  this->prop.maxDb = max;

  this->plotter->setFftRange(this->prop.minDb, this->prop.maxDb);

  if (updateUi)
    this->ui->sRange->setValue(this->prop.maxDb - this->prop.minDb);
}

void
Application::setIfFrequency(SUFLOAT freq, bool updateUi)
{
  this->prop.ifFreq = freq;

  // Queue update of center frequency
  if (this->state == RUNNING)
    this->detector.get()->setFreqLater(freq);

  if (updateUi) {
    this->syncPlotter();
    this->ui->sbIFFreq->setValue(static_cast<int>(freq));
  }
}

void
Application::setThrottleEnabled(bool enabled, bool updateUi)
{
  this->prop.throttle = enabled;

  if (this->state == RUNNING)
    this->analyzer.get()->setThrottle(
          this->prop.throttle
          ? this->prop.efSampRate
          : this->currSampleRate);

  if (updateUi)
    this->ui->cbThrottle->setChecked(enabled);
}

void
Application::setThrottleValue(unsigned int value, bool updateUi)
{
  this->prop.efSampRate = value;

  if (this->state == RUNNING)
    this->analyzer.get()->setThrottle(
          this->prop.throttle
          ? this->prop.efSampRate
          : this->currSampleRate);

  if (updateUi)
    this->ui->sbThrottleValue->setValue(static_cast<int>(value));
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

// TODO: GROUP ALL
void
Application::onSwPropChanged(int)
{
  this->setSpectrumWaterfallProportion(
        static_cast<float>(this->ui->sWRatio->value()) / 100.f,
        false);

  this->setSpectrumMinDb(
        this->ui->sFloor->value(),
        false);

  this->setSpectrumMaxDb(
        this->ui->sFloor->value()
        + this->ui->sRange->value(),
        false);
}

void
Application::onToggleLockPandapter(int state)
{
  this->setPandapterLocked(state == Qt::Checked, false);
}

void
Application::onTogglePeakHold(int state)
{
  this->setPeakHold(state == Qt::Checked, false);
}

void
Application::onChirp(const EchoDetector::Chirp &chirp)
{
  int lastRow;

  this->chirpModel->pushChirp(chirp);

  lastRow = this->chirpModel->rowCount() - 1;
  this->ui->eventTable->scrollTo(
        this->chirpModel->index(lastRow, 0));
  this->ui->eventTable->selectRow(lastRow);
}

void
Application::refreshSelection(void)
{
  QModelIndexList selected =
      this->ui->eventTable->selectionModel()->selectedRows();

  if (selected.count() == 1) {
    const EchoDetector::Chirp &chirp =
        this->chirpModel->at(
          static_cast<unsigned long>(selected.at(0).row()));
    this->currChirp = &chirp;
    this->updateChirpCharts(chirp);
  } else {
    this->currChirp = nullptr;
  }

  ui->pbSaveChirp->setEnabled(this->currChirp != nullptr);
  ui->pbSaveDoppler->setEnabled(this->currChirp != nullptr);
  ui->pbSavePower->setEnabled(this->currChirp != nullptr);
  ui->actionSave->setEnabled(this->currChirp != nullptr);
}

void
Application::onChirpSelected(const QItemSelection &, const QItemSelection &)
{
  this->refreshSelection();
}

void
Application::onThrottleChanged(void)
{
  this->setThrottleEnabled(this->ui->cbThrottle->isChecked(), false);
  this->setThrottleValue(
        static_cast<unsigned int>(this->ui->sbThrottleValue->value()), false);
  this->ui->sbThrottleValue->setEnabled(this->prop.throttle);
}

void
Application::onClearEventTable(void)
{
  if (this->chirpModel->rowCount() > 0) {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
          this,
          "Clear event table",
          "You are about to delete "
          + QString::number(this->chirpModel->rowCount())
          + " events from the event table. "
          + "This action cannot be undone. "
          + "Are you sure?",
          QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
      this->chirpModel->clear();
      this->refreshSelection();
    }
  }
}

void
Application::onSaveDopplerPlot(void)
{
  QString fileName = QFileDialog::getSaveFileName(
      this,
      "Save Doppler plot",
      "",
      "PNG image (*.png);;All Files (*)");

  if (!fileName.isEmpty()) {
    if (!saveChartView(this->dopplerView, fileName)) {
      QMessageBox::critical(
            this,
            "Save Doppler plot",
            "Failed to save plot in the specified location. Please try again.",
            QMessageBox::Ok);
    }
  }
}

void
Application::onSaveChirpPlot(void)
{
  QString fileName = QFileDialog::getSaveFileName(
      this,
      "Save Chirp plot",
      "",
      "PNG image (*.png);;All Files (*)");

  if (!fileName.isEmpty()) {
    if (!saveChartView(this->chirpView, fileName)) {
      QMessageBox::critical(
            this,
            "Save Chirp plot",
            "Failed to save plot in the specified location. Please try again.",
            QMessageBox::Ok);
    }
  }
}

void
Application::onSavePowerPlot(void)
{
  QString fileName = QFileDialog::getSaveFileName(
      this,
      "Save Power plot",
      "",
      "PNG image (*.png);;All Files (*)");

  if (!fileName.isEmpty()) {
    if (!saveChartView(this->pwpView, fileName)) {
      QMessageBox::critical(
            this,
            "Save Power plot",
            "Failed to save plot in the specified location. Please try again.",
            QMessageBox::Ok);
    }
  }
}

void
Application::onSaveDoppler(void)
{
  QString fileName = QFileDialog::getSaveFileName(
      this,
      "Save Doppler data",
      "",
      "MATLAB / Octave files (*.m);;All Files (*)");

  if (!fileName.isEmpty()) {
    if (!saveChirpData(
          this->currChirp,
          fileName,
          EchoDetector::Chirp::SCALARS
          | EchoDetector::Chirp::DOPPLER)) {
      QMessageBox::critical(
            this,
            "Save Doppler data",
            "Failed to save data in the specified location. Please try again.",
            QMessageBox::Ok);
    }
  }
}

void
Application::onSaveChirp(void)
{
  QString fileName = QFileDialog::getSaveFileName(
      this,
      "Save chirp samples",
      "",
      "MATLAB / Octave files (*.m);;All Files (*)");

  if (!fileName.isEmpty()) {
    if (!saveChirpData(
          this->currChirp,
          fileName,
          EchoDetector::Chirp::SCALARS
          | EchoDetector::Chirp::SAMPLES)) {
      QMessageBox::critical(
            this,
            "Save chirp samples",
            "Failed to save data in the specified location. Please try again.",
            QMessageBox::Ok);
    }
  }
}

void
Application::onSavePower(void)
{
  QString fileName = QFileDialog::getSaveFileName(
      this,
      "Save power data",
      "",
      "MATLAB / Octave files (*.m);;All Files (*)");

  if (!fileName.isEmpty()) {
    if (!saveChirpData(
          this->currChirp,
          fileName,
          EchoDetector::Chirp::SCALARS
          | EchoDetector::Chirp::POWER_NARROW
          | EchoDetector::Chirp::POWER_WIDE)) {
      QMessageBox::critical(
            this,
            "Save power data",
            "Failed to save data in the specified location. Please try again.",
            QMessageBox::Ok);
    }
  }
}

void
Application::onSaveFullChirpData(void)
{
  QString fileName = QFileDialog::getSaveFileName(
      this,
      "Save full chirp data",
      "",
      "MATLAB / Octave files (*.m);;All Files (*)");

  if (!fileName.isEmpty()) {
    if (!saveChirpData(
          this->currChirp,
          fileName,
          EchoDetector::Chirp::SCALARS
          | EchoDetector::Chirp::SAMPLES
          | EchoDetector::Chirp::DOPPLER
          | EchoDetector::Chirp::SNR
          | EchoDetector::Chirp::POWER_NARROW
          | EchoDetector::Chirp::POWER_WIDE)) {
      QMessageBox::critical(
            this,
            "Save full chirp data",
            "Failed to save data in the specified location. Please try again.",
            QMessageBox::Ok);
    }
  }
}

Application::~Application()
{
  // Ensure analyzer is properly stopped
  printf("Analyzer destruction\n");
  this->analyzer = nullptr;
  delete this->ui;
}
