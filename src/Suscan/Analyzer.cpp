//
//    Analyzer.cpp: Analyzer wrapper implementation
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

#include <Suscan/Analyzer.h>

using namespace Suscan;

// Async thread
void
Analyzer::AsyncThread::run()
{
  void *data = nullptr;
  uint32_t type;
  bool running = true;

  // FIXME: Capture allocation exceptions!
  do {
    data = this->owner->read(type);

    switch (type) {
      // Data messages
      case SUSCAN_ANALYZER_MESSAGE_TYPE_CHANNEL:
        emit message(ChannelMessage(static_cast<struct suscan_analyzer_channel_msg *>(data)));
        break;

      case SUSCAN_ANALYZER_MESSAGE_TYPE_INSPECTOR:
        emit message(InspectorMessage(static_cast<struct suscan_analyzer_inspector_msg *>(data)));
        break;

      case SUSCAN_ANALYZER_MESSAGE_TYPE_PSD:
        emit message(PSDMessage(static_cast<struct suscan_analyzer_psd_msg *>(data)));
        break;

      case SUSCAN_ANALYZER_MESSAGE_TYPE_SAMPLES:
        emit message(SamplesMessage(static_cast<struct suscan_analyzer_sample_batch_msg *>(data)));
        break;

      // Exit conditions
      case SUSCAN_WORKER_MSG_TYPE_HALT:
        emit halted();
        running = false;
        break;

      case SUSCAN_ANALYZER_MESSAGE_TYPE_EOS:
        emit eos();
        running = false;
        break;

      case SUSCAN_ANALYZER_MESSAGE_TYPE_READ_ERROR:
        emit read_error();
        running = false;
        break;

      default:
        // Everything else is disposed
        suscan_analyzer_dispose_message(type, data);
        data = nullptr;
    }
  } while (running);

  suscan_analyzer_dispose_message(type, data);
}

Analyzer::AsyncThread::AsyncThread(Analyzer *owner)
{
  this->owner = owner;
}

// Analyzer object
void *
Analyzer::read(uint32_t &type)
{
  return suscan_analyzer_read(this->instance, &type);
}


// Signal slots
void
Analyzer::captureMessage(Message)
{
  // TODO: implement me!
}

// Object construction and destruction
Analyzer::Analyzer(
    struct suscan_analyzer_params const& params,
    Source::Config const& config)
{
  SU_ATTEMPT(this->instance = suscan_analyzer_new(
        &params,
        config.instance,
        &mq.mq));

  this->asyncThread = std::make_unique<AsyncThread>(this);

  // TODO: Connect signals
}

Analyzer::~Analyzer()
{
  if (this->instance != nullptr)
    suscan_analyzer_destroy(this->instance);
}

