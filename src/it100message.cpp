#include "it100message.h"

namespace it100 {

It100Message::It100Message(QByteArray command, QByteArray data, QObject *parent) :
    QObject(parent)
{
    // default to invalid message state
    valid = false;

    this->command = command;
    this->data = data;

    generatePacket();
}

int It100Message::generatePacket()
{

    // Start off packet with command
    packetData = command;

    // Append data
    if (data.size()>0) packetData.append(data);

    // Append calculated checksum from current payload
    packetData.append(generateChecksum(&packetData));

    // Append token EOL
    packetData.append("\r\n");

    valid = true;
    return false;

}

QByteArray It100Message::generateChecksum(QByteArray *data)
{
    /* Count up HEX (as int) bytes */
    QString hexint = QString(data->toHex());
    int count = 0;
    for (int x = 0; x < data->size(); x++) {
        // hexint = QString(QByteArray(status.at(x)).toHex());
        bool ok;
        count += hexint.mid((x * 2),2).toUInt(&ok,16);
    }

    // Convert number (int) to HEX Ascii for chopping up
    QString asciiint = QString().setNum(count,16);

    // QByteArray checksum = QByteArray().fromHex();
    // TODO, still need to truncate this to 8 bits!

    // Create DCS's silly checksum
    QByteArray checksum;
    checksum.append(asciiint.mid(asciiint.size()-2,1).toUpper());
    checksum.append(asciiint.mid(asciiint.size()-1,1).toUpper());

    return checksum;
}

} // namespace it100
