#pragma once

#include <QAbstractTableModel>
#include <QVector>
#include <QString>
#include <QVariant>
#include <QModelIndex>

// One row of the search result: a leg of the fastest route, or another direct flight.
struct FlightRecord {
    QString kind;            // "ROUTE" (leg of the shortest route) or "ALT" (other direct flight)
    int index;               // leg number
    QString id;              // flight number
    QString origin, dest;
    QString date;            // yyyy-MM-dd
    QString depTime;         // HH:mm
    QString arrText;         // yyyy-MM-dd HH:mm
    int durationMin;
    double fare;
    int seatsTotal;
    int seatsAvail;
    bool bookable;
    QString reason;          // why it cannot be booked (if it cannot)

    FlightRecord() : index(0), durationMin(0), fare(0), seatsTotal(0), seatsAvail(0), bookable(false) {}
};

class FlightModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit FlightModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    // header click sorting - uses the hand written merge sort (sorting_algorithms.cpp)
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    void setFlights(const QVector<FlightRecord> &flights);
    FlightRecord flightAt(int row) const;

private:
    QVector<FlightRecord> m_flights;
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
};
