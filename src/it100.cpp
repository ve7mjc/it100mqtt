/*
  DSC IT-100 PowerSeries Integration Module

  */

#include "it100.h"

IT100::IT100(InterfaceType interfaceType, bool debugMode)
{
    
    _debugMode = debugMode;
    
    // we have not yet begun communicating
    _connectionIntent = false;
    _connectionAttempts = 0;
    communicationsGood = false;
    waitingForResponse = false;
    _waitingForStatusUpdate = false;

    // Create TCP Socket
    _connected = false;
    this->socket = new QTcpSocket();
    connect(this->socket, SIGNAL(readyRead()), 
        this, SLOT(onDataAvailable()));
    connect(this->socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(onTcpSocketStateChanged(QAbstractSocket::SocketState)));

    // Module Reconnect Timer
    moduleReconnectTimer = new QTimer();

    // Panel Clock Discipline Timer
    // Keep DSC alarm panel in sync with system time
    timedisciplineTimer = new QTimer(this);
    connect(timedisciplineTimer, SIGNAL(timeout()), 
        this, SLOT(onTimeDisciplineTimerTimeout()));

    // Communications Timeout Timer
    communicationsTimeoutTimer = new QTimer(this);
    communicationsTimeoutTimer->setInterval(it100CommunicationsTimeoutTime * 1000);
    connect(communicationsTimeoutTimer, SIGNAL(timeout()), 
        this, SLOT(onCommunicationsTimeoutTimerTimeout()));
        
    // QTimer Poll Timer
    this->pollTimer = new QTimer(this);
    this->pollTimer->setInterval((it100CommunicationsTimeoutTime-1) * 1000);
    connect(this->pollTimer, SIGNAL(timeout()), 
        this, SLOT(onPollTimerTimeout()));

}

bool IT100::setUserCode(quint32 code)
{
    if ((code > 999) && (code < 1000000)) {
        panelUserCode = code;
        return false;
    } else {
        qDebug() << "acceptable user code not found in config";
        return true;
    }
}

bool IT100::setProgrammerCode(quint32 code)
{
    if ((code > 999) && (code < 1000000)) {
        panelProgrammerCode = code;
        return false;
    } else {
        qDebug() << "acceptable programmer code not found in config";
        return true;
    }
}

/**
  * onDataAvailable()
  */
void IT100::onDataAvailable()
{
    int numBytesAvail = socket->bytesAvailable();
    if (!numBytesAvail) return;
 
    QByteArray receivedBytes;
    receivedBytes.resize(numBytesAvail);
    socket->read(receivedBytes.data(),receivedBytes.size());

    // Tokenize for newlines
    receivedData+=QString::fromUtf8(receivedBytes.data(),receivedBytes.size());
    if(receivedData.contains('\n'))
    {
        QStringList lineList=receivedData.split(QRegExp("\r\n|\n"));
        //If line ends with \n lineList will contain a trailing empty string
        //otherwise it will contain part of a line without the terminating \n
        //In both cases lineList.at(lineList.size()-1) should not be sent
        //with emit.
        int numLines=lineList.size()-1;
        receivedData=lineList.at(lineList.size()-1);
        for(int i=0;i<numLines;i++)
        {
            emit lineReceived(lineList.at(i));
            processReceivedLine(lineList.at(i).toUtf8());
        }
    }

}

/**
  * open(portName)
  */
void IT100::open(QHostAddress remoteAddr, quint16 port)
{
    remoteHostAddress = remoteAddr;
    remoteHostPort = port;

    // maintain connection after disconnect
    _connectionAttempts++;
    _connectionIntent = true;

    socket->connectToHost(remoteAddr,port);
}

void IT100::open()
{
    if (remoteHostAddress.isNull() || remoteHostPort == 0) return;
    open(remoteHostAddress,remoteHostPort);
}

void IT100::onConnected()
{

    pollTimer->start();
    communicationsTimeoutTimer->start();

    // Start Panel Time Sync Discipliner QTimer
    timedisciplineTimer->start(12 * 60 * 60 * 1000); // 12 hours
    // onTimeDisciplineTimerTimeout();

    // Request System Status
    // Prepare for the flood
    _waitingForStatusUpdate = true;
    sendCommand(CMD_STATUS_REQUEST);

}

/**
  * processReceivedLine(data)
  *
  */
