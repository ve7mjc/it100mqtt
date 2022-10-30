#ifndef IT100_H
#define IT100_H

#include <QDateTime>
#include <QTimer>

#include <QtNetwork>

#include <QStringList>
#include <QTimer>
#include <QDateTime>

#include "it100message.h"
#include "commonservice.h"

namespace it100 {

Q_NAMESPACE

inline static const int socketDataReceiveTimeoutSecs = 30;
inline static const int socketConnectRetrySecs = 5;

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

// these correspond to various it100 based commands
enum UserEventType {
    UserKeypadLockout, // 658 Keypad Lock-Out (Partition; NO USER)
    UserInvalidAccessCode, // 670 Invalid Access Code (Partion; NO USER)
    UserClosing, // 700 User Closing (Partition + User)
    UserOpening, // 750 User Opening (Partition + User)
};
Q_ENUM_NS(UserEventType)

class IT100 : public QObject
{
    Q_OBJECT
public:
    
    IT100(InterfaceType interfaceType, bool debugMode = false);

    ComponentStatus getStatus() { return status; }

    static QString userEventTypeToString(UserEventType type);

    void open(QHostAddress remoteAddr, quint16 port);
    void open();
    bool isConnected() { return _connected; }
    bool setCommunicationsGood(bool state);

    bool setUserCode(uint32_t code);
    bool setProgrammerCode(uint32_t code);

    // send command to IT-100 module
    void sendCommand(QByteArray command, QByteArray data);
    void sendCommand(QByteArray command, uint16_t data);
    void sendCommand(QByteArray command);

    void setRemoteHostAddress(QHostAddress address);
    void setRemoteHostPort(uint16_t port);
    void setPanelUserCode(uint32_t code) { panelUserCode = code; }

    int setZoneFriendlyName(int zoneNumber, QString name);
    QString getZoneFriendlyName(int zoneNumber);

    void armStay(int partition = 1);
    void armAway(int partition = 1);
    void disarm(int partition = 1);
    void setEnableVirtualKeypad(bool value = true);

    bool isWaitingForStatusUpdate();

    QHostAddress remoteHostAddress;
    uint16_t remoteHostPort;

    QString zoneFriendlyNames[64];
    QString lcdDisplayContents;

    InterfaceType interfaceType;

    QDateTime lastTransmittedCommsAt; // When serial comms last sent to IT-100
    QDateTime lastReceivedCommsAt; // When serial comms last received

private:

    void writePacket();

    // generate DSC style checksum from command+data bytes
    QByteArray generateChecksum(QByteArray data);

    // build packet from commmand+data bytes
    QByteArray generatePacket(QByteArray command, QByteArray data);

    int processReceivedLine(QByteArray data);

    ComponentStatus status = COMP_STATUS_UNKNOWN;

    QTcpSocket *socket = nullptr;
    bool _connected;
    bool _connectionIntent;
    int _connectionAttempts;
    bool communicationsGood;
    bool waitingForResponse;
    bool _waitingForStatusUpdate;
    bool _debugMode;

    QList<It100Message*> messageQueueOut;

    // QTimers
    QTimer *pollTimer = nullptr;
    QTimer *timedisciplineTimer = nullptr;
    QTimer *communicationsTimeoutTimer = nullptr;
    QTimer *moduleReconnectTimer = nullptr;

    QString receivedData;

    uint32_t panelUserCode;
    uint32_t panelProgrammerCode;

private slots:

    void processTcpSocketReadyRead();
    void onPollTimerTimeout();
    void processTcpSocketConnected();
    void onTimeDisciplineTimerTimeout();
    void onCommunicationsTimeoutTimerTimeout();
    void processModuleReconnectTimerTimeout();
    void processTcpSocketStateChange(QAbstractSocket::SocketState);
    void processTcpConnectRetryTimeout();

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

    /** user -1 when unknown */
    void userEvent(UserEventType type, int partition, int user);

    void zoneOpen(int partition, int zone);
    void zoneRestored(int partition, int zone);
    void zoneStatusChanged(int zone, int partition, ZoneStatus status);
    void partitionStatusChanged(int partition, PartitionStatus status, int zone = 0);
    void partitionArmedDescriptive(int partition, PartitionArmedMode mode);
    void troubleEvent(TroubleEvent event);

    /** an access code that was entered is invalid */
    void invalidAccessCode(int partition);

    /** indicates a partition has been armed by a user
        note: this is sent after exit_delay */
    void userClosing(int partition, int userCode);
    
    /** indicates partition has been armed by one of the following methods:
        Quick Arm, Auto Arm, Keysiwtch, DLS Software, or Wireless Key */
    void specialClosing(int partition);

    /** indicates partition has been armed but one or more zones have 
        been bypassed */
    void partialClosing(int partition);

    /** indicates partition has been opened by a user */
    void userOpening(int partition, int userCode);

    /** indicates partition has been disarmed by one of following methods:
        Keyswitch, DLS Software, Wireless Key */
    void specialOpening(int partition);


    void virtualKeypadDisplayUpdate();

};

} // namespace it100

#endif // IT100_H
