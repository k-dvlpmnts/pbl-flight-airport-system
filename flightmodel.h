#pragma once

#include <QAbstractTableModel>
#include <QVector>
#include <QString>
#include <QVariant>
#include <QModelIndex>
#include <Qt>

struct FlightRecord {
    QString id;
    QString dep;
    QString arr;
    QString duration;
    double fare;
    int seatsSoldCount;
};

class FlightModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit FlightModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setFlights(const QVector<FlightRecord> &flights);
    FlightRecord flightAt(int row) const;

private:
    QVector<FlightRecord> m_flights;
};
