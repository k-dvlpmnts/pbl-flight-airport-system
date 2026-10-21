#include "beltdata.h"
#include <QDateTime>

BeltSnapshot BeltSnapshot::fromReply(const Reply &r)
{
    BeltSnapshot s;
    if (!r.ok) return s;
    s.valid = true;
    s.nowText = r.headField(1);
    s.speed = r.headField(2).toDouble();
    s.length = r.headField(3).toDouble();

    const QVector<QStringList> sensors = r.rowsOf(QStringLiteral("SENSOR"));
    for (int i = 0; i < sensors.size(); ++i) {
        SensorInfo si;
        si.index = fld(sensors.at(i), 1).toInt();
        si.name = fld(sensors.at(i), 2);
        si.kind = fld(sensors.at(i), 3);
        si.distance = fld(sensors.at(i), 4).toDouble();
        s.sensors.append(si);
    }
    const QVector<QStringList> bags = r.rowsOf(QStringLiteral("BAG"));
    for (int i = 0; i < bags.size(); ++i) {
        const QStringList &b = bags.at(i);
        BagItem bi;
        bi.tag = fld(b, 1);
        bi.flight = fld(b, 2);
        bi.date = fld(b, 3);
        bi.pnr = fld(b, 4);
        bi.seat = fld(b, 5);
        bi.name = fld(b, 6);
        bi.weight = fld(b, 7).toInt();
        bi.state = fld(b, 8);
        bi.passed = fld(b, 9).toInt();
        bi.lostAt = fld(b, 10).toInt();
        bi.pos = fld(b, 11).toDouble();
        bi.pushCivil = fld(b, 12).toLongLong();
        bi.lostTime = fld(b, 13);
        bi.deliveredTime = fld(b, 14);
        bi.recoveredInfo = fld(b, 15);
        s.bags.append(bi);
    }
    const QVector<QStringList> pend = r.rowsOf(QStringLiteral("PENDING"));
    for (int i = 0; i < pend.size(); ++i) {
        const QStringList &p = pend.at(i);
        PendingBag pb;
        pb.tag = fld(p, 1);
        pb.name = fld(p, 2);
        pb.flight = fld(p, 3);
        pb.date = fld(p, 4);
        pb.dep = fld(p, 5);
        pb.checkedIn = fld(p, 6) == QLatin1String("1");
        pb.weight = fld(p, 7).toInt();
        s.pending.append(pb);
    }
    return s;
}

QString BeltSnapshot::sensorName(int index) const
{
    for (int i = 0; i < sensors.size(); ++i)
        if (sensors.at(i).index == index) return sensors.at(i).name;
    return QStringLiteral("sensor %1").arg(index);
}

int BeltSnapshot::lostCount() const
{
    int n = 0;
    for (int i = 0; i < bags.size(); ++i)
        if (bags.at(i).isLost()) ++n;
    return n;
}

QString expectedTimeText(const BeltSnapshot &snap, const BagItem &bag, int sensorIndex)
{
    if (snap.speed <= 0) return QStringLiteral("-");
    for (int i = 0; i < snap.sensors.size(); ++i) {
        if (snap.sensors.at(i).index != sensorIndex) continue;
        const qint64 secs = bag.pushCivil + static_cast<qint64>(snap.sensors.at(i).distance / snap.speed);
        const int tod = static_cast<int>(((secs % 86400) + 86400) % 86400);
        return QStringLiteral("%1:%2:%3")
            .arg(tod / 3600, 2, 10, QLatin1Char('0'))
            .arg((tod / 60) % 60, 2, 10, QLatin1Char('0'))
            .arg(tod % 60, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("-");
}
