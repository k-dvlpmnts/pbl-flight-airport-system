#pragma once
// First page and the passenger portal.
#include "basepage.h"
#include <QFrame>

class QLabel;
class QLineEdit;
class QSpinBox;
class QPushButton;
struct BeltSnapshot;

// A big rounded card with an icon, a title, some text and a button.
class OptionCard : public QFrame {
    Q_OBJECT
public:
    OptionCard(const QString &icon, const QString &title, const QString &text, const QString &buttonText,
               const QString &accentBg, const QString &accentFg, QWidget *parent = nullptr);
    QPushButton *button() const { return m_button; }

signals:
    void activated();

private:
    QPushButton *m_button;
};

class HomePage : public BasePage {
    Q_OBJECT
public:
    explicit HomePage(MainWindow *mw, QWidget *parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("SkyPort"); }
    void onEnter() override;
    void onPoll() override;

private:
    void applyEndpoint();
    void refreshStatus();

    QLineEdit *m_host;
    QSpinBox *m_port;
    QLabel *m_status;
};

class PassengerPortalPage : public BasePage {
    Q_OBJECT
public:
    explicit PassengerPortalPage(MainWindow *mw, QWidget *parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("Passenger portal"); }
    void onEnter() override;

private:
    void showBelt(const BeltSnapshot &snap);

    QLabel *m_welcome;
    QFrame *m_alertBox;
    QLabel *m_alertText;
};