int IT100::processReceivedLine(QByteArray data)
{

    bool error = false;

    // Reject packets that are less than 5 chars
    if (data.count() > 5) {

        // update timeout timers
        pollTimer->start();
        communicationsTimeoutTimer->start();
        lastReceivedCommsAt = lastReceivedCommsAt.currentDateTime();

        // fyi - we are ignoring the checksum
        QByteArray checksum = data.right(2);
        QByteArray command = data.left(3);
        QByteArray payload = data.mid(3,data.count() - 3 - 2);

        if (_debugMode) qDebug() << qPrintable(QString("it100 -> %1 %2 %3")
            .arg(QString(command))
            .arg(QString(payload))
            .arg(QString(checksum)));

        // set system date & time
        // hhmmMMDDYY
        QDateTime datetime;
        QString timestamp = datetime.currentDateTime().toString("dd-MMM-yyyy hh:mm:ss");

        if (_debugMode && (command == CMD_COMMAND_ACKNOWLEDGE)) {
            qDebug() << qPrintable(QString("it100 -> ACK")); }

        if (command == CMD_COMMAND_ERROR) {
            qDebug() << qPrintable(QString("it100 -> ERR: %1").arg(payload.data()));
        }

        // ###########################
        // Discrete Zone Status Events
        // ###########################

        // Zone Open - no partition
        if (command == CMD_ZONE_OPEN) {
            int zone = payload.right(payload.length()-1).toInt();
            emit zoneOpen(0,zone);
            emit zoneStatusChanged(zone,0,ZONE_STATUS_OPEN);
            if (_waitingForStatusUpdate && (zone == 64)) _waitingForStatusUpdate = false;
            if (_debugMode)
                qDebug() << qPrintable(QString("%1: Zone %2 (%3) Open")
                    .arg(timestamp).arg(zone)
                    .arg(getZoneFriendlyName(zone)));
        }

        // Zone Close - no partition
        if (command == CMD_ZONE_RESTORED) {
            int zone = payload.right(payload.length()-1).toInt();
            emit zoneRestored(0,zone); // fixme, change signal to not have partition as we dont know it
            emit zoneStatusChanged(zone,0,ZONE_STATUS_RESTORED);
            if (_waitingForStatusUpdate && (zone == 64)) _waitingForStatusUpdate = false;
            if (_debugMode)
                qDebug() << qPrintable(QString("%2: Zone %1 (%3) Restored")
                    .arg(payload.data())
                    .arg(timestamp)
                    .arg(getZoneFriendlyName(zone)));
        }

        // Zone Tamper
        if (command == CMD_ZONE_TAMPER) {
            int partition = payload.left(1).toInt();
            int zone = payload.right(payload.length()-1).toInt();
            emit zoneStatusChanged(zone,partition,ZONE_STATUS_TAMPER);
        }

        // Zone Tamper Restore
        if (command == CMD_ZONE_TAMPER_RESTORE) {
            int partition = payload.left(1).toInt();
            int zone = payload.right(payload.length()-1).toInt();
            emit zoneStatusChanged(zone,partition,ZONE_STATUS_TAMPER_RESTORED);
        }

        // Zone Fault (no partition)
        if (command == CMD_ZONE_FAULT) {
            int zone = payload.right(payload.length()-1).toInt();
            emit zoneStatusChanged(zone,0,ZONE_STATUS_FAULT);
        }

        // Zone Fault Restore - no partition
        if (command == CMD_ZONE_FAULT_RESTORE) {
            int zone = payload.right(payload.length()-1).toInt();
            emit zoneStatusChanged(zone, 0, ZONE_STATUS_FAULT_RESTORED);
        }

        // Zone Alarm
        if (command == CMD_ZONE_ALARM) {
            int partition = data.mid(3,1).toInt();
            int zone = data.mid(4,2).toInt();
            emit zoneStatusChanged(zone, partition, ZONE_STATUS_ALARM);
            // writeLog(QString("Partition %1, Zone %2 in ALARM ***").arg(partition).arg(zone), LOG_LEVEL_);
        }

        // Zone Alarm Restore
        if (command == CMD_ZONE_ALARM_RESTORE) {
            int partition = data.mid(3,1).toInt();
            int zone = data.mid(4,2).toInt();
            emit zoneStatusChanged(zone, partition, ZONE_STATUS_ALARM_RESTORED);
            if (_debugMode)
                qDebug() << qPrintable(QString("%1: *** Partition %2 alarm RESTORED ***")
                    .arg(partition).arg(timestamp));
        }

        // ################
        // Partition Events
        // ################

        // 652 Partition Armed; Data Bytes: 2; Partition 1-8, Mode.
        // This command indicates that a partition has been armed and the mode
        //  it has been armed in. This command is sent at the end of the 
        //  exit-delay and after the Bell Cutoff expires.
        //  Modes = 0 Away; 1 Stay; 2 Away, No Delay; 3 Stay, No Delay
        if (command == CMD_PARTITION_ARMED_DESCRIPTIVE_MODE) {
            
            quint8 partition = data.mid(3,1).toInt();
            // quint8 mode = static_cast<quint8>(data.mid(4,1).toInt());
            PartitionArmedMode mode = static_cast<PartitionArmedMode>(data.mid(4,1).toInt());
            
            if (_debugMode)
                qDebug() << qPrintable(QString("%1: Partition %2 Armed - Descriptive Mode (%3)")
                    .arg(timestamp)
                    .arg(partition)
                    .arg((int)mode));

            emit partitionArmedDescriptive(partition, mode);
        }

        // 655 Partition Disarmed
        if (command == CMD_PARTITION_DISARMED) {
            quint8 partition = data.mid(3,1).toInt();
            emit partitionStatusChanged(partition, PARTITION_STATUS_DISARMED);
            if (_debugMode)
                qDebug() << qPrintable(QString("%1: Partition %2 Disarmed")
                    .arg(timestamp)
                    .arg(partition));
        }

        // 653 Partition is n Ready to "Force Arm" aka there is a zone violated
        if (command == CMD_PARTITION_IN_READY_TO_FORCE_ALARM) {
            quint8 partition = data.mid(3,1).toInt();
            emit partitionStatusChanged(partition, PARTITION_STATUS_READY_FORCE_ARM);
            if (_debugMode) qDebug() << qPrintable(QString("%1: Partition %2 Ready to Force Arm")
                .arg(timestamp)
                .arg(partition));
        }

        if (command == CMD_PARTITION_IN_ALARM) {
            quint8 partition = data.mid(3,1).toInt();
            emit partitionStatusChanged(partition, PARTITION_STATUS_ALARM);
        }

        if (command == CMD_PARTITION_READY) {
            int partition = data.mid(3,1).toInt();
            emit partitionStatusChanged(partition, PARTITION_STATUS_READY);
        }

        if (command == CMD_PARTITION_NOT_READY) {
            int partition = data.mid(3,1).toInt();
            emit partitionStatusChanged(partition, PARTITION_STATUS_NOT_READY);
        }

        // 673 Parttition Busy
        if (command == CMD_PARTITION_BUSY) {
            int partition = data.mid(3,1).toInt();
            emit partitionStatusChanged(partition, PARTITION_STATUS_BUSY);
        }

        // User Closing
        // Partition has been armed by a user
        // includes user code
        if (command == CMD_USER_CLOSING) {
            int partition = data.mid(3,1).toInt();
            emit partitionStatusChanged(partition, PARTITION_STATUS_USER_CLOSING);
        }

        // Partition Partial Closing
        // Partition has been armed but one or more
        // zones have been bypassed
        if (command == CMD_PARTIAL_CLOSING) {
            int partition = data.mid(3,1).toInt();
            emit partitionStatusChanged(partition, PARTITION_STATUS_PARTIAL_CLOSING);
        }

        if (command == CMD_EXIT_DELAY_IN_PROGRESS) {
            int partition = data.mid(3,1).toInt();
            emit partitionStatusChanged(partition, PARTITION_STATUS_EXIT_DELAY_IN_PROGRESS);
        }
        if (command == CMD_ENTRY_DELAY_IN_PROGRESS) {
            int partition = data.mid(3,1).toInt();
            emit partitionStatusChanged(partition, PARTITION_STATUS_ENTRY_DELAY_IN_PROGRESS);
        }

        // Partition Special Closing
        // Partition has been armed by one of following methods
        // Quick Arm, Auto Arm, Keyswitch, DLS Software, or Wireless Key
        if (command == CMD_SPECIAL_CLOSING) {
            int partition = data.mid(3,1).toInt();
            emit partitionStatusChanged(partition, PARTITION_STATUS_PARTIAL_CLOSING);
        }

        if (command == CMD_INVALID_ACCESS_CODE) {
            int partition = data.mid(3,1).toInt();
            emit partitionStatusChanged(partition, PARTITION_STATUS_INVALID_ACCESS_CODE);
        }

        if (command == CMD_FUNCTION_NOT_AVAILABLE) {
            int partition = data.mid(3,1).toInt();
            emit partitionStatusChanged(partition, PARTITION_STATUS_FUNCTION_NOT_AVAILABLE);
        }

        if (command == CMD_LCD_UPDATE) {

            // qDebug() << "CMD_LCD_UPDATE " << payload;
            quint8 lineNumber = payload.left(1).toInt();
            quint8 columnNumber = payload.mid(1,2).toInt();
            quint8 characters = payload.mid(3,2).toInt();
            QString content = payload.right(payload.length()-5);

            // TODO, account for character modifications vice line updates
            // the condition below appears to be common whereas the it100
            // calls for a wrapping 32 character screen update starting at
            // line 0
            if (lineNumber == 0 && content.length() == 32) {
                lcdDisplayContents = content;
                lcdDisplayContents.insert(16,"\r\n");
            }

            if (columnNumber != 0) qDebug() << "Character Mod Detected on LCD Display";

            emit virtualKeypadDisplayUpdate();

        }

        // TROUBLE EVENTS

        if (command == CMD_PANEL_AC_TROUBLE)
            emit troubleEvent(TROUBLE_PANEL_AC);

        if (command == CMD_PANEL_AC_RESTORE)
            emit troubleEvent(TROUBLE_PANEL_AC_RESTORE);

        if (command == CMD_PANEL_BATTERY_TROUBLE)
            emit troubleEvent(TROUBLE_PANEL_BATTERY);

        if (command == CMD_PANEL_BATTERY_TROUBLE_RESTORE)
            emit troubleEvent(TROUBLE_PANEL_BATTERY_RESTORE);

        if (command == CMD_SYSTEM_BELL_TROUBLE)
            emit troubleEvent(TROUBLE_SYSTEM_BELL);

        if (command == CMD_SYSTEM_BELL_TROUBLE_RESTORAL)
            emit troubleEvent(TROUBLE_SYSTEM_BELL_RESTORE);

        if (command == CMD_TLM_LINE_1_TROUBLE)
            emit troubleEvent(TROUBLE_TLM_1);

        if (command == CMD_TLM_LINE_1_TROUBLE_RESTORAL)
            emit troubleEvent(TROUBLE_TLM_1_RESTORE);

        if (command == CMD_TLM_LINE_2_TROUBLE)
            emit troubleEvent(TROUBLE_TLM_2);

        if (command == CMD_TLM_LINE_2_TROUBLE_RESTORAL)
            emit troubleEvent(TROUBLE_TLM_2_RESTORE);

        if (command == CMD_FTC_TROUBLE)
            emit troubleEvent(TROUBLE_FTC);

        if (command == CMD_BUFFER_NEAR_FULL)
            emit troubleEvent(TROUBLE_BUFFER_NEAR_FULL);

        if (command == CMD_GENERAL_DEVICE_LOW_BATTERY)
            emit troubleEvent(TROUBLE_GENERAL_DEVICE_LOW_BATTERY);

        if (command == CMD_GENERAL_DEVICE_LOW_BATTERY_RESTORE)
            emit troubleEvent(TROUBLE_GENERAL_DEVICE_LOW_BATTERY_RESTORE);

        if (command == CMD_GENERAL_SYSTEM_TAMPER)
            emit troubleEvent(TROUBLE_GENERAL_SYSTEM_TAMPER);

        if (command == CMD_GENERAL_SYSTEM_TAMPER_RESTORE)
            emit troubleEvent(TROUBLE_GENERAL_SYSTEM_TAMPER_RESTORE);

        if (command == CMD_WIRELESS_KEY_LOW_BATTERY_TROUBLE)
            emit troubleEvent(TROUBLE_WIRELESS_KEY_LOW_BATTERY);

        if (command == CMD_WIRELESS_KEY_LOW_BATTERY_TROUBLE_RESTORE)
            emit troubleEvent(TROUBLE_WIRELESS_KEY_LOW_BATTERY_RESTORE);

        if (command == CMD_HANDHELD_KEYPAD_LOW_BATTERY_TROUBLE)
            emit troubleEvent(TROUBLE_HANDHELD_KEYPAD_LOW_BATTERY);

        if (command == CMD_HANDHELD_KEYPAD_LOW_BATTERY_TROUBLE_RESTORE)
            emit troubleEvent(TROUBLE_HANDHELD_KEYPAD_LOW_BATTERY_RESTORE);

        if (command == CMD_HOME_AUTOMATION_TROUBLE)
            emit troubleEvent(TROUBLE_HOME_AUTOMATION);

        if (command == CMD_HOME_AUTOMATION_TROUBLE_RESTORE)
            emit troubleEvent(TROUBLE_HOME_AUTOMATION_RESTORE);

    } else {
        error = true;
    }

    // Determine we have good communications so we can update our
    // broker status.  This should be improved to take into account
    // baud rate mismatches and garbage data.
    if (!error) {
        if (!communicationsGood) {
            this->status = COMP_STATUS_OK;
            emit communicationsBegin();
            communicationsGood = true;
        }
    }

    // In the game of ping pong, we can now consider ourselves to not be waiting
    // for a message to return
    // albeit certain commands result in a flood of return messages such
    // as the status request
    writePacket();

}


