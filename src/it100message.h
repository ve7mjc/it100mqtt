#ifndef IT100MESSAGE_H
#define IT100MESSAGE_H

#include <QObject>

namespace it100 {

class It100Message : public QObject
{
    Q_OBJECT
public:
    explicit It100Message(QByteArray command, QByteArray data = "", QObject *parent = 0);

    QByteArray *packet() { return &packetData; }
    bool isValid() { return valid; }

private:

    QByteArray command;
    QByteArray data;

    QByteArray packetData;
    bool valid;

    int generatePacket();
    QByteArray generateChecksum(QByteArray *data);

signals:

public slots:

};

} // namespace it100

#endif // IT100MESSAGE_H
