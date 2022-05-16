#include "it100mqtt.h"

It100Mqtt::It100Mqtt(QString settingsFile, QObject *parent) :
    QObject(parent)
{

    // silence the qmqtt debugging noise
    QLoggingCategory::setFilterRules(QStringLiteral("qmqtt.*=false"));

    // Load settings from supplied INI file
    //
    settings = new QSettings(settingsFile,QSettings::IniFormat,this);
    if (settings->status() != QSettings::NoError) {
        qDebug() << "Error accessing settings.cfg";
        writeLog("Error accessing settings.cfg", LOG_LEVEL_ERROR);
    } else {
        
        // General INI
        debugMode = settings->value("debug", false).toBool();
        if (debugMode) qDebug() <<
         "debug mode enabled!";
        
        settings->beginGroup("it100");
        it100RemoteHost = settings->value("host", QString()).toString();
        it100RemotePort = settings->value("port", 0).toInt(); // quint16
        it100UserCode = settings->value("user_code", QString()).toString();
        settings->endGroup();
        
        if (it100RemotePort == 0) {
            qDebug() << "error: unable to load configuration; exiting";
            _failed = true;
            return;
        };

        settings->beginGroup("mqtt");
        m_mqttRemoteHost = settings->value("host", QString()).toString();
        m_mqttRemotePort = settings->value("port", QString()).toInt();
        mqttClientName = settings->value("client_name", QString("it100")).toString();
        mqttTopicPrefix = settings->value("topic_prefix", QString("alarm/")).toString();
        mqttTopicPrefix = mqttTopicPrefix.append(mqttClientName);
        settings->endGroup();

        // Graylog
        settings->beginGroup("graylog");

        // Check if host and port are set -- if not, we will not enable graylog
        if (settings->value("host").isValid() && settings->value("port").isValid()) {
            QString grayLogName = "it100";
            if (settings->value("name").isValid()) grayLogName = settings->value("name").toString();
            graylog = new Graylog(grayLogName, settings->value("host", QString()).toString(), settings->value("port", quint16()).toInt());
            qDebug() << qPrintable(QString("Graylog2 configured for %1 at %2:%3").arg(grayLogName)
                                   .arg(settings->value("host", QString()).toString())
                                   .arg(settings->value("port", quint16()).toInt()));
        } else {
            graylog = new Graylog();
            graylog->setEnabled(false);
        }
        settings->endGroup();

        this->it100 = new IT100(IFACE_IPSERIAL,debugMode);

        // Configure MQTT
        client = new QMQTT::Client();
        client->setClientId(mqttClientName);
        //client->setUsername("user");
        //client->setPassword("password");
        mId = 0; // track sequential message ids
        QMQTT::Will *will = new QMQTT::Will(QString("%1/availability").arg(mqttTopicPrefix),"offline",QOS_1,true);
        client->setWill(will);

        connect(client, SIGNAL(connected()),
                this, SLOT(onMqttConnect()));
        connect(client, SIGNAL(disconnected()),
                this, SLOT(onMqttDisconnected()));
        connect(client, SIGNAL(received(QMQTT::Message)),
                this, SLOT(onMqttMessageReceived(QMQTT::Message)));

        // Apply IT100 Settings
        it100->setUserCode(it100UserCode.toInt());
        if (debugMode) 
            qDebug() << qPrintable(QString("user code is: %1").arg(it100UserCode.toInt()));

        connect(it100, SIGNAL(connected()), this, SLOT(onIt100Connected()));
        connect(it100, SIGNAL(disconnected()), 
        				this, SLOT(onIt100Disconnected()));
        connect(it100, SIGNAL(communicationsBegin()), 
        				this, SLOT(onIt100CommunicationsBegin()));
        connect(it100, SIGNAL(communicationsTimeout()), 
        				this, SLOT(onIt100CommunicationsTimeout()));
        connect(it100, SIGNAL(partitionStatusChanged(quint8,PartitionStatus)),
                this, SLOT(onIt100PartitionStatusChange(quint8,PartitionStatus)));
        connect(it100, SIGNAL(zoneStatusChanged(quint8,quint8,ZoneStatus)),
                this, SLOT(onIt100ZoneStatusChange(quint8,quint8,ZoneStatus)));
        connect(it100, SIGNAL(virtualKeypadDisplayUpdate()),
                this, SLOT(onIt100VirtualKeypadDisplayUpdate()));
        connect(it100, SIGNAL(troubleEvent(TroubleEvent)),
                this, SLOT(onIt100TroubleEvent(TroubleEvent)));
        connect(it100, SIGNAL(partitionArmedDescriptive(quint8,PartitionArmedMode)),
                this, SLOT(onIt100PartitionArmedDescriptive(quint8,PartitionArmedMode)));

        // Go ahead and connect
        connectToMqttBroker(m_mqttRemoteHost, m_mqttRemotePort);
        connectToIt100(it100RemoteHost, it100RemotePort);

        graylog->sendMessage("Starting it100-mqtt service", LevelNotice);
    }
}


