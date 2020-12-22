#include "graylog.h"

#include <QDebug>

Graylog::Graylog(QObject *parent) : QObject(parent)
{

    _initialized = false;

}

Graylog::Graylog(QString hostName, QString remoteHost, quint16 remotePort, QObject *parent)
 : QObject(parent)
{

    _hostName = hostName;
    _remoteHost = remoteHost;
    _remotePort = remotePort;

    _udpSocket = new QUdpSocket(this);

    _initialized = true;

}

void Graylog::sendMessage(QString messageShort, AlertLevel level, QString messageFull)
{

    // do not proceed if not configured
    if (!_initialized || !_enabled) return;

    QString msg = QString("{ \"version\": \"1.1\", \"host\": \"%1\", \"level\": \"%2\", \"short_message\": \"%3\"").arg(_hostName).arg(QString::number(level)).arg(messageShort);
    if (!messageFull.isEmpty()) msg.append(", \"message_full\": \"%1\" ").arg(messageFull);
    msg.append("}"); // close it up!
    qDebug() << qPrintable(QString("Graylog (%1:%2): %3").arg(_remoteHost).arg(QString(_remotePort)).arg(msg));
    _udpSocket->writeDatagram(msg.toUtf8(),QHostAddress(_remoteHost),_remotePort);

}
