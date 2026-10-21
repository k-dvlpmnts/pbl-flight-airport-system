#pragma once

#include <QMainWindow>
#include <QDateTime>
#include <QElapsedTimer>
#include <QSet>
#include <QString>
#include <QVector>

#include "beltdata.h"

class QStackedWidget;
class QLabel;
class QPushButton;
class QTimer;

class BasePage;
class HomePage;
class LoginPage;
class PassengerPortalPage;
class BookTicketPage;
class PassengerDetailsPage;
class BoardingPassPage;
class TrackLuggagePage;
class CheckInPage;
class BeltStaffPage;
class AdminPage;
class SimulatorHomePage;
class SimTimePage;
class SimPushPage;
class SimLossPage;

struct Session {
    QString role;            // "PASSENGER", "STAFF", "ADMIN" or empty
    QString username;
    QString password;        // kept so that admin requests can be authenticated
    QString staffType;       // "CHECKIN" or "BELT" for staff
    void clear() { role.clear(); username.clear(); password.clear(); staffType.clear(); }
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // ---- session / server
    Session &session() { return m_session; }
    Session &adminSession() { return m_admin; }          // credentials of the administrator in the admin panel
    bool serverOnline() const { return m_online; }
    bool serverSimulated() const { return m_simulated; }
    QDateTime serverNow() const;                       // the system clock (may be simulated)
    QDate serverDate() const { return serverNow().date(); }
    void setServerEndpoint(const QString &host, quint16 port);   // remembered between runs
    const BeltSnapshot &belt() const { return m_belt; }

    // ---- navigation (Back goes to the previous screen, Logout to the first page)
    void navigateTo(BasePage *page);
    void navigateReplacing(BasePage *page, BasePage *keepHistoryUpTo);
    void goBack();
    void logout();
    void onLoggedIn(const QString &role, const QString &user, const QString &password, const QString &staffType);

    // ---- pages
    HomePage *homePage() const { return m_home; }
    LoginPage *passengerLogin() const { return m_passengerLogin; }
    LoginPage *staffLogin() const { return m_staffLogin; }
    LoginPage *adminLogin() const { return m_adminLogin; }
    PassengerPortalPage *passengerPortal() const { return m_passengerPortal; }
    BookTicketPage *bookPage() const { return m_bookPage; }
    PassengerDetailsPage *detailsPage() const { return m_detailsPage; }
    BoardingPassPage *boardingPage() const { return m_boardingPage; }
    TrackLuggagePage *trackPage() const { return m_trackPage; }
    CheckInPage *checkInPage() const { return m_checkInPage; }
    BeltStaffPage *beltStaffPage() const { return m_beltStaffPage; }
    AdminPage *adminPage() const { return m_adminPage; }
    SimulatorHomePage *simHome() const { return m_simHome; }
    SimTimePage *simTime() const { return m_simTime; }
    SimPushPage *simPush() const { return m_simPush; }
    SimLossPage *simLoss() const { return m_simLoss; }

signals:
    void beltUpdated(const BeltSnapshot &snapshot);    // emitted about once a second for passengers / belt staff

private slots:
    void onTick();

private:
    void showPage(BasePage *page);
    void updateBars();
    void requestClock();
    void requestBelt();
    void checkAlerts(const BeltSnapshot &snap);
    void showAlert(const QString &title, const QString &text, bool warning);
    bool wantsBelt() const;

    // chrome
    QStackedWidget *m_stack;
    QLabel *m_titleLabel;
    QLabel *m_clockLabel;
    QPushButton *m_backBtn;
    QPushButton *m_logoutBtn;
    QTimer *m_timer;

    // navigation state
    BasePage *m_current;
    QVector<BasePage *> m_history;
    Session m_session;
    Session m_admin;

    // server clock
    QDateTime m_serverTime;
    QElapsedTimer m_serverClock;
    bool m_online;
    bool m_simulated;
    bool m_clockBusy;
    bool m_beltBusy;

    // luggage alerts
    BeltSnapshot m_belt;
    QSet<QString> m_alertedLost;
    QSet<QString> m_alertedRecovered;

    // pages
    HomePage *m_home;
    LoginPage *m_passengerLogin;
    LoginPage *m_staffLogin;
    LoginPage *m_adminLogin;
    PassengerPortalPage *m_passengerPortal;
    BookTicketPage *m_bookPage;
    PassengerDetailsPage *m_detailsPage;
    BoardingPassPage *m_boardingPage;
    TrackLuggagePage *m_trackPage;
    CheckInPage *m_checkInPage;
    BeltStaffPage *m_beltStaffPage;
    AdminPage *m_adminPage;
    SimulatorHomePage *m_simHome;
    SimTimePage *m_simTime;
    SimPushPage *m_simPush;
    SimLossPage *m_simLoss;
};