void It100Mqtt::updateServiceStatus()
{
    // is IT100 module communicating correctly?
    if (it100->getStatus() == COMP_STATUS_OK && 
            mqttStatus == COMP_STATUS_OK) {
        sd_notify (0, "READY=1");
    } else {
        // todo: determine difference between starting and failed
        // sd_notify(0, "STATUS=FAILED");
    }
}

void It100Mqtt::onIt100VirtualKeypadDisplayUpdate()
{
	// send as QOS 1 (QOS 2 not supported by RabbitMQ MQTT)
	// Retained topic for late joining clients
    writeMqtt(QString("%1/keypad_lcd_data").arg(mqttTopicPrefix), this->it100->lcdDisplayContents, QOS_1, true);
}


void It100Mqtt::onMqttConnect()
{
    // Subscribe to commands as QOS 2
    // QoS2, Exactly once: The message is always delivered exactly once.
    // The message must be stored locally at the sender, until the sender
    // receives confirmation that the message has been published by the receiver.
    // The message is stored in case the message must be sent again.
    // QoS2 is the safest, but slowest mode of transfer.
    // A more sophisticated handshaking and acknowledgement sequence is used
    // than for QoS1 to ensure no duplication of messages occurs.
    //
    client->subscribe(QString("%1/command").arg(mqttTopicPrefix), QOS_1);
    mqttStatus = COMP_STATUS_OK;
    updateServiceStatus();
    // writeMqtt(QString("%1/partition/1/arm_status").arg(mqttTopicPrefix), "unknown", QOS_1, true);

}

void It100Mqtt::onMqttDisconnected()
{
    QTimer::singleShot(1000, this, SLOT(reconnectTimerTimeout()));
    graylog->sendMessage("it100-mqtt mqtt disconnected", LevelNotice);
    mqttStatus = COMP_STATUS_FAILED;
    updateServiceStatus();
}

void It100Mqtt::reconnectTimerTimeout()
{
    connectToMqttBroker(m_mqttRemoteHost, m_mqttRemotePort);
}


