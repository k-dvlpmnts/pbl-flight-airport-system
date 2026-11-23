#pragma once

#include <QAbstractTableModel>
#include <QVector>
#include <QString>
#include <QVariant>
#include <QModelIndex>

struct FlightRecord {
    QString kind;            
    int index;               
    QString id;              
    QString origin, dest;
    QString date;            
    QString depTime;         
    QString arrText;         
    int durationMin;
    double fare;
    int seatsTotal;
    int seatsAvail;
    bool bookable;
    QString reason;          

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
    
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    void setFlights(const QVector<FlightRecord> &flights);
    FlightRecord flightAt(int row) const;

private:
    QVector<FlightRecord> m_flights;
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
};
