#ifndef IT100MQTT_H
#define IT100MQTT_H

#ifndef QMQTT_LIBRARY
#define QMQTT_LIBRARY
#endif

// #ifdef USE_SYSTEMD
#include <systemd/sd-daemon.h>
#include <fcntl.h>
#include <time.h>
// #endif

#include <QLoggingCategory>

#include "graylog.h"
#include "it100.h"
#include "alarmpanel.h"
#include <qmqtt/qmqtt.h>

#include <QCoreApplication>

#include "commonservice.h"

#include <QObject>
#include <QTimer>
#include <QSettings>
#include <QMap>

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

    it100::IT100 *it100 = nullptr;
    
    bool failed() { return _failed; }

    // state of service
    // it100 communicating correctly? -- not timed out?
    // MQTT connected -- not timed out?

    void updateServiceStatus();

private:

    QString nameFromUserCodeSlot(int32_t user);
    bool loadUserSlots();

    bool _failed = false;

    AlarmPanel panel;

    Graylog *graylog;

    QTimer *testTimer;
//    QSettings *settings;

    quint16 mId;

    // Settings
    bool debugMode;
    QHostAddress it100RemoteHost;
    quint16 it100RemotePort;
    QString it100UserCode;
    QHostAddress m_mqttRemoteHost;
    quint16 m_mqttRemotePort;

    QMap<int,QString> userSlots;
    QString configFile;

    ComponentStatus mqttStatus = COMP_STATUS_UNKNOWN;

signals:

public slots:

    void onMqttConnect();
    void onMqttDisconnected();
    void reconnectTimerTimeout();

    void onMqttMessageReceived(QMQTT::Message);
    void onTestTimerTimeout();

    void onIt100Connected();
    void onIt100Disconnected();
    void onIt100ZoneStatusChange(int16_t zone, int16_t partition, it100::ZoneStatus status);
    void processIt100UserEvent(it100::UserEventType type, int16_t partition, int16_t user);
    void onIt100PartitionStatusChange(int16_t partition, it100::PartitionStatus status);
    void onIt100PartitionArmedDescriptive(int16_t partition, it100::PartitionArmedMode mode);
    void onIt100TroubleEvent(it100::TroubleEvent event);
    void onIt100VirtualKeypadDisplayUpdate();
    void onIt100CommunicationsBegin();
    void onIt100CommunicationsTimeout();

};

#endif // IT100MQTT_H
