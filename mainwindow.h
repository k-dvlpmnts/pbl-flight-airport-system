#pragma once

#include <QMainWindow>
#include <QDateTime>
#include <QElapsedTimer>
#include <QSet>
#include <QString>
#include <QVector>


class QStackedWidget;
class QLabel;
class QPushButton;

class BasePage;
class HomePage;
class LoginPage;

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

private:
    void showPage(BasePage *page);
    QStackedWidget *m_stack;
    QLabel *m_titleLabel;
    QLabel *m_clockLabel;
    QPushButton *m_backBtn;
    QPushButton *m_logoutBtn;

    // navigation state
    BasePage *m_current;
    QVector<BasePage *> m_history;
    
    HomePage *m_home;
    LoginPage *m_passengerLogin;
    LoginPage *m_staffLogin;
    LoginPage *m_adminLogin;
};
