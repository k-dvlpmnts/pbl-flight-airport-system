#pragma once
#include <QWidget>
#include <QVector>
#include <QStringList>

class QGridLayout;
class QToolButton;

// Aircraft seat map: 6 seats per row (A B C | D E F), aisle in the middle.
class SeatWidget : public QWidget {
    Q_OBJECT
public:
    explicit SeatWidget(QWidget *parent = nullptr);

    void setSeatCount(int total);                     // rebuilds the map when the size changes
    int seatCount() const { return m_buttons.size(); }
    void setSold(const QStringList &soldLabels);      // updates in place, keeps the user's selection
    void setInteractive(bool on);                     // false => nothing can be selected
    QStringList selectedLabels() const;
    void clearSelection();

    static QString labelFor(int index);               // 0 -> "1A", 7 -> "2B"

signals:
    void seatToggled(int seatIndex, bool selected);
    void selectionChanged();
    void seatTaken(const QString &label);             // a seat the user selected was just sold to someone else

private:
    void rebuild(int total);
    void clearLayout();
    void applyStates();

    QGridLayout *m_grid;
    QVector<QToolButton *> m_buttons;
    QVector<bool> m_sold;
    bool m_interactive;
};
