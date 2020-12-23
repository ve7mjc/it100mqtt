#ifndef IT100_H
#define IT100_H

#include <QObject>
#include <QDateTime>
#include <QTimer>

#include <QtNetwork>

#include <QStringList>
#include <QTimer>
#include <QDateTime>
#include <QDebug>

#include "it100message.h"

static const quint8 it100CommunicationsTimeoutTime = 30; // >= 5
static const quint8 it100TcpConnecionRetryTimeSeconds = 5;

enum InterfaceType {
    IFACE_RS232 = 0,
    IFACE_IPSERIAL = 1
};

enum ZoneStatus {
    ZONE_STATUS_OPEN,
    ZONE_STATUS_RESTORED,
    ZONE_STATUS_TAMPER,
    ZONE_STATUS_TAMPER_RESTORED,
    ZONE_STATUS_FAULT,
    ZONE_STATUS_FAULT_RESTORED,
    ZONE_STATUS_ALARM,
    ZONE_STATUS_ALARM_RESTORED
};

enum PartitionStatus {
    PARTITION_STATUS_ARMED,
    PARTITION_STATUS_DISARMED,
    PARTITION_STATUS_ALARM,
    PARTITION_STATUS_RESTORE,
    PARTITION_STATUS_READY,
    PARTITION_STATUS_NOT_READY,
    PARTITION_STATUS_READY_FORCE_ARM,
    PARTITION_STATUS_EXIT_DELAY_IN_PROGRESS,
    PARTITION_STATUS_ENTRY_DELAY_IN_PROGRESS,
    PARTITION_STATUS_USER_CLOSING,
    PARTITION_STATUS_PARTIAL_CLOSING,
    PARTITION_STATUS_SPECIAL_CLOSING,
    PARTITION_STATUS_INVALID_ACCESS_CODE,
    PARTITION_STATUS_FUNCTION_NOT_AVAILABLE,
    PARTITION_STATUS_BUSY
};

enum PartitionArmedMode {
    PARTITION_ARMED_AWAY = 0,
    PARTITION_ARMED_STAY = 1,
    PARTITION_ARMED_AWAY_NODELAY = 2,
    PARTITION_ARMED_STAY_NODELAY = 3
};

enum TroubleEvent {
    TROUBLE_PANEL_BATTERY,
    TROUBLE_PANEL_BATTERY_RESTORE,
    TROUBLE_PANEL_AC,
    TROUBLE_PANEL_AC_RESTORE,
    TROUBLE_SYSTEM_BELL,
    TROUBLE_SYSTEM_BELL_RESTORE,
    TROUBLE_TLM_1,
    TROUBLE_TLM_1_RESTORE,
    TROUBLE_TLM_2,
    TROUBLE_TLM_2_RESTORE,
    TROUBLE_FTC,
    TROUBLE_BUFFER_NEAR_FULL,
    TROUBLE_GENERAL_DEVICE_LOW_BATTERY,
    TROUBLE_GENERAL_DEVICE_LOW_BATTERY_RESTORE,
    TROUBLE_GENERAL_SYSTEM_TAMPER,
    TROUBLE_GENERAL_SYSTEM_TAMPER_RESTORE,
    TROUBLE_WIRELESS_KEY_LOW_BATTERY,
    TROUBLE_WIRELESS_KEY_LOW_BATTERY_RESTORE,
    TROUBLE_HANDHELD_KEYPAD_LOW_BATTERY,
    TROUBLE_HANDHELD_KEYPAD_LOW_BATTERY_RESTORE,
    TROUBLE_HOME_AUTOMATION,
    TROUBLE_HOME_AUTOMATION_RESTORE
};

class IT100 : public QObject
{
    Q_OBJECT
public:
    
    IT100(InterfaceType interfaceType, bool debugMode = false);

    InterfaceType interfaceType;

    QDateTime lastTransmittedCommsAt; // When serial comms last sent to IT-100
    QDateTime lastReceivedCommsAt; // When serial comms last received

    void open(QHostAddress remoteAddr, quint16 port);
    void open();
    bool isConnected() { return _connected; }
    bool setCommunicationsGood(bool state);

    bool setUserCode(quint32 code);
    bool setProgrammerCode(quint32 code);

    // send command to IT-100 module
    void sendCommand(QByteArray command, QByteArray data);
    void sendCommand(QByteArray command, quint16 data);
    void sendCommand(QByteArray command);

    void setRemoteHostAddress(QHostAddress address);
    void setRemoteHostPort(quint16 port);
    void setPanelUserCode(quint16 code) { panelUserCode = code; }

    int setZoneFriendlyName(int zoneNumber, QString name);
    QString getZoneFriendlyName(int zoneNumber);

    void armStay(quint8 partition = 1);
    void armAway(quint8 partition = 1);
    void disarm(quint8 partition = 1);
    void virtualKeypadEnable(bool value = true);

    bool isWaitingForStatusUpdate();

    QHostAddress remoteHostAddress;
    quint16 remoteHostPort;

    QString zoneFriendlyNames[64];
    QString lcdDisplayContents;

