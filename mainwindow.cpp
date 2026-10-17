#include "mainwindow.h"

#include "basepage.h"
#include "serverapi.h"
#include "theme.h"
#include "homepages.h"
#include "loginwindow.h"
#include "bookingpages.h"
#include "ticketpages.h"
#include "luggagepages.h"
#include "staffpages.h"
#include "simulatorpages.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_current(nullptr), m_online(false), m_simulated(false), m_clockBusy(false), m_beltBusy(false)
{
    setWindowTitle(QStringLiteral("SkyPort - Airline & Airport Management"));
    resize(1320, 820);
    setMinimumSize(1100, 700);

    QSettings settings(QStringLiteral("SkyPort"), QStringLiteral("AirlineClient"));
    ServerApi::instance().setEndpoint(settings.value(QStringLiteral("server/host"), QStringLiteral("127.0.0.1")).toString(),
                                      static_cast<quint16>(settings.value(QStringLiteral("server/port"), 8080).toInt()));

    // ------------------------------------------------------------ chrome
    QWidget *central = new QWidget(this);
    QVBoxLayout *root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    QFrame *bar = new QFrame(central);
    bar->setObjectName(QStringLiteral("topBar"));
    QGridLayout *barLayout = new QGridLayout(bar);
    barLayout->setContentsMargins(18, 10, 18, 10);

    m_backBtn = Theme::button(QString::fromUtf8("\xE2\x86\x90  Back"), "nav", bar);              // ←  (top left)
    m_backBtn->setMinimumWidth(110);
    m_titleLabel = Theme::label(QString(), "h2", bar);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_clockLabel = Theme::label(QStringLiteral("Connecting..."), "chipWarn", bar);
    m_logoutBtn = Theme::button(QStringLiteral("Logout"), "danger", bar);                       // top right
    m_logoutBtn->setMinimumWidth(110);

    QWidget *rightBox = new QWidget(bar);
    QHBoxLayout *rightLayout = new QHBoxLayout(rightBox);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(12);
    rightLayout->addWidget(m_clockLabel);
    rightLayout->addWidget(m_logoutBtn);

    barLayout->addWidget(m_backBtn, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
    barLayout->addWidget(m_titleLabel, 0, 1);
    barLayout->addWidget(rightBox, 0, 2, Qt::AlignRight | Qt::AlignVCenter);
    barLayout->setColumnStretch(0, 1);
    barLayout->setColumnStretch(1, 2);
    barLayout->setColumnStretch(2, 1);

    m_stack = new QStackedWidget(central);
    root->addWidget(bar);
    root->addWidget(m_stack, 1);
    setCentralWidget(central);

    connect(m_backBtn, &QPushButton::clicked, this, [this]() { goBack(); });
    connect(m_logoutBtn, &QPushButton::clicked, this, [this]() { logout(); });

    // ------------------------------------------------------------ pages
    m_home = new HomePage(this);
    m_passengerLogin = new LoginPage(this, QStringLiteral("PASSENGER"));
    m_staffLogin = new LoginPage(this, QStringLiteral("STAFF"));
    m_adminLogin = new LoginPage(this, QStringLiteral("ADMIN"));
    m_passengerPortal = new PassengerPortalPage(this);
    m_bookPage = new BookTicketPage(this);
    m_detailsPage = new PassengerDetailsPage(this);
    m_boardingPage = new BoardingPassPage(this);
    m_trackPage = new TrackLuggagePage(this);
    m_checkInPage = new CheckInPage(this);
    m_beltStaffPage = new BeltStaffPage(this);
    m_adminPage = new AdminPage(this);
    m_simHome = new SimulatorHomePage(this);
    m_simTime = new SimTimePage(this);
    m_simPush = new SimPushPage(this);
    m_simLoss = new SimLossPage(this);

    BasePage *all[] = { m_home, m_passengerLogin, m_staffLogin, m_adminLogin, m_passengerPortal, m_bookPage,
                        m_detailsPage, m_boardingPage, m_trackPage, m_checkInPage, m_beltStaffPage, m_adminPage,
                        m_simHome, m_simTime, m_simPush, m_simLoss };
    for (unsigned i = 0; i < sizeof(all) / sizeof(all[0]); ++i) m_stack->addWidget(all[i]);

    showPage(m_home);

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::onTick);
    m_timer->start();
    requestClock();
}

MainWindow::~MainWindow() {}

void MainWindow::showPage(BasePage *page)
{
    m_current = page;
    m_stack->setCurrentWidget(page);
    m_titleLabel->setText(page->pageTitle());
    updateBars();
    page->onEnter();
}

void MainWindow::updateBars()
{
    const bool onHome = (m_current == m_home);
    m_backBtn->setVisible(!m_history.isEmpty() && !onHome);      // nothing to go back to on the first page
    m_logoutBtn->setVisible(!onHome);                            // logout always returns to the first page
}

void MainWindow::navigateTo(BasePage *page)
{
    if (!page || page == m_current) return;
    if (m_current) {
        m_current->onLeave();
        m_history.append(m_current);
    }
    showPage(page);
}

void MainWindow::navigateReplacing(BasePage *page, BasePage *keepHistoryUpTo)
{
    if (!page) return;
    while (!m_history.isEmpty() && m_history.last() != keepHistoryUpTo) m_history.removeLast();
    if (m_current) m_current->onLeave();
    showPage(page);
}

void MainWindow::goBack()
{
    if (m_history.isEmpty()) return;
    BasePage *prev = m_history.takeLast();
    if (m_current) m_current->onLeave();
    // coming back to a login screen (or the first page) means the user is signed out
    if (prev == m_adminLogin) {
        m_admin.clear();                                  // leaving the admin panel keeps the staff session
    } else if (qobject_cast<LoginPage *>(prev) || prev == m_home) {
        m_session.clear();
        m_admin.clear();
        m_alertedLost.clear();
        m_alertedRecovered.clear();
    }
    showPage(prev);
}

void MainWindow::logout()
{
    if (m_current) m_current->onLeave();
    m_session.clear();
    m_admin.clear();
    m_alertedLost.clear();
    m_alertedRecovered.clear();
    m_belt = BeltSnapshot();
    m_history.clear();
    showPage(m_home);
}

void MainWindow::onLoggedIn(const QString &role, const QString &user, const QString &password, const QString &staffType)
{
    if (role == QLatin1String("ADMIN")) {                 // the admin panel has its own credentials
        m_admin.role = role;
        m_admin.username = user;
        m_admin.password = password;
        navigateTo(m_adminPage);
        return;
    }
    m_session.role = role;
    m_session.username = user;
    m_session.password = password;
    m_session.staffType = staffType;
    m_alertedLost.clear();
    m_alertedRecovered.clear();

    if (role == QLatin1String("PASSENGER")) {
        navigateTo(m_passengerPortal);
    } else if (staffType == QLatin1String("BELT")) {
        navigateTo(m_beltStaffPage);
    } else {
        navigateTo(m_checkInPage);
    }
}
