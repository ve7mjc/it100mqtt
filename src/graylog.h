#ifndef GRAYLOG_H
#define GRAYLOG_H

#include <QObject>

#include <QUdpSocket>

enum AlertLevel {
    LevelAlert = 1,
    LevelCritical = 2,
    LevelError = 3,
    LevelWarning = 4,
    LevelNotice = 5,
    LevelInformational = 6,
    LevelDebug = 7
};

class Graylog : public QObject
{
    Q_OBJECT
public:

    explicit Graylog(QObject *parent = 0);
    Graylog(QString hostName, QString remoteHost, quint16 remotePort, QObject *parent = 0);

    void sendMessage(QString messageShort, AlertLevel level = LevelInformational, QString messageFull = QString());

    bool isEnabled() { return _enabled; }
    void setEnabled(bool val = true) { _enabled = val; }

private:

    bool _initialized;
    bool _enabled;

    QString _hostName;
    QString _remoteHost;
    quint16 _remotePort;

    QUdpSocket *_udpSocket;


signals:

public slots:
};

#endif // GRAYLOG_H
