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
    QTableView *flightView;
    FlightModel *flightModel;
    SeatWidget *seatWidget;

    // Login fields
    QLineEdit *pUser;
    QLineEdit *pPass;
    QLineEdit *sUser;
    QLineEdit *sPass;

    // Network manager
    QNetworkAccessManager *net;
};