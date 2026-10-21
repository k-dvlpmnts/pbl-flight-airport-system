#include "homepages.h"

#include "mainwindow.h"
#include "beltdata.h"
#include "serverapi.h"
#include "theme.h"
#include "loginwindow.h"
#include "bookingpages.h"
#include "ticketpages.h"
#include "luggagepages.h"
#include "simulatorpages.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

// ---------------------------------------------------------------- OptionCard
OptionCard::OptionCard(const QString &icon, const QString &title, const QString &text, const QString &buttonText,
                       const QString &accentBg, const QString &accentFg, QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("card"));
    setMinimumWidth(270);
    setMaximumWidth(330);

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setContentsMargins(26, 26, 26, 24);
    l->setSpacing(8);

    QLabel *badge = new QLabel(icon, this);
    badge->setFixedSize(60, 60);
    badge->setAlignment(Qt::AlignCenter);
    badge->setStyleSheet(QStringLiteral("background: %1; color: %2; border-radius: 30px; font-size: 24pt;").arg(accentBg, accentFg));
    l->addWidget(badge);
    l->addSpacing(6);
    l->addWidget(Theme::label(title, "h2", this));
    QLabel *desc = Theme::label(text, "muted", this);
    desc->setWordWrap(true);
    l->addWidget(desc);
    l->addStretch(1);
    l->addSpacing(8);
    m_button = Theme::button(buttonText, "primary", this);
    m_button->setMinimumHeight(40);
    l->addWidget(m_button);
    connect(m_button, &QPushButton::clicked, this, &OptionCard::activated);
}

// ---------------------------------------------------------------- HomePage
HomePage::HomePage(MainWindow *mw, QWidget *parent) : BasePage(mw, parent)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(40, 24, 40, 24);
    root->addStretch(1);

    QLabel *logo = Theme::label(QString::fromUtf8("\xE2\x9C\x88  SkyPort"), "h1", this);
    logo->setAlignment(Qt::AlignCenter);
    root->addWidget(logo);
    QLabel *sub = Theme::label(QStringLiteral("Airline & Airport Management System"), "muted", this);
    sub->setAlignment(Qt::AlignCenter);
    root->addWidget(sub);
    root->addSpacing(26);

    QHBoxLayout *row = new QHBoxLayout();
    row->setSpacing(24);
    row->addStretch(1);

    OptionCard *passenger = new OptionCard(QString::fromUtf8("\xE2\x9C\x88"), QStringLiteral("Passenger login"),
        QStringLiteral("Book tickets, view boarding passes and track your luggage on the belt."),
        QStringLiteral("Passenger login"), QStringLiteral("#dbeafe"), QStringLiteral("#2563eb"), this);
    OptionCard *staff = new OptionCard(QString::fromUtf8("\xE2\x9A\x99"), QStringLiteral("Staff login"),
        QStringLiteral("Check-in counter and baggage belt staff. Administrators create staff accounts here."),
        QStringLiteral("Staff login"), QStringLiteral("#ccfbf1"), QStringLiteral("#0f766e"), this);
    OptionCard *sim = new OptionCard(QString::fromUtf8("\xE2\x96\xB6"), QStringLiteral("Simulator"),
        QStringLiteral("Set the system date and time, put luggage on the belt and simulate a bag loss."),
        QStringLiteral("Open simulator"), QStringLiteral("#fef3c7"), QStringLiteral("#b45309"), this);
    row->addWidget(passenger);
    row->addWidget(staff);
    row->addWidget(sim);
    row->addStretch(1);
    root->addLayout(row);
    root->addSpacing(28);

    connect(passenger, &OptionCard::activated, this, [this]() { app()->navigateTo(app()->passengerLogin()); });
    connect(staff, &OptionCard::activated, this, [this]() { app()->navigateTo(app()->staffLogin()); });
    connect(sim, &OptionCard::activated, this, [this]() { app()->navigateTo(app()->simHome()); });

    // ---- server connection
    QHBoxLayout *srvRow = new QHBoxLayout();
    srvRow->addStretch(1);
    QFrame *srv = Theme::card(this);
    srv->setFixedWidth(820);
    QHBoxLayout *s = new QHBoxLayout(srv);
    s->setContentsMargins(22, 14, 22, 14);
    s->setSpacing(10);
    s->addWidget(Theme::label(QStringLiteral("Server"), "h2", srv));
    m_host = new QLineEdit(srv);
    m_host->setPlaceholderText(QStringLiteral("IP address of the server laptop"));
    m_host->setMinimumWidth(200);
    s->addWidget(m_host, 1);
    s->addWidget(Theme::label(QStringLiteral("Port"), "muted", srv));
    m_port = new QSpinBox(srv);
    m_port->setRange(1, 65535);
    m_port->setValue(8080);
    s->addWidget(m_port);
    QPushButton *apply = Theme::button(QStringLiteral("Save and connect"), nullptr, srv);
    s->addWidget(apply);
    m_status = Theme::label(QStringLiteral("Connecting..."), "chipWarn", srv);
    s->addWidget(m_status);
    srvRow->addWidget(srv);
    srvRow->addStretch(1);
    root->addLayout(srvRow);
    root->addStretch(2);

    connect(apply, &QPushButton::clicked, this, [this]() { applyEndpoint(); });
    connect(m_host, &QLineEdit::returnPressed, this, [this]() { applyEndpoint(); });
}

