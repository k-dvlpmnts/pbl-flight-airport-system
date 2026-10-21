#pragma once
// Client side copy of what the server reports about the luggage belt.
#include <QString>
#include <QVector>
#include "serverapi.h"

struct SensorInfo {
    int index;
    QString name;
    QString kind;                    // "RFID" or "PHOTON"
    double distance;                 // metres from the start of the belt
    SensorInfo() : index(0), distance(0) {}
};

struct BagItem {
    QString tag, flight, date, pnr, seat, name;
    int weight;
    QString state;                   // ONBELT, DELIVERED, LOST, RECOVERED
    int passed;                      // highest sensor that saw the bag
    int lostAt;                      // sensor that missed it, -1 if none
    double pos;                      // metres from the start, -1 = not on the belt
    qint64 pushCivil;                // system clock (seconds) when it crossed the start RFID
    QString lostTime, deliveredTime, recoveredInfo;
    BagItem() : weight(0), passed(0), lostAt(-1), pos(0), pushCivil(0) {}
    bool isLost() const { return state == QLatin1String("LOST"); }
};

struct PendingBag {                  // booked with a bag that is not on the belt yet
    QString tag, name, flight, date, dep;
    bool checkedIn;
    int weight;
    PendingBag() : checkedIn(false), weight(0) {}
};

struct BeltSnapshot {
    bool valid;
    QString nowText;
    double speed;
    double length;
    QVector<SensorInfo> sensors;
    QVector<BagItem> bags;           // in belt (doubly linked list) order
    QVector<PendingBag> pending;

    BeltSnapshot() : valid(false), speed(0), length(0) {}
    static BeltSnapshot fromReply(const Reply &r);
    QString sensorName(int index) const;
    int lostCount() const;
};

// "HH:MM:SS" of the moment a bag reaches sensor j, derived from its start time
QString expectedTimeText(const BeltSnapshot &snap, const BagItem &bag, int sensorIndex);
