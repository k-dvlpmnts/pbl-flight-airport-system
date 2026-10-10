#include "flightmodel.h"

FlightModel::FlightModel(QObject *parent) : QAbstractTableModel(parent) {}

int FlightModel::rowCount(const QModelIndex&) const { return m_flights.size(); }
int FlightModel::columnCount(const QModelIndex&) const { return 6; }

QVariant FlightModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) return {};
    const auto &f = m_flights.at(index.row());
    switch (index.column()) {
        case 0: return f.id;
        case 1: return f.dep;
        case 2: return f.arr;
        case 3: return f.duration;
        case 4: return QString::number(f.fare, 'f', 2);
        case 5: return QString::number(qMax(0, 30 - f.seatsSoldCount)); // seats available demo
    }
    return {};
}

QVariant FlightModel::headerData(int section, Qt::Orientation, int role) const {
    if (role != Qt::DisplayRole) return {};
    static const char* headers[] = {"Flight","Dep","Arr","Duration","Fare","Seats"};
    return headers[section];
}

void FlightModel::setFlights(const QVector<FlightRecord> &flights) {
    beginResetModel();
    m_flights = flights;
    endResetModel();
}

FlightRecord FlightModel::flightAt(int row) const {
    return m_flights.value(row);
}