/**
  * onPollTimer()
  * Called periodically by pollTimer::timeout() signal
  * This process is responsible for ensuring traffic is
  * routinely sent and received by the IT-100 module.
  * This method will throw a signal if deemed the unit has
  * not responded in time frame set out by IT100::timeoutSecs
  */
void IT100::onPollTimerTimeout()
{
    if (_connected) this->sendCommand(CMD_POLL);
}

// Are we waiting for a complete IT100 status request response
//
bool IT100::isWaitingForStatusUpdate()
{
    return _waitingForStatusUpdate;
}

/**
  * onTimeDiscipliner()
  * called by QTimer
  * Set DSC alarm panel time to system time
  * No slew, etc.
  */
void IT100::onTimeDisciplineTimerTimeout()
{
    // set system date & time hhmmMMDDYY
    QDateTime datetime;
    if (_connected)
        sendCommand(CMD_SET_TIME_AND_DATE,QByteArray(datetime.currentDateTime().toString("hhmmMMddyy").toUtf8()));
}

void IT100::onCommunicationsTimeoutTimerTimeout()
{
    int lastCommsSecs = lastReceivedCommsAt.secsTo(QDateTime::currentDateTime());

    if (lastCommsSecs >= it100CommunicationsTimeoutTime) {
        // We are now not communicating with the module
        communicationsGood = false;
        this->status = COMP_STATUS_FAILED;
        emit communicationsTimeout();
    }
}

