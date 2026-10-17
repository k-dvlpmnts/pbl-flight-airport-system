#pragma once
// Login screen used for passengers, staff and administrators (replaces the old
// LoginWindow dialog: the check is now done by the server against the .dat files).
#include "basepage.h"

class QLineEdit;
class QLabel;
class QPushButton;

class LoginPage : public BasePage {
    Q_OBJECT
public:
    // role: "PASSENGER", "STAFF" or "ADMIN"
    LoginPage(MainWindow *mw, const QString &role, QWidget *parent = nullptr);

    QString pageTitle() const override;
    void onEnter() override;

private slots:
    void doLogin();
    void doRegister();

private:
    void showMessage(const QString &text, bool good);
    void setBusy(bool busy);

    QString m_role;
    QString m_title;
    QLineEdit *m_user;
    QLineEdit *m_pass;
    QLabel *m_msg;
    QPushButton *m_loginBtn;
    QPushButton *m_registerBtn;
    QPushButton *m_adminBtn;
};
