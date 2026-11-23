#pragma once
#include <QWidget>

class MainWindow;

class BasePage : public QWidget {
    Q_OBJECT
public:
    explicit BasePage(MainWindow *app, QWidget *parent = nullptr) : QWidget(parent), m_app(app) {}
    ~BasePage() override {}

    virtual QString pageTitle() const = 0;
    virtual void onEnter() {}      
    virtual void onPoll() {}       
    virtual void onLeave() {}      

protected:
    MainWindow *app() const { return m_app; }

private:
    MainWindow *m_app;
};
