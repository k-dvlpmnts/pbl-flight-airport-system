#pragma once
// Check-in counter (staff) and the administrator panel (creates staff accounts).
#include "basepage.h"
#include <QStringList>
#include <QVector>

class QComboBox;
class QDateEdit;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

class CheckInPage : public BasePage {
    Q_OBJECT
public:
    explicit CheckInPage(MainWindow *mw, QWidget *parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("Check-in counter"); }
    void onEnter() override;
    void onPoll() override;

private slots:
    void doCheckIn();

private:
    void loadFlights();
    void loadPassengers();
    void fillFlights(const QVector<QStringList> &rows);
    void fillPassengers();
    void updateForm();
    QString isoDate() const;

    QDateEdit *m_date;
    QTableWidget *m_flights;
    QTableWidget *m_pax;
    QLineEdit *m_filter;
    QLabel *m_banner;
    QLabel *m_selected;
    QLineEdit *m_code;
    QPushButton *m_checkBtn;
    QLabel *m_msg;

    QString m_flightId;
    QString m_selPnr;
    QString m_selSeat;
    QVector<QStringList> m_paxRows;
    QString m_flightSig;
    QString m_paxSig;
    bool m_open;
    QString m_closes;
    bool m_dateInit;
    int m_pollCount;
};

class AdminPage : public BasePage {
    Q_OBJECT
public:
    explicit AdminPage(MainWindow *mw, QWidget *parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("Administrator - staff accounts"); }
    void onEnter() override;

private slots:
    void createStaff();

private:
    void loadStaff();

    QLineEdit *m_user;
    QLineEdit *m_pass;
    QComboBox *m_type;
    QLabel *m_msg;
    QTableWidget *m_table;
};
