#pragma once

#include <QDialog>

class QLineEdit;
class QPushButton;

class LoginWindow : public QDialog {
    Q_OBJECT

public:
    explicit LoginWindow(const QString &role, QWidget *parent = nullptr);

private slots:
    void handleLogin();

private:
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QString roleType;
};