void It100Mqtt::onMqttMessageReceived(QMQTT::Message message)
{
    QString payload = QString(message.payload()).toLower();

    QString logMessage = QString("Received MQTT Message: %1 %2 (id=%3;qos=%4)")
            .arg(QString(message.topic()))
            .arg(payload)
            .arg(message.id())
            .arg(message.qos());

    graylog->sendMessage(logMessage, LevelDebug);
    writeLog(logMessage);

    if (message.topic() == QString("%1/command").arg(mqttTopicPrefix)) {
        if ((payload == "arm")||(payload == "arm_away")||(payload == "arm_stay")) {
            it100->armAway();
            writeLog("requested to arm", LOG_LEVEL_DEBUG);
            graylog->sendMessage("Requested to ARM; Arming", LevelNotice);
        }
        if (payload == "disarm") {
            it100->disarm();
            writeLog("requested to disarm", LOG_LEVEL_DEBUG);
            graylog->sendMessage("Requested to DISARM, Disarming", LevelNotice);
        } 
        if (payload.length() == 1) {

            // SINGLE CHARACTER -- KEYPRESS
            // Some operations require a long keypress (> 1.5 seconds)
            // To accomodate this, ALL keypresses must be followed by a keybreak ('^')
            // To generate a long keypress, insert a 1.5 second delay before sending
            // the keybreak

            uint ascii = (uint)message.payload().at(0);

            // ALLOW ONLY
            // 0 THROUGH 9
            // *,#,<,>
            // a through e, F,A,P
            if (((ascii >= 48) && (ascii <= 57)) ||
                    (ascii == 0x2a) || (ascii == 0x23) || (ascii == 0x3c) || (ascii == 0x3e) ||
                    ((ascii >= 0x61) && (ascii <= 0x65)) || (ascii == 0x46)  || (ascii == 0x41)  || (ascii == 0x50)) {
                        it100->sendCommand(IT100::CMD_KEY_PRESSED_VIRT, message.payload());
                        it100->sendCommand(IT100::CMD_KEY_PRESSED_VIRT, QByteArray("^")); // keybreak
                    }

        }
        //else if (message.payload().toInt()) { // is command alpha?
        //}

        else {
            QString logMessage = QString("Command not understood: %1").arg(payload);
            graylog->sendMessage(logMessage, LevelError);
            writeLog(logMessage, LOG_LEVEL_ERROR);
        }
    }
}

void It100Mqtt::writeMqtt(const char *topic, const char *message, QosLevel qos, bool retain)
{
    QMQTT::Message msg(mId++,topic,message,qos,retain);
    client->publish(msg);
}

void It100Mqtt::writeMqtt(const char *topic, QString message, QosLevel qos, bool retain)
{
    writeMqtt(topic, message.toStdString().c_str(), qos, retain);
}

void It100Mqtt::writeMqtt(QString topic, const char *message, QosLevel qos, bool retain)
{
    writeMqtt(topic.toStdString().c_str(), message, qos, retain);
}

void It100Mqtt::writeMqtt(QString topic, QString message, QosLevel qos, bool retain)
{
    writeMqtt(topic.toStdString().c_str(), message.toStdString().c_str(), qos, retain);
}


void It100Mqtt::onTestTimerTimeout()
{

}

// Connect it100 to remote tcp/ip host
//
int It100Mqtt::connectToIt100(QHostAddress host, qint16 port)
{
    it100->open(host,port);
    return false;
}

int It100Mqtt::connectToMqttBroker(QHostAddress host, qint16 port)
{
    client->setHost(host.toString());
    client->setPort(port);
    // needing to clean session to prevent duplicates
    // for the time being
    client->setCleansess(true);
    client->connect();

    writeLog("it100 Started", LOG_LEVEL_NOTICE);

    // Start a test timerw
    //
    testTimer = new QTimer();
    //testTimer->start(1500);
    connect(this->testTimer, SIGNAL(timeout()), this, SLOT(onTestTimerTimeout()));

    return false;
}

void It100Mqtt::onIt100Connected()
{
    // This is a TCP connection only and does not infer
    // good communications with the module
    // See ::onIt100CommunicationsBegin() for this
    writeMqtt(QString("client/%1").arg(mqttClientName), "{ \"connected:\" : \"true\" }", QOS_1, true);
    writeLog(QString("Connected to IT100 at %1:%2").arg(it100->remoteHostAddress.toString()).arg(it100->remoteHostPort), LOG_LEVEL_DEBUG);
    graylog->sendMessage("Connected to it100 via tcp", LevelInformational);
}

