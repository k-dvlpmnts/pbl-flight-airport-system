#pragma once

#include <QMainWindow>

class QStackedWidget;
class QWidget;
class QComboBox;
class QDateEdit;
class QPushButton;
class QTableView;
class FlightModel;
class SeatWidget;
class QItemSelection;
class QLineEdit;
class QLabel;
class QNetworkAccessManager;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void passenger();
    void staff();
    void goBack();
    void ticket();
    void onSearchFlights();
    void onFlightSelected(const QItemSelection &selected, const QItemSelection &deselected);
    void validateUser(const QString &username, const QString &password, bool isStaff);
    void registerUser(const QString &username, const QString &password, bool isStaff);
    void populateStopsFromServer();
    void onSeatToggled(int seatIndex, bool selected);
    void bookSelectedFlight();

private:
    QStackedWidget *stack;
    QWidget *menuPage;
    QWidget *passengerLoginPage;
    QWidget *staffLoginPage;
    QWidget *passengerPortalPage;
    QWidget *staffPortalPage;
    QWidget *ticketing;

    // UI elements
    QComboBox *fromCombo;
    QComboBox *toCombo;
    QDateEdit *dateEdit;
    QPushButton *searchBtn;
    QPushButton *bookBtn;
    QTableView *flightView;
    FlightModel *flightModel;
    SeatWidget *seatWidget;
    QLabel *routeLabel;
    QLabel *fareLabel;

    // Login fields
    QLineEdit *pUser;
    QLineEdit *pPass;
    QLineEdit *sUser;
    QLineEdit *sPass;
    QString loggedInUsername;

    // Network manager
    QNetworkAccessManager *net;
    QList<int> selectedSeats;
    double currentFlightFare;
};