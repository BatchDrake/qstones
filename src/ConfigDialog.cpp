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

#include <QFileDialog>
#include <QMessageBox>

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
  connect(
        this->ui->rbAudio,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onRadioButtonToggle(bool)));

  connect(
        this->ui->rbIq,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onRadioButtonToggle(bool)));

  connect(
        this->ui->rbWav,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onRadioButtonToggle(bool)));

  connect(
        this->ui->rbSuscan,
        SIGNAL(toggled(bool)),
        this,
        SLOT(onRadioButtonToggle(bool)));

  connect(
        this->ui->pbBrowseIq,
        SIGNAL(clicked()),
        this,
        SLOT(onBrowseIqClicked(void)));

  connect(
        this->ui->pbBrowseWav,
        SIGNAL(clicked()),
        this,
        SLOT(onBrowseWavClicked(void)));
}

Suscan::Source::Config
ConfigDialog::getProfile(void)
{
  if (this->ui->rbAudio->isChecked()) {
    this->audioProfile.setSampleRate(this->ui->leAudioSampRate->text().toUInt());
    return this->audioProfile;
  } else if (this->ui->rbIq->isChecked()) {
    this->iqProfile.setSampleRate(this->ui->leIqSampRate->text().toUInt());
    return this->iqProfile;
  } else if (this->ui->rbWav->isChecked()) {
    return this->wavProfile;
  } else {
    QVariant data = this->ui->profileCombo->itemData(this->ui->profileCombo->currentIndex());
    return data.value<Suscan::Source::Config>();
  }
}

void
ConfigDialog::setAudioDevice(const suscan_source_device_t *dev)
{
  this->audioProfile.setSDRDevice(dev);
  this->audio_detected = true;
}

SUPRIVATE SUBOOL
find_audio_device(
    const suscan_source_device_t *dev,
    unsigned int,
    void *privdata)
{
  ConfigDialog *dialog = static_cast<ConfigDialog *>(privdata);
  const char *devstr;

  devstr = SoapySDRKwargs_get(dev->args, "driver");
  if (devstr != nullptr && strcmp(devstr, "audio") == 0) {
    dialog->setAudioDevice(dev);
    return SU_FALSE;
  }

  return SU_TRUE;
}

ConfigDialog::ConfigDialog(QWidget *parent) : QDialog(parent)
{
  this->ui = new Ui_Config();
  this->ui->setupUi(this);

  // Detect audio
  suscan_source_device_walk(find_audio_device, this);
  if (!this->audio_detected) {
    QMessageBox::warning(
          this,
          "No suitable audio device found",
          "Failed to detect soundcard device via SoapySDR. "
          "Live audio capture will be disabled.",
          QMessageBox::Ok);
    this->ui->rbAudio->setEnabled(false);
    this->ui->rbWav->setChecked(true);
  }

  // Setup integer validators
  this->ui->leAudioDevice->setValidator(new QIntValidator(0, 100, this));
  this->ui->leAudioSampRate->setValidator(new QIntValidator(1, 100000, this));
  this->ui->leIqSampRate->setValidator(new QIntValidator(1, 1000000, this));

  this->audioProfile = Suscan::Source::Config(
        SUSCAN_SOURCE_TYPE_SDR,
        SUSCAN_SOURCE_FORMAT_AUTO);
  this->audioProfile.setLabel("Soundcard");

  this->wavProfile = Suscan::Source::Config(
        SUSCAN_SOURCE_TYPE_FILE,
        SUSCAN_SOURCE_FORMAT_WAV);
  this->wavProfile.setLabel("WAV file");

  this->iqProfile = Suscan::Source::Config(
        SUSCAN_SOURCE_TYPE_FILE,
        SUSCAN_SOURCE_FORMAT_RAW);
  this->iqProfile.setLabel("I/Q file");

  this->setWindowTitle("Signal source configuration");

  // Populate this dialog with profiles
  this->populateProfiles();
  this->connectAll();

  // Act as something was clicked
  this->onRadioButtonToggle(false);
}

void
ConfigDialog::onRadioButtonToggle(bool)
{
  this->ui->leAudioSampRate->setEnabled(this->ui->rbAudio->isChecked());
  this->ui->leAudioDevice->setEnabled(false);

  this->ui->pbBrowseWav->setEnabled(this->ui->rbWav->isChecked());

  this->ui->pbBrowseIq->setEnabled(this->ui->rbIq->isChecked());
  this->ui->leIqSampRate->setEnabled(this->ui->rbIq->isChecked());

  this->ui->profileCombo->setEnabled(this->ui->rbSuscan->isChecked());
}

void
ConfigDialog::onBrowseIqClicked(void)
{
  QString path = QFileDialog::getOpenFileName(
        this,
        "Open I/Q file",
        QString(),
        "IQ files (*.raw);;All files (*)");

  if (!path.isEmpty()) {
    this->ui->lIqPath->setText(path);
    this->iqProfile.setPath(path.toStdString());
  }
}

void
ConfigDialog::onBrowseWavClicked(void)
{
  QString path = QFileDialog::getOpenFileName(
        this,
        "Open WAV file",
        QString(),
        "WAV files (*.wav);;All files (*)");

  if (!path.isEmpty()) {
    this->ui->lWavPath->setText(path);
    this->wavProfile.setPath(path.toStdString());
  }
}

ConfigDialog::~ConfigDialog()
{
    delete this->ui;
}

