#ifndef ALARMPANEL_H
#define ALARMPANEL_H

#include <QObject>
#include "it100.h"

class Zone;
class Partition;

class AlarmPanel : public QObject
{
    Q_OBJECT
public:
    explicit AlarmPanel(QObject *parent = nullptr);

    Partition *partition(int32_t num);

private:

    QList<Partition*> partitions;


signals:

public slots:
};


class Zone {

};

class Partition {

public:

    QList<Zone*> zones;
    bool armed;
    it100::PartitionArmedMode armed_mode;

};

#endif // ALARMPANEL_H
