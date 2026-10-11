#include "loginwindow.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

LoginWindow::LoginWindow(const QString &role, QWidget *parent)
    : QDialog(parent), roleType(role) {

    setWindowTitle(role + " Login");

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *userLabel = new QLabel("Username:");
    usernameEdit = new QLineEdit;

    QLabel *passLabel = new QLabel("Password:");
    passwordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);

    QPushButton *loginBtn = new QPushButton("Login");

    layout->addWidget(userLabel);
    layout->addWidget(usernameEdit);
    layout->addWidget(passLabel);
    layout->addWidget(passwordEdit);
    layout->addWidget(loginBtn);

    connect(loginBtn, &QPushButton::clicked, this, &LoginWindow::handleLogin);
}

void LoginWindow::handleLogin() {
    QString username = usernameEdit->text();
    QString password = passwordEdit->text();

    // Simple check (replace with real authentication later)
    if (!username.isEmpty() && !password.isEmpty()) {
        QMessageBox::information(this, "Success", roleType + " logged in successfully!");
        accept(); // close dialog
    } else {
        QMessageBox::warning(this, "Error", "Please enter both username and password.");
    }
}