void HomePage::onEnter()
{
    m_host->setText(ServerApi::instance().host());
    m_port->setValue(ServerApi::instance().port());
    refreshStatus();
}

void HomePage::onPoll()
{
    refreshStatus();
}

void HomePage::applyEndpoint()
{
    const QString host = m_host->text().trimmed();
    if (host.isEmpty()) return;
    app()->setServerEndpoint(host, static_cast<quint16>(m_port->value()));
    Theme::setRole(m_status, "chipWarn");
    m_status->setText(QStringLiteral("Connecting..."));
}

void HomePage::refreshStatus()
{
    const QString where = ServerApi::instance().host() + QLatin1Char(':') + QString::number(ServerApi::instance().port());
    if (app()->serverOnline()) {
        Theme::setRole(m_status, "chipOk");
        m_status->setText(QString::fromUtf8("\xE2\x97\x8F  Connected  ") + where);                // ●
    } else {
        Theme::setRole(m_status, "chipWarn");
        m_status->setText(QString::fromUtf8("\xE2\x97\x8F  Not connected  ") + where);
    }
}

// ---------------------------------------------------------------- PassengerPortalPage
PassengerPortalPage::PassengerPortalPage(MainWindow *mw, QWidget *parent) : BasePage(mw, parent)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(40, 24, 40, 24);
    root->addStretch(1);

    m_welcome = Theme::label(QStringLiteral("Welcome"), "h1", this);
    m_welcome->setAlignment(Qt::AlignCenter);
    root->addWidget(m_welcome);
    QLabel *sub = Theme::label(QStringLiteral("What would you like to do today?"), "muted", this);
    sub->setAlignment(Qt::AlignCenter);
    root->addWidget(sub);
    root->addSpacing(14);

    QHBoxLayout *alertRow = new QHBoxLayout();
    alertRow->addStretch(1);
    m_alertBox = new QFrame(this);
    m_alertBox->setObjectName(QStringLiteral("alertBox"));
    m_alertBox->setFixedWidth(860);
    QHBoxLayout *ab = new QHBoxLayout(m_alertBox);
    ab->setContentsMargins(18, 12, 18, 12);
    m_alertText = Theme::label(QString(), "error", m_alertBox);
    m_alertText->setWordWrap(true);
    ab->addWidget(m_alertText);
    m_alertBox->hide();
    alertRow->addWidget(m_alertBox);
    alertRow->addStretch(1);
    root->addLayout(alertRow);
    root->addSpacing(12);

    QHBoxLayout *row = new QHBoxLayout();
    row->setSpacing(24);
    row->addStretch(1);
    OptionCard *book = new OptionCard(QString::fromUtf8("\xE2\x9C\x88"), QStringLiteral("Book ticket"),
        QStringLiteral("Pick From, To and a date, choose your seats and enter the passenger details."),
        QStringLiteral("Book ticket"), QStringLiteral("#dbeafe"), QStringLiteral("#2563eb"), this);
    OptionCard *pass = new OptionCard(QString::fromUtf8("\xE2\x98\xB0"), QStringLiteral("Boarding pass"),
        QStringLiteral("See your tickets and boarding passes with the PNR and check-in code."),
        QStringLiteral("My boarding passes"), QStringLiteral("#ede9fe"), QStringLiteral("#6d28d9"), this);
    OptionCard *track = new OptionCard(QString::fromUtf8("\xE2\x97\x8E"), QStringLiteral("Track luggage"),
        QStringLiteral("Follow your bag along the baggage belt, live. You are alerted if it falls off."),
        QStringLiteral("Track luggage"), QStringLiteral("#ccfbf1"), QStringLiteral("#0f766e"), this);
    row->addWidget(book);
    row->addWidget(pass);
    row->addWidget(track);
    row->addStretch(1);
    root->addLayout(row);
    root->addStretch(2);

    connect(book, &OptionCard::activated, this, [this]() { app()->navigateTo(app()->bookPage()); });
    connect(pass, &OptionCard::activated, this, [this]() { app()->navigateTo(app()->boardingPage()); });
    connect(track, &OptionCard::activated, this, [this]() { app()->navigateTo(app()->trackPage()); });
    connect(mw, &MainWindow::beltUpdated, this, [this](const BeltSnapshot &snap) { showBelt(snap); });
}

void PassengerPortalPage::onEnter()
{
    m_welcome->setText(QStringLiteral("Welcome, ") + app()->session().username);
    showBelt(app()->belt());
}

void PassengerPortalPage::showBelt(const BeltSnapshot &snap)
{
    QStringList lost;
    for (int i = 0; i < snap.bags.size(); ++i) {
        const BagItem &b = snap.bags.at(i);
        if (b.isLost())
            lost.append(QStringLiteral("%1 (tag %2) - missed by %3").arg(b.name, b.tag, snap.sensorName(b.lostAt)));
    }
    if (lost.isEmpty()) {
        m_alertBox->hide();
        return;
    }
    m_alertText->setText(QString::fromUtf8("\xE2\x9A\xA0  Luggage alert: ") + lost.join(QStringLiteral("; "))
                         + QStringLiteral(".  Open \"Track luggage\" for details."));
    m_alertBox->show();
}
