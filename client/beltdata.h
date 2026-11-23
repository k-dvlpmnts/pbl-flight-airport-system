#pragma once

#include <QString>
#include <QVector>
#include "serverapi.h"

struct SensorInfo {
    int index;
    QString name;
    QString kind;                    
    double distance;                 
    SensorInfo() : index(0), distance(0) {}
};

struct BagItem {
    QString tag, flight, date, pnr, seat, name;
    int weight;
    QString state;                   
    int passed;                      
    int lostAt;                      
    double pos;                      
    qint64 pushCivil;                
    QString lostTime, deliveredTime, recoveredInfo;
    BagItem() : weight(0), passed(0), lostAt(-1), pos(0), pushCivil(0) {}
    bool isLost() const { return state == QLatin1String("LOST"); }
};

struct PendingBag {                  
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
    QVector<BagItem> bags;           
    QVector<PendingBag> pending;

    BeltSnapshot() : valid(false), speed(0), length(0) {}
    static BeltSnapshot fromReply(const Reply &r);
    QString sensorName(int index) const;
    int lostCount() const;
};


QString expectedTimeText(const BeltSnapshot &snap, const BagItem &bag, int sensorIndex);