// this method is called every it100TcpConnecionRetryTimeSeconds * 1000ms
// until connected - never give up!
void IT100::on_moduleReconnectTimerTimeout()
{
    this->open();
}

/**
  * generateDscChecksum
  * Produce DSC style 2 byte checksum
  * Input is command+data bytes
  * Can be used for packet generation + verification
  */
QByteArray IT100::generateChecksum(QByteArray data)
{
    /* Count up HEX (as int) bytes */
    QString hexint = QString(data.toHex());
    int count = 0;
    for (int x = 0; x < data.size(); x++) {
        // hexint = QString(QByteArray(status.at(x)).toHex());
        bool ok;
        count += hexint.mid((x * 2),2).toUInt(&ok,16);
    }

    /* Convert number (int) to HEX Ascii for chopping up */
    QString asciiint = QString().setNum(count,16);

    // QByteArray checksum = QByteArray().fromHex();
    // TODO, still need to truncate this to 8 bits!

    /* create silly checksum */
    QByteArray checksum;
    checksum.append(asciiint.mid(asciiint.size()-2,1).toUpper());
    checksum.append(asciiint.mid(asciiint.size()-1,1).toUpper());

    return checksum;

}

/**
  * generatePacket(command,data)
  * Accept DSC command and data bytes
  * generate checksum, and append CR/LF
  */