void It100Mqtt::onIt100Disconnected()
{
    writeLog(QString("DISCONNECTED from IT100 at %1:%2").arg(it100->remoteHostAddress.toString()).arg(it100->remoteHostPort), LOG_LEVEL_DEBUG);
    graylog->sendMessage("Disconnected from it100 via tcp", LevelNotice);
}

void It100Mqtt::onIt100CommunicationsBegin()
{
    // writeMqtt(QString("client-status/%1").arg(mqttClientName), "{ \"connected:" : true", QOS_1, true);
    writeMqtt(QString("%1/event").arg(mqttTopicPrefix), "it100 module is communicating");
    writeMqtt(QString("%1/availability").arg(mqttTopicPrefix),"online",QOS_1,true);
    writeLog("IT-100 is communicating", LOG_LEVEL_NOTICE);
    graylog->sendMessage("it100 module is communicating", LevelNotice);
    
    updateServiceStatus();
}

void It100Mqtt::onIt100CommunicationsTimeout()
{
    // writeMqtt(QString("status/%1").arg(mqttClientName),"fault",QOS_1,true);
    writeMqtt(QString("%1/event").arg(mqttTopicPrefix), "it100 module communications timeout");
    writeMqtt(QString("%1/availability").arg(mqttTopicPrefix),"offline",QOS_1,true);
    writeLog("Communications with IT100 has timed out", LOG_LEVEL_ERROR);
    graylog->sendMessage("it100 module communications timeout", LevelNotice);

    updateServiceStatus();
}

void It100Mqtt::onIt100ZoneStatusChange(quint8 zone, quint8 partition, ZoneStatus status)
{
        switch (status) {
            
        case ZONE_STATUS_ALARM:
            if (!it100->isWaitingForStatusUpdate())
                writeMqtt(QString("%1/zone/%2/event").arg(mqttTopicPrefix).arg(zone),"alarm");
            writeMqtt(QString("%1/zone/%2/condition").arg(mqttTopicPrefix).arg(zone),"alarm",QOS_1,true);
            graylog->sendMessage(QString("zone %1 is in alarm!").arg(zone), LevelCritical);
            break;
            
        case ZONE_STATUS_ALARM_RESTORED:
            writeMqtt(QString("%1/zone/%2/event").arg(mqttTopicPrefix).arg(zone),"alarm_restored");
            writeMqtt(QString("alarm/event").arg(zone),"alarm_restored");
            graylog->sendMessage(QString("zone %1 alarm restored!").arg(zone), LevelCritical);
            break;
            
        case ZONE_STATUS_OPEN:
            if (!it100->isWaitingForStatusUpdate())
                writeMqtt(QString("%1/zone/%2/event").arg(mqttTopicPrefix).arg(zone),"violated");
            writeMqtt(QString("%1/zone/%2/state").arg(mqttTopicPrefix).arg(zone),"open",QOS_1,true);
            writeMqtt(QString("%1/zone/%2/condition").arg(mqttTopicPrefix).arg(zone),"violated",QOS_1,true);
            graylog->sendMessage(QString("Zone %1 Open").arg(zone), LevelInformational);
            break;
            
        case ZONE_STATUS_RESTORED:
            if (!it100->isWaitingForStatusUpdate())
                writeMqtt(QString("%1/zone/%2/event").arg(mqttTopicPrefix).arg(zone),"restored");
            writeMqtt(QString("%1/zone/%2/state").arg(mqttTopicPrefix).arg(zone),"closed",QOS_1,true);
            writeMqtt(QString("%1/zone/%2/condition").arg(mqttTopicPrefix).arg(zone),"secure",QOS_1,true);
            graylog->sendMessage(QString("Zone %1 Restored").arg(zone), LevelInformational);
            break;
            
        case ZONE_STATUS_TAMPER:
            if (!it100->isWaitingForStatusUpdate())
                writeMqtt(QString("%1/zone/%2/event").arg(mqttTopicPrefix).arg(zone),"tamper");
            writeMqtt(QString("%1/zone/%2/condition").arg(mqttTopicPrefix).arg(zone),"tamper",QOS_1,true);
            graylog->sendMessage(QString("Zone %1 Tamper").arg(zone), LevelCritical);
            break;
            
        case ZONE_STATUS_TAMPER_RESTORED:
            if (!it100->isWaitingForStatusUpdate())
                writeMqtt(QString("%1/zone/%2/event").arg(mqttTopicPrefix).arg(zone),"tamper_restored");
                graylog->sendMessage(QString("Zone %1 Tamper Restored").arg(zone), LevelNotice);
            break;
            
        case ZONE_STATUS_FAULT:
            if (!it100->isWaitingForStatusUpdate())
                writeMqtt(QString("%1/zone/%1/event").arg(mqttTopicPrefix).arg(zone),"fault",QOS_1,true);
            writeMqtt(QString("%1/zone/%1/condition").arg(mqttTopicPrefix).arg(zone),"fault",QOS_1,true);
            graylog->sendMessage(QString("Zone %1 Fault").arg(zone), LevelCritical);
            break;
            
        case ZONE_STATUS_FAULT_RESTORED:
            if (!it100->isWaitingForStatusUpdate())
                writeMqtt(QString("%1/zone/%2/event").arg(mqttTopicPrefix).arg(zone),"fault_restored");
            // how do we determine the condition of the zone now?? does it100 send a restored/violated?
            graylog->sendMessage(QString("Zone %1 Fault Restored").arg(zone), LevelNotice);
            break;

//            static const QByteArray CMD_LCD_UPDATE;
//            static const QByteArray CMD_LCD_CURSOR;
//            static const QByteArray CMD_LED_STATUS;
//            static const QByteArray CMD_BEEP_STATUS;
//            static const QByteArray CMD_TONE_STATUS;
//            static const QByteArray CMD_BUZZER_STATUS;
//            static const QByteArray CMD_DOOR_CHIME_STATUS;

        default:
            break;
        }

        writeLog(QString("ZoneStatusChange(partition=%1,zone=%2,status=%3)").arg(partition).arg(zone).arg((qint8)status), LOG_LEVEL_DEBUG);
}

