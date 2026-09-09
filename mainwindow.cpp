#include "mainwindow.h"
#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QFont>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {

    setWindowTitle("Portal Menu");
    setStyleSheet("QWidget { background-color: #F0DFDD; }");

    // QMainWindow requires a central widget for layouts
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Buttons
    QPushButton *passengerBtn = new QPushButton("Passenger Portal");
    QPushButton *staffBtn = new QPushButton("Staff Portal");
    QPushButton *exitBtn = new QPushButton("Exit");

    // Fonts
    QFont font("Segoe UI", 12, QFont::Bold);
    passengerBtn->setFont(font);
    staffBtn->setFont(font);
    exitBtn->setFont(font);

    // Styles
    passengerBtn->setStyleSheet("QPushButton { background-color: #B06058; color: white; padding: 10px; border-radius: 8px; } QPushButton:hover { background-color: #CFC0C0; }");
    staffBtn->setStyleSheet("QPushButton { background-color: #B06058; color: white; padding: 10px; border-radius: 8px; } QPushButton:hover { background-color: #CFC0C0; }");
    exitBtn->setStyleSheet("QPushButton { background-color: #B06058; color: white; padding: 10px; border-radius: 8px; } QPushButton:hover { background-color: #CFC0C0; }");

    // Connect (using member functions)
    connect(passengerBtn, &QPushButton::clicked, this, &MainWindow::passenger);
    connect(staffBtn, &QPushButton::clicked, this, &MainWindow::staff);
    connect(exitBtn, &QPushButton::clicked, qApp, &QApplication::quit);

    // Layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(passengerBtn);
    layout->addWidget(staffBtn);
    layout->addWidget(exitBtn);
    layout->setSpacing(15);
    layout->setContentsMargins(50, 50, 50, 50);

    centralWidget->setLayout(layout);
    resize(900, 500);
}

MainWindow::~MainWindow() {}

void MainWindow::passenger() {
    QMessageBox::information(this, "Passenger Portal", "Passenger portal opened!");
}

void MainWindow::staff() {
    QMessageBox::information(this, "Staff Portal", "Staff portal opened!");
}