QByteArray IT100::generatePacket(QByteArray command, QByteArray data = "")
{
    /* Start off packet with command */
    QByteArray packet = command;

    /* Append data */
    if (data.size()>0) packet.append(data);

    /* Append calculated checksum from current payload */
    packet.append(generateChecksum(packet));

    /* Append token EOL */
    packet.append("\r\n");

    return packet;
}



/**
  * Send Command to DSC IT-100 Module
  * Basically a wrapper, to allow for abstraction
  */
void IT100::sendCommand(QByteArray command, QByteArray data)
{
    // Create IT100 Message
    It100Message *message = new It100Message(command,data);

    // Append message to queue
    messageQueueOut.append(message);

    // Force the message along if the conditions are right
    if (isConnected() && !waitingForResponse)
        writePacket();

}

void IT100::sendCommand(QByteArray command, quint16 data)
{
    sendCommand(command,QByteArray::number(data));
}

void IT100::sendCommand(QByteArray command)
{
    sendCommand(command,QByteArray());
}

// Write packet from queue
// remove last message
//
// possible improvement would be to leave the last message
// in the queue for cases of re-send
//
void IT100::writePacket()
{
    waitingForResponse = false;
    if (messageQueueOut.count()) {
        if (_debugMode) {
            QString debugMsg = QString::fromUtf8(*messageQueueOut.at(0)->packet());
            if (debugMsg.endsWith("\r\n")) debugMsg.truncate(debugMsg.count()-2);
            qDebug() << qPrintable(QString("wrote: %1").arg(debugMsg));
        }
        socket->write(messageQueueOut.at(0)->packet()->data());
        messageQueueOut.removeAt(0);
        waitingForResponse = true;
    }
}