void It100Mqtt::onIt100PartitionArmedDescriptive(quint8 partition, PartitionArmedMode mode)
{
    QString armed_mode = "armed_away";  
    QString hass_armed_mode = "armed_away";
    if (mode == PARTITION_ARMED_STAY || mode == PARTITION_ARMED_STAY_NODELAY) {
        armed_mode = "armed_stay";
        hass_armed_mode = "armed_home";
        qDebug() << "ARMED STAY!";
    }
        
    if (!it100->isWaitingForStatusUpdate())
        writeMqtt(QString("%1/partition/%2/event").arg(mqttTopicPrefix).arg(partition),armed_mode);

    writeMqtt(QString("%1/partition/%2/armed").arg(mqttTopicPrefix).arg(partition),"1",QOS_1,true);
    writeMqtt(QString("%1/partition/%2/state").arg(mqttTopicPrefix).arg(partition),armed_mode,QOS_1,true);
    writeMqtt(QString("%1/partition/%2/hass_state").arg(mqttTopicPrefix).arg(partition),hass_armed_mode,QOS_1,true);

    
//    writeLog(QString("Alarm partition %1 armed").arg(partition), LOG_LEVEL_NOTICE);
//    graylog->sendMessage(QString("Partition %1 Armed!").arg(partition), LevelInformational);
    // panel.partition(partition)->armed = true;

}

