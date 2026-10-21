#include "flightmodel.h"
#include "theme.h"

#include <QBrush>
#include <QColor>

// hand written containers / sorting shared with the server (no STL)
#include "sorting_algorithms.cpp"

namespace {

// comparator object for Sorting::mergeSort : <0, 0, >0
class FlightLess {
public:
    FlightLess(int column, bool ascending) : m_col(column), m_asc(ascending) {}
    int operator()(const FlightRecord &a, const FlightRecord &b) const {
        const int r = compare(a, b);
        return m_asc ? r : -r;
    }
private:
    static int cmpInt(long long x, long long y) { return x < y ? -1 : (x > y ? 1 : 0); }
    static int cmpStr(const QString &x, const QString &y) { return x < y ? -1 : (x > y ? 1 : 0); }
    int compare(const FlightRecord &a, const FlightRecord &b) const {
        switch (m_col) {
        case 0: {
            const int ka = (a.kind == QLatin1String("ROUTE")) ? 0 : 1;
            const int kb = (b.kind == QLatin1String("ROUTE")) ? 0 : 1;
            if (ka != kb) return ka < kb ? -1 : 1;
            return cmpInt(a.index, b.index);
        }
        case 1: return cmpStr(a.id, b.id);
        case 2: return cmpStr(a.origin + a.dest, b.origin + b.dest);
        case 3: return cmpStr(a.date, b.date);
        case 4: return cmpStr(a.date + a.depTime, b.date + b.depTime);
        case 5: return cmpStr(a.arrText, b.arrText);
        case 6: return cmpInt(a.durationMin, b.durationMin);
        case 7: return cmpInt(static_cast<long long>(a.fare * 100), static_cast<long long>(b.fare * 100));
        case 8: return cmpInt(a.seatsAvail, b.seatsAvail);
        default: return cmpInt(a.bookable ? 0 : 1, b.bookable ? 0 : 1);
        }
    }
    int m_col;
    bool m_asc;
};

} // namespace

FlightModel::FlightModel(QObject *parent)
    : QAbstractTableModel(parent), m_sortColumn(0), m_sortOrder(Qt::AscendingOrder) {}

int FlightModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_flights.size();
}

int FlightModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 10;
}

QVariant FlightModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_flights.size()) return QVariant();
    const FlightRecord &f = m_flights.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:
            if (f.kind == QLatin1String("ROUTE")) return QStringLiteral("Route leg %1").arg(f.index);
            return QStringLiteral("Other direct");
        case 1: return f.id;
        case 2: return f.origin + QString::fromUtf8(" \xE2\x86\x92 ") + f.dest;      // →
        case 3: return f.date;
        case 4: return f.depTime;
        case 5: return f.arrText.mid(11);
        case 6: return Theme::duration(f.durationMin);
        case 7: return Theme::money(f.fare);
        case 8: return QStringLiteral("%1 / %2").arg(f.seatsAvail).arg(f.seatsTotal);
        case 9: return f.bookable ? (f.seatsAvail > 0 ? QStringLiteral("Open") : QStringLiteral("Sold out")) : f.reason;
        }
    } else if (role == Qt::ForegroundRole) {
        if (index.column() == 9) {
            if (!f.bookable || f.seatsAvail <= 0) return QColor("#b91c1c");
            return QColor("#15803d");
        }
    } else if (role == Qt::BackgroundRole) {
        if (f.kind == QLatin1String("ROUTE")) return QColor("#eff6ff");
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() >= 6 && index.column() <= 8) return QVariant(int(Qt::AlignCenter));
    } else if (role == Qt::ToolTipRole) {
        if (!f.bookable) return f.reason;
    }
    return QVariant();
}

QVariant FlightModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return QVariant();
    static const char *headers[] = {"Journey", "Flight", "Route", "Date", "Departs", "Arrives",
                                    "Duration", "Fare", "Seats left", "Status"};
    if (section < 0 || section > 9) return QVariant();
    return QString::fromLatin1(headers[section]);
}

void FlightModel::sort(int column, Qt::SortOrder order)
{
    m_sortColumn = column;
    m_sortOrder = order;

    DataStructures::DynArray<FlightRecord> arr;           // hand written dynamic array
    for (int i = 0; i < m_flights.size(); ++i) arr.push(m_flights.at(i));
    Sorting::mergeSort(arr, FlightLess(column, order == Qt::AscendingOrder));

    beginResetModel();
    m_flights.clear();
    for (int i = 0; i < arr.size(); ++i) m_flights.append(arr[i]);
    endResetModel();
}

void FlightModel::setFlights(const QVector<FlightRecord> &flights)
{
    beginResetModel();
    m_flights = flights;
    endResetModel();
    sort(m_sortColumn, m_sortOrder);
}

FlightRecord FlightModel::flightAt(int row) const
{
    return m_flights.value(row);
}
