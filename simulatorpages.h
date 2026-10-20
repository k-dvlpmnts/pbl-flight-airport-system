#pragma once
// Simulator: change the system date/time, put luggage on the belt, simulate a bag loss.
#include "basepage.h"
#include "beltdata.h"
#include <QStringList>
#include <QVector>

class QComboBox;
class QDateTimeEdit;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;

// Table of the flights departing within 3 hours of the system time (shared by two screens).
class SimFlightList : public QWidget {
    Q_OBJECT
public:
    explicit SimFlightList(QWidget *parent = nullptr);
    void refresh();                         // asks the server again
    bool hasSelection() const { return !m_flight.isEmpty(); }
    QString flightId() const { return m_flight; }
    QString date() const { return m_date; }
    void forgetSelection();

signals:
    void selectionChanged();

private:
    void fill(const QVector<QStringList> &rows);

    QTableWidget *m_table;
    QLabel *m_info;
    QString m_flight;
    QString m_date;
    QString m_signature;
};

class SimulatorHomePage : public BasePage {
    Q_OBJECT
public:
    explicit SimulatorHomePage(MainWindow *mw, QWidget *parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("Simulator"); }
    void onEnter() override;
    void onPoll() override;

private:
    QLabel *m_clock;
};

class SimTimePage : public BasePage {
    Q_OBJECT
public:
    explicit SimTimePage(MainWindow *mw, QWidget *parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("Simulator - date and time"); }
    void onEnter() override;
    void onPoll() override;

private slots:
    void apply();
    void resetToRealTime();

private:
    void shift(qint64 seconds);

    QLabel *m_now;
    QDateTimeEdit *m_edit;
    QLabel *m_msg;
};

class SimPushPage : public BasePage {
    Q_OBJECT
public:
    explicit SimPushPage(MainWindow *mw, QWidget *parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("Simulator - put luggage on the belt"); }
    void onEnter() override;
    void onPoll() override;

private slots:
    void push();

private:
    void loadPassengers();
    void fillPassengers(const QVector<QStringList> &rows);
    void updateButton();

    SimFlightList *m_flights;
    QTableWidget *m_pax;
    QPushButton *m_pushBtn;
    QPlainTextEdit *m_log;
    QLabel *m_clock;
    QString m_selPnr, m_selSeat, m_selStatus;
    QString m_signature;
    int m_pollCount;
};

class SimLossPage : public BasePage {
    Q_OBJECT
public:
    explicit SimLossPage(MainWindow *mw, QWidget *parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("Simulator - bag loss"); }
    void onEnter() override;
    void onPoll() override;

private slots:
    void triggerLoss();

private:
    void loadBags();
    void fillBags(const QVector<QStringList> &rows);
    void updateControls();

    SimFlightList *m_flights;
    QTableWidget *m_bags;
    QComboBox *m_sensor;
    QPushButton *m_lossBtn;
    QLabel *m_msg;
    QLabel *m_clock;
    QVector<SensorInfo> m_sensors;
    QVector<QStringList> m_rows;
    QString m_selPnr, m_selSeat;
    QString m_signature;
    int m_comboForPassed;
    int m_pollCount;
};
