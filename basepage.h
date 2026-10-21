#pragma once
#include <QWidget>

class MainWindow;

// Every screen of the application.  MainWindow shows the title, the Back
// button (top left) and the Logout button (top right) around it.
class BasePage : public QWidget {
    Q_OBJECT
public:
    explicit BasePage(MainWindow *app, QWidget *parent = nullptr) : QWidget(parent), m_app(app) {}
    ~BasePage() override {}

    virtual QString pageTitle() const = 0;
    virtual void onEnter() {}      // the page has just become visible
    virtual void onPoll() {}       // called about once a second while the page is visible
    virtual void onLeave() {}      // the page is about to be hidden

protected:
    MainWindow *app() const { return m_app; }

private:
    MainWindow *m_app;
};