/**
  * setZoneFriendlyName(int,QString)
  * Set respective zone number to friendly string
  */
int IT100::setZoneFriendlyName(int zoneNumber, QString name)
{
    zoneFriendlyNames[zoneNumber-1] = name;
    return false;
}

/**
  * getZoneFriendlyName(int zone)
  * Return friendly name of zone, if set
  */
QString IT100::getZoneFriendlyName(int zoneNumber)
{
    if (this->zoneFriendlyNames[zoneNumber-1] != "")
        return this->zoneFriendlyNames[zoneNumber-1];
    else
        return QString("undefined");
}

void IT100::onTcpSocketStateChanged(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::HostLookupState:
        break;

    case QAbstractSocket::ConnectingState:
        if (_debugMode) qDebug() << qPrintable(
            QString("connecting to it100 %1 tcp/%2")
                .arg(remoteHostAddress.toString())
                .arg(QString::number(remoteHostPort)));
        break;

    case QAbstractSocket::ConnectedState:
        _connected = true;
        _connectionAttempts = 0;
        waitingForResponse = false; // need to get things going again
        onConnected();
        emit connected();
        qDebug() << qPrintable(QString("connected to it100 %1 tcp/%2")
            .arg(remoteHostAddress.toString())
            .arg(QString::number(remoteHostPort)));
        break;

    case QAbstractSocket::UnconnectedState:

        // Maintain state tracking
        _connected = false;
        communicationsGood = false;
        emit disconnected();
        qDebug() << qPrintable(QString("ERROR: disconnected from %1:%2")
            .arg(remoteHostAddress.toString())
            .arg(QString::number(remoteHostPort)));

        // begin reconnect process if disconnect was unexpected
        // who are we kidding, there is no disconnect method anyways
        // try immediately first, then enter a cycling reconnect timer
        if (_connectionIntent) {
            if (!_connectionAttempts) {
                if (_debugMode) qDebug() << "attempting reconnect";
                open();
            }
            else { // start connection retry timer
                if (_connectionAttempts == 1)
                    if (_debugMode) qDebug() << qPrintable(
                        QString("starting reconnect timer; every %1 seconds")
                            .arg(3));
                QTimer::singleShot(it100TcpConnecionRetryTimeSeconds * 1000, 
                    this, SLOT(on_moduleReconnectTimerTimeout()));
            }
        }

        break;
    default:
        qDebug() << state;
        break;
    }

}

// Connection Retry Delay Period has expired
// attempt another connection
void IT100::onTcpConnectRetryTimeout()
{
    open();
}

void IT100::setRemoteHostAddress(QHostAddress address)
{
    remoteHostAddress = address;
}

void IT100::setRemoteHostPort(quint16 port)
{
    remoteHostPort = port;
}

void IT100::armAway(quint8 partition)
{
    sendCommand(IT100::CMD_PARTITION_ARM_CONTROL_AWAY,QByteArray::number(partition));
}

void IT100::armStay(quint8 partition)
{
    sendCommand(IT100::CMD_PARTITION_ARM_CONTROL_STAY,QByteArray::number(partition));
}

void IT100::disarm(quint8 partition)
{
    QByteArray code = QByteArray::number(panelUserCode);
    for(int i = code.length(); i < 6; i++) code.append("0"); // padd zeros to 6 digits
    sendCommand(IT100::CMD_PARTITION_DISARM_CONTROL_WITH_CODE,
        QByteArray::number(partition).append(code));
    qDebug() << "code is " << code;
}

void IT100::virtualKeypadEnable(bool value)
{
    sendCommand(IT100::CMD_VIRTUAL_KEYPAD_CONTROL,(quint8)value);
}

/**
  * Application Originated Commands
  */
