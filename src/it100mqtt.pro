QT       += core network
QT       -= gui

TARGET = it100mqtt

CONFIG   += network
CONFIG   -= app_bundle

TEMPLATE = app

unix: DEFINES += USE_SYSTEMD

DEFINES += QMQTT_LIBRARY
include(qmqtt/qmqtt.pri)

LIBS += -lsystemd

SOURCES += main.cpp \
    it100.cpp \
    it100mqtt.cpp \
    it100message.cpp \
    graylog.cpp \
    alarmpanel.cpp

HEADERS += \
    it100.h \
    it100mqtt.h \
    it100message.h \
    graylog.h \
    alarmpanel.h \
    commonservice.h

OTHER_FILES +=

SUBDIRS += \
    qmqtt/qmqtt.pro