    /**
      * Application Originated Commands
      */
    static const QByteArray CMD_POLL;
    static const QByteArray CMD_STATUS_REQUEST;
    static const QByteArray CMD_LABELS_REQUEST;
    static const QByteArray CMD_SET_TIME_AND_DATE;
    static const QByteArray CMD_COMMAND_OUTPUT_CONTROL;
    static const QByteArray CMD_PARTITION_ARM_CONTROL_AWAY;
    static const QByteArray CMD_PARTITION_ARM_CONTROL_STAY;
    static const QByteArray CMD_PARTITION_ARM_CONTROL_ARMED_NO_ENTRY_DELAY;
    static const QByteArray CMD_PARTITION_ARM_CONTROL_WITH_CODE;
    static const QByteArray CMD_PARTITION_DISARM_CONTROL_WITH_CODE;
    static const QByteArray CMD_TIME_STAMP_CONTROL;
    static const QByteArray CMD_TIME_DATE_BROADCAST_CONTROL;
    static const QByteArray CMD_TEMPERATURE_BROADCAST_CONTROL;
    static const QByteArray CMD_VIRTUAL_KEYPAD_CONTROL;
    static const QByteArray CMD_TRIGGER_PANIC_ALARM;
    static const QByteArray CMD_KEY_PRESSED_VIRT;
    static const QByteArray CMD_BAUD_RATE_CHANGE;
    static const QByteArray CMD_GET_TEMPERATURE_SET_POINT;
    static const QByteArray CMD_TEMPERATURE_CHANGE;
    static const QByteArray CMD_SAVE_TEMPERATURE_SETTING;
    static const QByteArray CMD_CODE_SEND;

    /**
      * IT-100 Originated Commands
      */
    static const QByteArray CMD_COMMAND_ACKNOWLEDGE;
    static const QByteArray CMD_COMMAND_ERROR;
    static const QByteArray CMD_SYSTEM_ERROR;
    static const QByteArray CMD_TIME_DATE_BROADCAST;
    static const QByteArray CMD_RING_DETECETD;
    static const QByteArray CMD_INDOOR_TEMPERATURE_BROADCAST;
    static const QByteArray CMD_OUTDOOR_TEMPERATURE_BROADCAST;
    static const QByteArray CMD_THERMOSTAT_SET_POINTS;
    static const QByteArray CMD_BROADCAST_LABELS;
    static const QByteArray CMD_BAUD_RATE_SET;
    static const QByteArray CMD_ZONE_ALARM;
    static const QByteArray CMD_ZONE_ALARM_RESTORE;
    static const QByteArray CMD_ZONE_TAMPER;
    static const QByteArray CMD_ZONE_TAMPER_RESTORE;
    static const QByteArray CMD_ZONE_FAULT;
    static const QByteArray CMD_ZONE_FAULT_RESTORE;
    static const QByteArray CMD_ZONE_OPEN;
    static const QByteArray CMD_ZONE_RESTORED;
    static const QByteArray CMD_DURESS_ALARM;
    static const QByteArray CMD_F_KEY_ALARM;
    static const QByteArray CMD_F_KEY_RESTORAL;
    static const QByteArray CMD_A_KEY_ALARM;
    static const QByteArray CMD_A_KEY_RESTORAL;
    static const QByteArray CMD_P_KEY_ALARM;
    static const QByteArray CMD_P_KEY_RESTORAL;
    static const QByteArray CMD_AUXILIARY_INPUT_ALARM;
    static const QByteArray CMD_AUXILIARY_INPUT_ALARM_RESTORED;
    static const QByteArray CMD_PARTITION_READY;
    static const QByteArray CMD_PARTITION_NOT_READY;
    static const QByteArray CMD_PARTITION_ARMED_DESCRIPTIVE_MODE;
    static const QByteArray CMD_PARTITION_IN_READY_TO_FORCE_ALARM;
    static const QByteArray CMD_PARTITION_IN_ALARM;
    static const QByteArray CMD_PARTITION_DISARMED;
    static const QByteArray CMD_EXIT_DELAY_IN_PROGRESS;
    static const QByteArray CMD_ENTRY_DELAY_IN_PROGRESS;
    static const QByteArray CMD_KEYPAD_LOCKOUT;
    static const QByteArray CMD_KEYPAD_BLANKING;
    static const QByteArray CMD_COMMAND_OUTPUT_IN_PROGRESS;
    static const QByteArray CMD_INVALID_ACCESS_CODE;
    static const QByteArray CMD_FUNCTION_NOT_AVAILABLE;
    static const QByteArray CMD_FAIL_TO_ARM;
    static const QByteArray CMD_PARTITION_BUSY;
    static const QByteArray CMD_USER_CLOSING;
    static const QByteArray CMD_SPECIAL_CLOSING;
    static const QByteArray CMD_PARTIAL_CLOSING;
    static const QByteArray CMD_USER_OPENING;
    static const QByteArray CMD_SPECIAL_OPENING;
    static const QByteArray CMD_PANEL_BATTERY_TROUBLE;
    static const QByteArray CMD_PANEL_BATTERY_TROUBLE_RESTORE;
    static const QByteArray CMD_PANEL_AC_TROUBLE;
    static const QByteArray CMD_PANEL_AC_RESTORE;
    static const QByteArray CMD_SYSTEM_BELL_TROUBLE;
    static const QByteArray CMD_SYSTEM_BELL_TROUBLE_RESTORAL;
    static const QByteArray CMD_TLM_LINE_1_TROUBLE;
    static const QByteArray CMD_TLM_LINE_1_TROUBLE_RESTORAL;
    static const QByteArray CMD_TLM_LINE_2_TROUBLE;
    static const QByteArray CMD_TLM_LINE_2_TROUBLE_RESTORAL;
    static const QByteArray CMD_FTC_TROUBLE;
    static const QByteArray CMD_BUFFER_NEAR_FULL;
    static const QByteArray CMD_GENERAL_DEVICE_LOW_BATTERY;
    static const QByteArray CMD_GENERAL_DEVICE_LOW_BATTERY_RESTORE;
    static const QByteArray CMD_WIRELESS_KEY_LOW_BATTERY_TROUBLE;
    static const QByteArray CMD_WIRELESS_KEY_LOW_BATTERY_TROUBLE_RESTORE;
    static const QByteArray CMD_HANDHELD_KEYPAD_LOW_BATTERY_TROUBLE;
    static const QByteArray CMD_HANDHELD_KEYPAD_LOW_BATTERY_TROUBLE_RESTORE;
    static const QByteArray CMD_GENERAL_SYSTEM_TAMPER;
    static const QByteArray CMD_GENERAL_SYSTEM_TAMPER_RESTORE;
    static const QByteArray CMD_HOME_AUTOMATION_TROUBLE;
    static const QByteArray CMD_HOME_AUTOMATION_TROUBLE_RESTORE;
    static const QByteArray CMD_TROUBLE_STATUS_LED_ON;
    static const QByteArray CMD_TROUBLE_STATUS_LED_OFF;
    static const QByteArray CMD_FIRE_TROUBLE_ALARM;
    static const QByteArray CMD_FIRE_TROUBLE_ALARM_RESTORE;
    static const QByteArray CMD_CODE_REQUIRED;
    static const QByteArray CMD_LCD_UPDATE;
    static const QByteArray CMD_LCD_CURSOR;
    static const QByteArray CMD_LED_STATUS;
    static const QByteArray CMD_BEEP_STATUS;
    static const QByteArray CMD_TONE_STATUS;
    static const QByteArray CMD_BUZZER_STATUS;
    static const QByteArray CMD_DOOR_CHIME_STATUS;
    static const QByteArray CMD_SOFTWARE_VERSION;


private:

