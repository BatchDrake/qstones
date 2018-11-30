#-------------------------------------------------
#
# Project created by QtCreator 2018-11-20T09:19:16
#
#-------------------------------------------------

QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qstones
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++14

INCLUDEPATH += $$PWD/include

SOURCES += \
    src/main.cpp \
    src/Suscan/Analyzer.cpp \
    src/Suscan/Exception.cpp \
    src/Suscan/Source.cpp \
    src/Suscan/MQ.cpp \
    src/Suscan/Library.cpp \
    src/Loader.cpp \
    src/Application.cpp \
    src/ConfigDialog.cpp \
    src/Suscan/Message.cpp \
    src/Suscan/Messages/PSDMessage.cpp \
    src/Suscan/Messages/ChannelMessage.cpp \
    src/Suscan/Messages/InspectorMessage.cpp \
    src/Suscan/Messages/SamplesMessage.cpp \
    src/Gqrx/CPlotter.cpp \
    src/Suscan/Messages/GenericMessage.cpp \
    src/graves/graves.c \
    src/EchoDetector.cpp \
    src/ChirpModel.cpp \
    src/Suscan/Logger.cpp

HEADERS += \
    include/Suscan/Analyzer.h \
    include/Suscan/Compat.h \
    include/Suscan/Source.h \
    include/Suscan/MQ.h \
    include/Suscan/Library.h \
    include/Loader.h \
    include/Application.h \
    include/ConfigDialog.h \
    include/Suscan/Messages/PSDMessage.h \
    include/Suscan/Message.h \
    include/Suscan/Messages/ChannelMessage.h \
    include/Suscan/Messages/InspectorMessage.h \
    include/Suscan/Messages/SamplesMessage.h \
    include/Gqrx/CPlotter.h \
    include/Suscan/Messages/GenericMessage.h \
    include/graves/graves.h \
    include/EchoDetector.h \
    include/Suscan/Logger.h

FORMS += \
    ui/config.ui \
    ui/mainui.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

SUBDIRS += \
    qstones.pro

RESOURCES += \
    icons/oxygen.qrc

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += suscan