const QByteArray IT100::CMD_POLL = "000";
const QByteArray IT100::CMD_STATUS_REQUEST = "001";
const QByteArray IT100::CMD_LABELS_REQUEST = "002";
const QByteArray IT100::CMD_SET_TIME_AND_DATE = "010";
const QByteArray IT100::CMD_COMMAND_OUTPUT_CONTROL = "020";
const QByteArray IT100::CMD_PARTITION_ARM_CONTROL_AWAY = "030";
const QByteArray IT100::CMD_PARTITION_ARM_CONTROL_STAY = "031";
const QByteArray IT100::CMD_PARTITION_ARM_CONTROL_ARMED_NO_ENTRY_DELAY = "032";
const QByteArray IT100::CMD_PARTITION_ARM_CONTROL_WITH_CODE = "033";
const QByteArray IT100::CMD_PARTITION_DISARM_CONTROL_WITH_CODE = "040";
const QByteArray IT100::CMD_TIME_STAMP_CONTROL = "055";
const QByteArray IT100::CMD_TIME_DATE_BROADCAST_CONTROL = "056";
const QByteArray IT100::CMD_TEMPERATURE_BROADCAST_CONTROL = "057";
const QByteArray IT100::CMD_VIRTUAL_KEYPAD_CONTROL = "058";
const QByteArray IT100::CMD_TRIGGER_PANIC_ALARM = "060";
const QByteArray IT100::CMD_KEY_PRESSED_VIRT = "070";
const QByteArray IT100::CMD_BAUD_RATE_CHANGE = "080";
const QByteArray IT100::CMD_GET_TEMPERATURE_SET_POINT = "095";
const QByteArray IT100::CMD_TEMPERATURE_CHANGE = "096";
const QByteArray IT100::CMD_SAVE_TEMPERATURE_SETTING = "097";
const QByteArray IT100::CMD_CODE_SEND = "200";

/**
  * IT-100 Originated Commands
  */