void It100Mqtt::onIt100PartitionStatusChange(quint8 partition, PartitionStatus status)
{
    switch (status) {
    case PARTITION_STATUS_ALARM:
        if (!it100->isWaitingForStatusUpdate())
            writeMqtt(QString("%1/partition/%2/event").arg(mqttTopicPrefix).arg(partition),"alarm");
            writeLog(QString("Alarm partition %1 in ALARM").arg(partition), LOG_LEVEL_NOTICE);
            graylog->sendMessage(QString("Partition %1 is in Alarm!").arg(partition), LevelCritical);
        writeMqtt(QString("%1/partition/%2/state").arg(mqttTopicPrefix).arg(partition),"alarm",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/hass_state").arg(mqttTopicPrefix).arg(partition),"triggered",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/condition").arg(mqttTopicPrefix).arg(partition),"alarm",QOS_1,true);
        break;

    case PARTITION_STATUS_DISARMED: // 655
        // we do not have a partition condition -- that should come in another message
        if (!it100->isWaitingForStatusUpdate())
            writeMqtt(QString("%1/partition/%2/event").arg(mqttTopicPrefix).arg(partition),"disarmed");
            writeLog(QString("Alarm partition %1 disarmed").arg(partition), LOG_LEVEL_NOTICE);
            graylog->sendMessage(QString("Partition %1 Disarmed!").arg(partition), LevelInformational);
        writeMqtt(QString("%1/partition/%2/hass_state").arg(mqttTopicPrefix).arg(partition),"disarmed",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/armed").arg(mqttTopicPrefix).arg(partition),"0",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/state").arg(mqttTopicPrefix).arg(partition),"disarmed",QOS_1,true);
        panel.partition(partition)->armed = false;
        break;

    case PARTITION_STATUS_READY:
        if (!it100->isWaitingForStatusUpdate())
            writeMqtt(QString("%1/partition/%2/event").arg(mqttTopicPrefix).arg(partition),"disarmed");
            graylog->sendMessage(QString("Partition %1 is Ready").arg(partition), LevelInformational);
        writeMqtt(QString("%1/partition/%2/armed").arg(mqttTopicPrefix).arg(partition),"0",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/hass_state").arg(mqttTopicPrefix).arg(partition),"disarmed",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/state").arg(mqttTopicPrefix).arg(partition),"disarmed",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/condition").arg(mqttTopicPrefix).arg(partition),"ready");
        break;

    case PARTITION_STATUS_NOT_READY:
        if (!it100->isWaitingForStatusUpdate())
            writeMqtt(QString("%1/partition/%2/event").arg(mqttTopicPrefix).arg(partition),"not_ready");
            graylog->sendMessage(QString("Partition %1 NOT Ready").arg(partition), LevelInformational);
        writeMqtt(QString("%1/partition/%2/armed").arg(mqttTopicPrefix).arg(partition),"0",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/hass_state").arg(mqttTopicPrefix).arg(partition),"disarmed",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/state").arg(mqttTopicPrefix).arg(partition),"disarmed",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/condition").arg(mqttTopicPrefix).arg(partition),"not_ready",QOS_1,true);
        break;
        
    case PARTITION_STATUS_BUSY:
        if (!it100->isWaitingForStatusUpdate())
            writeMqtt(QString("%1/partition/%2/event").arg(mqttTopicPrefix).arg(partition),"busy");
        writeMqtt(QString("%1/partition/%2/armed").arg(mqttTopicPrefix).arg(partition),"0",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/hass_state").arg(mqttTopicPrefix).arg(partition),"disarmed",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/state").arg(mqttTopicPrefix).arg(partition),"busy",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/condition").arg(mqttTopicPrefix).arg(partition),"busy",QOS_1,true);
        break;

    case PARTITION_STATUS_READY_FORCE_ARM:
        if (!it100->isWaitingForStatusUpdate())
            writeMqtt(QString("%1/partition/%2/event").arg(mqttTopicPrefix).arg(partition),"ready_force_arm");
            graylog->sendMessage(QString("Partition %1 Ready to Force Arm").arg(partition), LevelInformational);
        writeMqtt(QString("%1/partition/%2/armed").arg(mqttTopicPrefix).arg(partition),"0",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/hass_state").arg(mqttTopicPrefix).arg(partition),"disarmed",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/state").arg(mqttTopicPrefix).arg(partition),"disarmed",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/condition").arg(mqttTopicPrefix).arg(partition),"ready_force_arm",QOS_1,true);
        break;

    case PARTITION_STATUS_EXIT_DELAY_IN_PROGRESS:
        if (!it100->isWaitingForStatusUpdate())
            writeMqtt(QString("%1/partition/%2/event").arg(mqttTopicPrefix).arg(partition),"exit_delay");
            graylog->sendMessage(QString("Partition %1 Exit Delay in Progress").arg(partition), LevelInformational);
        writeMqtt(QString("%1/partition/%2/hass_state").arg(mqttTopicPrefix).arg(partition),"arming",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/state").arg(mqttTopicPrefix).arg(partition),"exit_delay",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/condition").arg(mqttTopicPrefix).arg(partition),"exit_delay",QOS_1,true);
        break;

    case PARTITION_STATUS_ENTRY_DELAY_IN_PROGRESS:
        if (!it100->isWaitingForStatusUpdate())
            writeMqtt(QString("%1/partition/%2/event").arg(mqttTopicPrefix).arg(partition),"entry_delay");
            graylog->sendMessage(QString("Partition %1 Entry Delay in Progress").arg(partition), LevelInformational);
        writeMqtt(QString("%1/partition/%2/hass_state").arg(mqttTopicPrefix).arg(partition),"pending",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/state").arg(mqttTopicPrefix).arg(partition),"entry_delay",QOS_1,true);
        writeMqtt(QString("%1/partition/%2/condition").arg(mqttTopicPrefix).arg(partition),"entry_delay",QOS_1,true);
        break;

    // we need to break this out as a seperate EVENT as it tells us which user armed
    
//    case PARTITION_STATUS_USER_CLOSING:
//        if (!it100->isWaitingForStatusUpdate())
//            writeMqtt(QString("%1/partition/%2/event").arg(mqttTopicPrefix).arg(partition),"user_closing");
//        writeMqtt(QString("%1/partition/%2/event").arg(mqttTopicPrefix).arg(partition),"user_closing");
//        break;

    case PARTITION_STATUS_PARTIAL_CLOSING:
        writeMqtt(QString("%1/partition/%2/event").arg(mqttTopicPrefix).arg(partition),"partial_closing");
        break;

    case PARTITION_STATUS_SPECIAL_CLOSING:
        writeMqtt(QString("%1/partition/%2/event").arg(mqttTopicPrefix).arg(partition),"special_closing");
        break;

    case PARTITION_STATUS_INVALID_ACCESS_CODE:
        writeMqtt(QString("%1/partition/%2/event").arg(mqttTopicPrefix).arg(partition),"invalid_access_code");
        writeLog(QString("Invalid access code on Partition %1").arg(partition), LOG_LEVEL_ERROR);
        graylog->sendMessage(QString("Partition %1 Invalid Access Code!").arg(partition), LevelNotice);
        break;

    case PARTITION_STATUS_FUNCTION_NOT_AVAILABLE:
        writeMqtt(QString("%1/partition/%2/event").arg(mqttTopicPrefix).arg(partition),"function_not_available");
        writeLog(QString("Invalid access code on Partition %1").arg(partition), LOG_LEVEL_ERROR);
        break;

    }
    writeLog(QString("PartitionStatusChange(partition=%1,status=%2)").arg(partition).arg((quint8)status));
}

