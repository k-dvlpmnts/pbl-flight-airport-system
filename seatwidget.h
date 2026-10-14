#pragma once
#include <QWidget>
#include <QVector>

class QGridLayout;
class QToolButton;

class SeatWidget : public QWidget {
    Q_OBJECT
public:
    explicit SeatWidget(QWidget *parent = nullptr);
    void populate(const QVector<bool> &sold); // sold[i] == true => seat sold

signals:
    void seatToggled(int seatIndex, bool selected);

private:
    void clearLayout();
    QGridLayout *grid;
    QVector<QToolButton*> buttons;
};
