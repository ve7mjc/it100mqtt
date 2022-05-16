#ifndef COMMONSERVICE_H
#define COMMONSERVICE_H

#include <QObject>
#include <QDebug>

enum ComponentStatus {
    COMP_STATUS_OK = 0,
    COMP_STATUS_UNKNOWN,
    COMP_STATUS_STARTING,
    COMP_STATUS_FAILED,
};

#endif // COMMONSERVICE_H
