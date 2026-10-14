#pragma once
#include <QMainWindow>
#include <QList>

class QStackedWidget;
class QWidget;
class QComboBox;
class QDateEdit;
class QPushButton;
class QTableView;
class QListWidget;
class QLineEdit;
class QLabel;
class QNetworkAccessManager;
class FlightModel;
class SeatWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);

private slots:
    void goBack();
    void logout();
    void onPassengerLogin();
    void onStaffLogin();
    void onAdminLogin();
    void openSimulator();
    void openBooking();
    void openTrackLuggage();
    void onSearch();
    void onBookConfirm();
    void onBagPush();
    void onBagLoss();
    void onBagSafe();
    void refreshBagList();

private:
    QStackedWidget* stack;

    QWidget* firstPage;
    QWidget* passengerLoginPage;
    QWidget* staffLoginPage;
    QWidget* adminLoginPage;
    QWidget* adminPanelPage;
    QWidget* passengerPortalPage;
    QWidget* staffPortalPage;
    QWidget* bookingPage;
    QWidget* trackPage;
    QWidget* boardingPassPage;
    QWidget* simulatorPage;

    QLineEdit *pUser, *pPass, *sUser, *sPass, *aUser, *aPass;
    QLineEdit *newStaffUser, *newStaffPass;
    QComboBox *newStaffRole;
    QComboBox *fromCombo, *toCombo;
    QDateEdit *dateEdit;
    QTableView *flightView;
    FlightModel *flightModel;
    SeatWidget *seatWidget;
    QLabel *routeLabel, *fareLabel;
    QListWidget *bagList, *boardingList;
    QPushButton *bookBtn;

    QNetworkAccessManager* net;
    QString serverBase;
    QString loggedInUser;
    QString loggedInRole;
    QString currentFlight;
    QString currentDate;
    double currentFare;
    QList<int> selectedSeats;
    QList<QWidget*> history;
    QListWidget *bagList, *boardingList, *simBagList;
    QPushButton* backBtn();
    QPushButton* logoutBtn();
    void pushAndGo(QWidget* w);
};