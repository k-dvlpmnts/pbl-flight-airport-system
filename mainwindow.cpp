#include "mainwindow.h"
#include <QStackedWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {

    setWindowTitle("Portal Menu");
    resize(900, 500);

    // Global stylesheet for beauty
    setStyleSheet(R"(
        QWidget {
            background-color: #F5F0EC;
            font-family: 'Segoe UI';
            color: #333;
        }
        QPushButton {
            background-color: #B06058;
            color: white;
            padding: 10px 20px;
            border-radius: 8px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #CFC0C0;
            color: #222;
        }
        QLineEdit {
            background-color: #FFF;
            border: 2px solid #B06058;
            border-radius: 6px;
            padding: 6px;
        }
        QLabel {
            font-size: 14px;
            font-weight: 600;
        }
    )");

    stack = new QStackedWidget(this);
    setCentralWidget(stack);

    // --- Menu Page ---
    menuPage = new QWidget;
    QVBoxLayout *menuLayout = new QVBoxLayout(menuPage);

    QLabel *menuTitle = new QLabel("Welcome to Flight Airport System");
    menuTitle->setAlignment(Qt::AlignCenter);
    menuTitle->setStyleSheet("font-size: 22px; font-weight: bold; color: #B06058;");
    menuLayout->addWidget(menuTitle);

    QPushButton *passengerBtn = new QPushButton("Passenger Portal");
    QPushButton *staffBtn = new QPushButton("Staff Portal");
    QPushButton *exitBtn = new QPushButton("Exit");

    menuLayout->addWidget(passengerBtn);
    menuLayout->addWidget(staffBtn);
    menuLayout->addWidget(exitBtn);
    menuLayout->setAlignment(Qt::AlignCenter);

    connect(passengerBtn, &QPushButton::clicked, this, &MainWindow::passenger);
    connect(staffBtn, &QPushButton::clicked, this, &MainWindow::staff);
    connect(exitBtn, &QPushButton::clicked, qApp, &QApplication::quit);

    stack->addWidget(menuPage);

    // --- Passenger Login Page ---
    passengerLoginPage = new QWidget;
    QVBoxLayout *pLayout = new QVBoxLayout(passengerLoginPage);

    QLabel *pTitle = new QLabel("Passenger Login");
    pTitle->setAlignment(Qt::AlignCenter);
    pTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #B06058;");
    pLayout->addWidget(pTitle);

    pLayout->addWidget(new QLabel("Username:"));
    QLineEdit *pUser = new QLineEdit;
    pLayout->addWidget(pUser);

    pLayout->addWidget(new QLabel("Password:"));
    QLineEdit *pPass = new QLineEdit;
    pPass->setEchoMode(QLineEdit::Password);
    pLayout->addWidget(pPass);

    QPushButton *pLogin = new QPushButton("Login");
    QPushButton *pBack = new QPushButton("Back");
    pLayout->addWidget(pLogin);
    pLayout->addWidget(pBack);

    connect(pBack, &QPushButton::clicked, this, &MainWindow::goBack);

    stack->addWidget(passengerLoginPage);

    // --- Staff Login Page ---
    staffLoginPage = new QWidget;
    QVBoxLayout *sLayout = new QVBoxLayout(staffLoginPage);

    QLabel *sTitle = new QLabel("Staff Login");
    sTitle->setAlignment(Qt::AlignCenter);
    sTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #B06058;");
    sLayout->addWidget(sTitle);

    sLayout->addWidget(new QLabel("Username:"));
    QLineEdit *sUser = new QLineEdit;
    sLayout->addWidget(sUser);

    sLayout->addWidget(new QLabel("Password:"));
    QLineEdit *sPass = new QLineEdit;
    sPass->setEchoMode(QLineEdit::Password);
    sLayout->addWidget(sPass);

    QPushButton *sLogin = new QPushButton("Login");
    QPushButton *sBack = new QPushButton("Back");
    sLayout->addWidget(sLogin);
    sLayout->addWidget(sBack);

    connect(sBack, &QPushButton::clicked, this, &MainWindow::goBack);

    stack->addWidget(staffLoginPage);

    // Start at menu
    stack->setCurrentWidget(menuPage);
}

MainWindow::~MainWindow() {}

void MainWindow::passenger() {
    stack->setCurrentWidget(passengerLoginPage);
}

void MainWindow::staff() {
    stack->setCurrentWidget(staffLoginPage);
}

void MainWindow::goBack() {
    stack->setCurrentWidget(menuPage);
}
