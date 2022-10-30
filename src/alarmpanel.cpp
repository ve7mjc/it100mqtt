#include "alarmpanel.h"

AlarmPanel::AlarmPanel(QObject *parent) : QObject(parent)
{

}

Partition *AlarmPanel::partition(int32_t num) {

    // add knowledge of partition if it does not exist
    if (partitions.count()<num) {
        for(int x=partitions.count(); x < num; x++) {
            qDebug() << "requested partition " << num << " adding x = " << x;
            partitions.append(new Partition());
        }
    }

    return partitions.at(num-1);

}