    QTcpSocket *socket;
    bool _connected;
    bool _connectionIntent;
    int _connectionAttempts;
    bool communicationsGood;
    bool waitingForResponse;
    bool _waitingForStatusUpdate;
    bool _debugMode;

    QList<It100Message*> messageQueueOut;

    quint32 panelUserCode;
    quint32 panelProgrammerCode;

    void writePacket();

    QString receivedData;
    int processReceivedLine(QByteArray data);

    // generate DSC style checksum from command+data bytes
    QByteArray generateChecksum(QByteArray data);

    // build packet from commmand+data bytes
    QByteArray generatePacket(QByteArray command, QByteArray data);

    // QTimers
    QTimer *pollTimer;
    QTimer *timedisciplineTimer;
    QTimer *communicationsTimeoutTimer;
    QTimer *moduleReconnectTimer;

private slots:

    void onDataAvailable();
    void onPollTimerTimeout();
    void onConnected();
    void onTimeDisciplineTimerTimeout();
    void onCommunicationsTimeoutTimerTimeout();
    void on_moduleReconnectTimerTimeout();
    void onTcpSocketStateChanged(QAbstractSocket::SocketState);
    void onTcpConnectRetryTimeout();

signals:

    void connected();
    void disconnected();

    /**
     * Signal called when data is received from the serial port.
     * This signal is line based, data is grouped by line and a signal
     * is emitted for each line.
     * \param data the line of text just received.
     */
    void lineReceived(QString data);

    /**
     * Signal called when data is sent to the serial port.
     */
    void lineSent(QString data);

    /**
     * Signal called when valid serial communications has begun
     * with IT-100 module
     */
    void communicationsBegin();

    /**
      * Signal called when serial timeout is determined
      * This would indicate failure of IT-100 module or
      * loss of comms.
      */
    void communicationsTimeout();

    void zoneOpen(quint8 partition, quint8 zone);
    void zoneRestored(quint8 partition, quint8 zone);
    void zoneStatusChanged(quint8 zone, quint8 partition, ZoneStatus status);
    void partitionStatusChanged(quint8 partition, PartitionStatus status, quint8 zone = 0);
    void partitionArmedDescriptive(quint8 partition, PartitionArmedMode mode);
    void troubleEvent(TroubleEvent event);

    void virtualKeypadDisplayUpdate();

};



#endif // IT100_H