const QByteArray IT100::CMD_COMMAND_ACKNOWLEDGE = "500";
const QByteArray IT100::CMD_COMMAND_ERROR = "501";
const QByteArray IT100::CMD_SYSTEM_ERROR = "502";
const QByteArray IT100::CMD_TIME_DATE_BROADCAST = "550";
const QByteArray IT100::CMD_RING_DETECETD = "560";
const QByteArray IT100::CMD_INDOOR_TEMPERATURE_BROADCAST = "561";
const QByteArray IT100::CMD_OUTDOOR_TEMPERATURE_BROADCAST = "562";
const QByteArray IT100::CMD_THERMOSTAT_SET_POINTS = "563";
const QByteArray IT100::CMD_BROADCAST_LABELS = "570";
const QByteArray IT100::CMD_BAUD_RATE_SET = "580";
const QByteArray IT100::CMD_ZONE_ALARM = "601";
const QByteArray IT100::CMD_ZONE_ALARM_RESTORE = "602";
const QByteArray IT100::CMD_ZONE_TAMPER = "603";
const QByteArray IT100::CMD_ZONE_TAMPER_RESTORE = "604";
const QByteArray IT100::CMD_ZONE_FAULT = "605";
const QByteArray IT100::CMD_ZONE_FAULT_RESTORE = "606";
const QByteArray IT100::CMD_ZONE_OPEN = "609";
const QByteArray IT100::CMD_ZONE_RESTORED = "610";
const QByteArray IT100::CMD_DURESS_ALARM = "620";
const QByteArray IT100::CMD_F_KEY_ALARM = "621";
const QByteArray IT100::CMD_F_KEY_RESTORAL = "622";
const QByteArray IT100::CMD_A_KEY_ALARM = "623";
const QByteArray IT100::CMD_A_KEY_RESTORAL = "624";
const QByteArray IT100::CMD_P_KEY_ALARM = "625";
const QByteArray IT100::CMD_P_KEY_RESTORAL = "626";
const QByteArray IT100::CMD_AUXILIARY_INPUT_ALARM = "631";
const QByteArray IT100::CMD_AUXILIARY_INPUT_ALARM_RESTORED = "632";
const QByteArray IT100::CMD_PARTITION_READY = "626";
const QByteArray IT100::CMD_PARTITION_NOT_READY = "625";
const QByteArray IT100::CMD_PARTITION_ARMED_DESCRIPTIVE_MODE = "652";
const QByteArray IT100::CMD_PARTITION_IN_READY_TO_FORCE_ALARM = "653";
const QByteArray IT100::CMD_PARTITION_IN_ALARM = "654";
const QByteArray IT100::CMD_PARTITION_DISARMED = "655";
const QByteArray IT100::CMD_EXIT_DELAY_IN_PROGRESS = "656";
const QByteArray IT100::CMD_ENTRY_DELAY_IN_PROGRESS = "657";
const QByteArray IT100::CMD_KEYPAD_LOCKOUT = "658";
const QByteArray IT100::CMD_KEYPAD_BLANKING = "659";
const QByteArray IT100::CMD_COMMAND_OUTPUT_IN_PROGRESS = "660";
const QByteArray IT100::CMD_INVALID_ACCESS_CODE = "670";
const QByteArray IT100::CMD_FUNCTION_NOT_AVAILABLE = "671";
const QByteArray IT100::CMD_FAIL_TO_ARM = "672";
const QByteArray IT100::CMD_PARTITION_BUSY = "673";
const QByteArray IT100::CMD_USER_CLOSING = "700";
const QByteArray IT100::CMD_SPECIAL_CLOSING = "701";
const QByteArray IT100::CMD_PARTIAL_CLOSING = "702";
const QByteArray IT100::CMD_USER_OPENING = "750";
const QByteArray IT100::CMD_SPECIAL_OPENING = "751";
const QByteArray IT100::CMD_PANEL_BATTERY_TROUBLE = "800";
const QByteArray IT100::CMD_PANEL_BATTERY_TROUBLE_RESTORE ="801";
const QByteArray IT100::CMD_PANEL_AC_TROUBLE = "802";
const QByteArray IT100::CMD_PANEL_AC_RESTORE = "803";
const QByteArray IT100::CMD_SYSTEM_BELL_TROUBLE ="806";
const QByteArray IT100::CMD_SYSTEM_BELL_TROUBLE_RESTORAL = "807";
const QByteArray IT100::CMD_TLM_LINE_1_TROUBLE = "810";
const QByteArray IT100::CMD_TLM_LINE_1_TROUBLE_RESTORAL = "811";
const QByteArray IT100::CMD_TLM_LINE_2_TROUBLE = "812";
const QByteArray IT100::CMD_TLM_LINE_2_TROUBLE_RESTORAL = "813";
const QByteArray IT100::CMD_FTC_TROUBLE = "814";
const QByteArray IT100::CMD_BUFFER_NEAR_FULL = "816";
const QByteArray IT100::CMD_GENERAL_DEVICE_LOW_BATTERY = "821";
const QByteArray IT100::CMD_GENERAL_DEVICE_LOW_BATTERY_RESTORE = "822";
const QByteArray IT100::CMD_WIRELESS_KEY_LOW_BATTERY_TROUBLE = "825";
const QByteArray IT100::CMD_WIRELESS_KEY_LOW_BATTERY_TROUBLE_RESTORE = "826";
const QByteArray IT100::CMD_HANDHELD_KEYPAD_LOW_BATTERY_TROUBLE = "827";
const QByteArray IT100::CMD_HANDHELD_KEYPAD_LOW_BATTERY_TROUBLE_RESTORE = "828";
const QByteArray IT100::CMD_GENERAL_SYSTEM_TAMPER = "829";
const QByteArray IT100::CMD_GENERAL_SYSTEM_TAMPER_RESTORE = "830";
const QByteArray IT100::CMD_HOME_AUTOMATION_TROUBLE = "831";
const QByteArray IT100::CMD_HOME_AUTOMATION_TROUBLE_RESTORE = "832";
const QByteArray IT100::CMD_TROUBLE_STATUS_LED_ON = "840";
const QByteArray IT100::CMD_TROUBLE_STATUS_LED_OFF = "841";
const QByteArray IT100::CMD_FIRE_TROUBLE_ALARM = "842";
const QByteArray IT100::CMD_FIRE_TROUBLE_ALARM_RESTORE = "843";
const QByteArray IT100::CMD_CODE_REQUIRED = "900";
const QByteArray IT100::CMD_LCD_UPDATE = "901";
const QByteArray IT100::CMD_LCD_CURSOR = "902";
const QByteArray IT100::CMD_LED_STATUS = "903";
const QByteArray IT100::CMD_BEEP_STATUS = "904";
const QByteArray IT100::CMD_TONE_STATUS = "905";
const QByteArray IT100::CMD_BUZZER_STATUS = "906";
const QByteArray IT100::CMD_DOOR_CHIME_STATUS = "907";
const QByteArray IT100::CMD_SOFTWARE_VERSION = "908";
