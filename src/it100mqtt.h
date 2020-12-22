#ifndef IT100MQTT_H
#define IT100MQTT_H

#ifndef QMQTT_LIBRARY
#define QMQTT_LIBRARY
#endif

#include <QLoggingCategory>

#include "graylog.h"
#include "it100.h"
#include "alarmpanel.h"
#include <qmqtt/qmqtt.h>

#include <QCoreApplication>

enum LogLevel {
    LOG_LEVEL_ERROR,
    LOG_LEVEL_SECURITY,
    LOG_LEVEL_NOTICE,
    LOG_LEVEL_DEBUG
};

enum QosLevel {
    QOS_0 = 0,
    QOS_1 = 1,
    QOS_2 = 2
};

#include <QDebug>
#include <QObject>
#include <QTimer>
#include <QSettings>

class It100Mqtt : public QObject
{
    Q_OBJECT
public:
    explicit It100Mqtt(QString settingsFile, QObject *parent = 0);

    int connectToIt100(QHostAddress host, qint16 port = 4001);
    int connectToMqttBroker(QHostAddress host, qint16 port = 1883);

    void writeLog(QString msg, LogLevel level = LOG_LEVEL_DEBUG);
    void writeLog(const char *msg, LogLevel level = LOG_LEVEL_DEBUG);
    void writeMqtt(const char *topic, const char *message, QosLevel qos = QOS_0, bool retain = false);
    void writeMqtt(const char *topic, QString message, QosLevel qos = QOS_0, bool retain = false);
    void writeMqtt(QString topic, const char *message, QosLevel qos = QOS_0, bool retain = false);
    void writeMqtt(QString topic, QString message, QosLevel qos = QOS_0, bool retain = false);

    QMQTT::Client *client;
    QString mqttClientName;
    QString mqttTopicPrefix; // idac/module/[mqttClientName]/

    IT100 *it100;
    
    bool failed() { return _failed; }


private:

    bool _failed = false;

    AlarmPanel panel;

    Graylog *graylog;

    QTimer *testTimer;
    QSettings *settings;

    quint16 mId;

    // Settings
    bool debugMode;
    QHostAddress it100RemoteHost;
    quint16 it100RemotePort;
    QString it100UserCode;
    QHostAddress m_mqttRemoteHost;
    quint16 m_mqttRemotePort;

signals:

public slots:

    void onMqttConnect();
    void onMqttDisconnected();
    void reconnectTimerTimeout();

    void onMqttMessageReceived(QMQTT::Message);
    void onTestTimerTimeout();

    void onIt100Connected();
    void onIt100Disconnected();
    void onIt100ZoneStatusChange(quint8 zone, quint8 partition, ZoneStatus status);
    void onIt100PartitionStatusChange(quint8 partition, PartitionStatus status);
    void onIt100PartitionArmedDescriptive(quint8 partition, PartitionArmedMode mode);
    void onIt100TroubleEvent(TroubleEvent event);
    void onIt100VirtualKeypadDisplayUpdate();
    void onIt100CommunicationsBegin();
    void onIt100CommunicationsTimeout();

};

#endif // IT100MQTT_H