void It100Mqtt::onIt100TroubleEvent(TroubleEvent event)
{

    /*
        TODO: Implement the following checks:
        TROUBLE_TLM_1,
        TROUBLE_TLM_1_RESTORE,
        TROUBLE_TLM_2,
        TROUBLE_TLM_2_RESTORE,
        TROUBLE_FTC,
        TROUBLE_BUFFER_NEAR_FULL,
        TROUBLE_WIRELESS_KEY_LOW_BATTERY,
        TROUBLE_WIRELESS_KEY_LOW_BATTERY_RESTORE,
        TROUBLE_HANDHELD_KEYPAD_LOW_BATTERY,
        TROUBLE_HANDHELD_KEYPAD_LOW_BATTERY_RESTORE,
        TROUBLE_HOME_AUTOMATION,
        TROUBLE_HOME_AUTOMATION_RESTORE
      */

    switch (event) {

    case TROUBLE_PANEL_BATTERY:
        graylog->sendMessage("Trouble: Panel Battery", LevelError);
        writeMqtt(QString("%1/event").arg(mqttTopicPrefix),"trouble_panel_battery");
        break;

    case TROUBLE_PANEL_BATTERY_RESTORE:
        graylog->sendMessage("Trouble: Panel Battery Restore", LevelNotice);
        writeMqtt(QString("%1/event").arg(mqttTopicPrefix),"trouble_panel_battery_restore");
        break;

    case TROUBLE_PANEL_AC:
        graylog->sendMessage("Trouble: Panel AC", LevelError);
        writeMqtt(QString("%1/event").arg(mqttTopicPrefix),"trouble_panel_ac");
        break;

    case TROUBLE_PANEL_AC_RESTORE:
        graylog->sendMessage("Trouble: Panel AC Restored", LevelNotice);
        writeMqtt(QString("%1/event").arg(mqttTopicPrefix),"trouble_panel_ac_restore");
        break;

    case TROUBLE_SYSTEM_BELL:
        graylog->sendMessage("Trouble: System Bell", LevelCritical);
        writeMqtt(QString("%1/event").arg(mqttTopicPrefix),"trouble_panel_bell");
        break;

    case TROUBLE_SYSTEM_BELL_RESTORE:
        graylog->sendMessage("Trouble: System Bell Restore", LevelNotice);
        writeMqtt(QString("%1/event").arg(mqttTopicPrefix),"trouble_panel_bell_restore");
        break;

    case TROUBLE_GENERAL_SYSTEM_TAMPER:
        graylog->sendMessage("Trouble: General System Tamper", LevelCritical);
        writeMqtt(QString("%1/event").arg(mqttTopicPrefix),"trouble_general_tamper");
        break;

    case TROUBLE_GENERAL_SYSTEM_TAMPER_RESTORE:
        graylog->sendMessage("Trouble: General System Tamper Restore", LevelNotice);
        writeMqtt(QString("%1/event").arg(mqttTopicPrefix),"trouble_general_tamper_restore");
        break;

    case TROUBLE_GENERAL_DEVICE_LOW_BATTERY:
        graylog->sendMessage("Trouble: General Device Low Battery", LevelError);
        writeMqtt(QString("%1/event").arg(mqttTopicPrefix),"trouble_general_device_low_battery");
        break;

    case TROUBLE_GENERAL_DEVICE_LOW_BATTERY_RESTORE:
        graylog->sendMessage("Trouble: General Device Low Battery Restore", LevelNotice);
        writeMqtt(QString("%1/event").arg(mqttTopicPrefix),"trouble_general_device_low_battery_restore");
        break;

    default:
        break;

    }
}

// Write log entry to MQTT broker
//
void It100Mqtt::writeLog(QString msg, LogLevel level)
{
    writeLog(msg.toStdString().c_str(), level);
}

void It100Mqtt::writeLog(const char *msg, LogLevel level)
{

    // Determine topic level for log level
    //
    QString logLevel = "debug";
    switch (level) {
    case LOG_LEVEL_ERROR:
        logLevel = "error";
        break;
    case LOG_LEVEL_SECURITY:
        logLevel = "security";
        break;
    case LOG_LEVEL_NOTICE:
        logLevel = "notice";
        break;
    default:
        logLevel = "debug";
    }

    // Build and publish MQTT message
    QMQTT::Message ttMsg(mId++,QString("log/%1/%2").arg(mqttClientName).arg(logLevel),QByteArray(msg));
    client->publish(ttMsg);
}
