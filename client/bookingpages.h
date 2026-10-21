#pragma once
// "Book ticket" : search (graph search for the shortest route) -> seat map -> passenger details
#include "basepage.h"
#include "flightmodel.h"
#include <QStringList>
#include <QVector>

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableView;
class QVBoxLayout;
class QWidget;
class SeatWidget;
struct Reply;

class BookTicketPage : public BasePage {
    Q_OBJECT
public:
    explicit BookTicketPage(MainWindow *mw, QWidget *parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("Book ticket"); }
    void onEnter() override;
    void onPoll() override;
    void clearSelection();                     // forget the chosen seats (after a successful booking)

private slots:
    void search();
    void onFlightSelected();
    void continueToDetails();

private:
    void loadStops();
    void loadSeats();
    void applySeats(const Reply &rep);
    void clearSeatPanel();
    void updateSummary();

    QComboBox *m_from;
    QComboBox *m_to;
    QDateEdit *m_date;
    QPushButton *m_searchBtn;
    QLabel *m_routeLabel;
    QLabel *m_routeInfo;
    QTableView *m_table;
    FlightModel *m_model;

    QLabel *m_seatTitle;
    QLabel *m_seatHint;
    QLabel *m_notice;
    SeatWidget *m_seats;
    QLabel *m_summary;
    QPushButton *m_continue;

    FlightRecord m_cur;
    bool m_hasCur;
    bool m_bookable;
    bool m_stopsLoaded;
    int m_pollCount;
};

class PassengerDetailsPage : public BasePage {
    Q_OBJECT
public:
    explicit PassengerDetailsPage(MainWindow *mw, QWidget *parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("Passenger details"); }
    void setBooking(const FlightRecord &leg, const QStringList &seats);

private slots:
    void confirm();

private:
    struct FormRow {
        QString seat;
        QLineEdit *name;
        QSpinBox *age;
        QComboBox *gender;
        QSpinBox *bag;
        FormRow() : name(nullptr), age(nullptr), gender(nullptr), bag(nullptr) {}
    };

    QLabel *m_summary;
    QLabel *m_msg;
    QWidget *m_formHost;
    QVBoxLayout *m_formLayout;
    QPushButton *m_confirm;
    QVector<FormRow> m_rows;
    FlightRecord m_leg;
};
