#pragma once

#include <QMainWindow>

class QStackedWidget;
class QWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void passenger();
    void staff();
    void goBack();

private:
    QStackedWidget *stack;
    QWidget *menuPage;
    QWidget *passengerLoginPage;
    QWidget *staffLoginPage;
};
