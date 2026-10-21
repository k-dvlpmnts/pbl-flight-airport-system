#include "staffpages.h"

#include "mainwindow.h"
#include "serverapi.h"
#include "theme.h"
#include "loginwindow.h"

#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QDateEdit>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

QTableWidget *makeTable(QWidget *parent, const QStringList &headers)
{
    QTableWidget *t = new QTableWidget(parent);
    t->setColumnCount(headers.size());
    t->setHorizontalHeaderLabels(headers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setAlternatingRowColors(true);
    t->setShowGrid(false);
    t->verticalHeader()->setVisible(false);
    t->verticalHeader()->setDefaultSectionSize(34);
    t->horizontalHeader()->setStretchLastSection(true);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    return t;
}

void setCell(QTableWidget *t, int row, int col, const QString &text, const char *color = nullptr)
{
    QTableWidgetItem *item = new QTableWidgetItem(text);
    if (color) item->setForeground(QBrush(QColor(QString::fromLatin1(color))));
    t->setItem(row, col, item);
}

QVBoxLayout *scrollRoot(QWidget *page)
{
    QVBoxLayout *outer = new QVBoxLayout(page);
    outer->setContentsMargins(0, 0, 0, 0);
    QScrollArea *scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget *content = new QWidget();
    scroll->setWidget(content);
    Theme::transparentScroll(scroll);
    outer->addWidget(scroll);
    return new QVBoxLayout(content);
}

} // namespace

// =====================================================================================
//  CheckInPage
// =====================================================================================
CheckInPage::CheckInPage(MainWindow *mw, QWidget *parent)
    : BasePage(mw, parent), m_open(false), m_dateInit(false), m_pollCount(0)
{
    QVBoxLayout *root = scrollRoot(this);
    root->setContentsMargins(28, 14, 28, 18);
    root->setSpacing(12);

    // ------------------------------------------------------------ header row
    QHBoxLayout *top = new QHBoxLayout();
    top->setSpacing(10);
    top->addWidget(Theme::label(QStringLiteral("Check-in counter"), "h2", this));
    top->addStretch(1);
    top->addWidget(Theme::label(QStringLiteral("Flight date"), "muted", this));
    m_date = new QDateEdit(this);
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat(QStringLiteral("dd MMM yyyy"));
    m_date->setMinimumWidth(150);
    top->addWidget(m_date);
    QPushButton *admin = Theme::button(QStringLiteral("Admin"), "ghost", this);
    top->addWidget(admin);
    root->addLayout(top);
    connect(admin, &QPushButton::clicked, this, [this]() { app()->navigateTo(app()->adminLogin()); });

    QFrame *bannerBox = new QFrame(this);
    bannerBox->setObjectName(QStringLiteral("infoBox"));
    QHBoxLayout *bb = new QHBoxLayout(bannerBox);
    bb->setContentsMargins(18, 10, 18, 10);
    m_banner = new QLabel(bannerBox);
    m_banner->setWordWrap(true);
    m_banner->setText(QStringLiteral("Passengers can be checked in strictly up to 3 hours before their flight departs. "
                                     "Select a flight, pick the passenger and type the check-in code from the boarding pass."));
    bb->addWidget(m_banner);
    root->addWidget(bannerBox);

    // ------------------------------------------------------------ flights
    root->addWidget(Theme::label(QStringLiteral("1.  Flights on this date"), "h2", this));
    m_flights = makeTable(this, QStringList() << QStringLiteral("Flight") << QStringLiteral("Route") << QStringLiteral("Departs")
                                              << QStringLiteral("Booked") << QStringLiteral("Checked in") << QStringLiteral("Check-in"));
    m_flights->setMinimumHeight(190);
    m_flights->setMaximumHeight(230);
    root->addWidget(m_flights);

    // ------------------------------------------------------------ passengers + form
    QHBoxLayout *bottom = new QHBoxLayout();
    bottom->setSpacing(14);

    QVBoxLayout *left = new QVBoxLayout();
    QHBoxLayout *lh = new QHBoxLayout();
    lh->addWidget(Theme::label(QStringLiteral("2.  Passengers of the flight"), "h2", this));
    lh->addStretch(1);
    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText(QStringLiteral("Search PNR, name or seat"));
    m_filter->setMinimumWidth(220);
    lh->addWidget(m_filter);
    left->addLayout(lh);
    m_pax = makeTable(this, QStringList() << QStringLiteral("PNR") << QStringLiteral("Seat") << QStringLiteral("Passenger")
                                          << QStringLiteral("Age") << QStringLiteral("Gender") << QStringLiteral("Status"));
    m_pax->setMinimumHeight(250);
    left->addWidget(m_pax, 1);
    bottom->addLayout(left, 6);

    QFrame *formCard = Theme::card(this);
    QVBoxLayout *fl = new QVBoxLayout(formCard);
    fl->setContentsMargins(22, 18, 22, 18);
    fl->setSpacing(8);
    fl->addWidget(Theme::label(QStringLiteral("3.  Check in"), "h2", formCard));
    m_selected = new QLabel(formCard);
    m_selected->setTextFormat(Qt::RichText);
    m_selected->setWordWrap(true);
    fl->addWidget(m_selected);
    fl->addSpacing(6);
    fl->addWidget(Theme::label(QStringLiteral("CHECK-IN CODE FROM THE BOARDING PASS"), "caption", formCard));
    m_code = new QLineEdit(formCard);
    m_code->setPlaceholderText(QStringLiteral("6 digit code"));
    m_code->setMaxLength(12);
    fl->addWidget(m_code);
    m_msg = Theme::label(QString(), "error", formCard);
    m_msg->setWordWrap(true);
    m_msg->hide();
    fl->addWidget(m_msg);
    fl->addStretch(1);
    m_checkBtn = Theme::button(QString::fromUtf8("\xE2\x9C\x93  Check in passenger"), "primary", formCard);
    m_checkBtn->setMinimumHeight(44);
    fl->addWidget(m_checkBtn);
    bottom->addWidget(formCard, 4);
    root->addLayout(bottom, 1);

    // ------------------------------------------------------------ wiring
    connect(m_date, &QDateEdit::dateChanged, this, [this]() {
        m_flightId.clear();
        m_selPnr.clear();
        m_selSeat.clear();
        m_paxRows.clear();
        m_flightSig.clear();
        m_paxSig.clear();
        m_pax->setRowCount(0);
        loadFlights();
        updateForm();
    });
    connect(m_flights, &QTableWidget::itemSelectionChanged, this, [this]() {
        const int row = m_flights->currentRow();
        if (row < 0 || !m_flights->item(row, 0)) return;
        const QString id = m_flights->item(row, 0)->text();
        if (id == m_flightId) return;
        m_flightId = id;
        m_selPnr.clear();
        m_selSeat.clear();
        m_paxSig.clear();
        m_msg->hide();
        loadPassengers();
    });
    connect(m_pax, &QTableWidget::itemSelectionChanged, this, [this]() {
        const int row = m_pax->currentRow();
        if (row >= 0 && m_pax->item(row, 0) && m_pax->item(row, 1)) {
            m_selPnr = m_pax->item(row, 0)->text();
            m_selSeat = m_pax->item(row, 1)->text();
        }
        m_msg->hide();
        updateForm();
    });
    connect(m_filter, &QLineEdit::textChanged, this, [this]() { fillPassengers(); });
    connect(m_checkBtn, &QPushButton::clicked, this, &CheckInPage::doCheckIn);
    connect(m_code, &QLineEdit::returnPressed, this, &CheckInPage::doCheckIn);
    updateForm();
}

QString CheckInPage::isoDate() const
{
    return m_date->date().toString(Qt::ISODate);
}

void CheckInPage::onEnter()
{
    const QDate today = app()->serverDate();
    m_date->setDateRange(today.addDays(-1), today.addDays(31));
    if (!m_dateInit) {
        m_date->setDate(today);                      // also triggers loadFlights()
        m_dateInit = true;
    }
    m_flightSig.clear();
    m_paxSig.clear();
    m_pollCount = 0;
    loadFlights();
    if (!m_flightId.isEmpty()) loadPassengers();
}

void CheckInPage::onPoll()
{
    const QDate today = app()->serverDate();
    if (m_date->minimumDate() != today.addDays(-1)) m_date->setDateRange(today.addDays(-1), today.addDays(31));
    if (++m_pollCount % 2 == 0) {
        loadFlights();
        if (!m_flightId.isEmpty()) loadPassengers();
    }
}

void CheckInPage::loadFlights()
{
    const QString date = isoDate();
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("CI_FLIGHTS"), date});
    connect(r, &PendingReply::finished, this, [this, date](const Reply &rep) {
        if (!rep.ok || date != isoDate()) return;
        fillFlights(rep.rowsOf(QStringLiteral("F")));
    });
}

void CheckInPage::fillFlights(const QVector<QStringList> &rows)
{
    QString sig;
    for (int i = 0; i < rows.size(); ++i) sig += rows.at(i).join(QLatin1Char('|')) + QLatin1Char('\n');
    if (sig == m_flightSig) return;
    m_flightSig = sig;

    m_flights->blockSignals(true);
    m_flights->setRowCount(rows.size());
    int select = -1;
    for (int i = 0; i < rows.size(); ++i) {
        const QStringList &f = rows.at(i);
        const bool open = (fld(f, 7) == QLatin1String("1"));
        setCell(m_flights, i, 0, fld(f, 1));
        setCell(m_flights, i, 1, fld(f, 2) + QString::fromUtf8(" \xE2\x86\x92 ") + fld(f, 3));
        setCell(m_flights, i, 2, fld(f, 4));
        setCell(m_flights, i, 3, fld(f, 5));
        setCell(m_flights, i, 4, fld(f, 6));
        setCell(m_flights, i, 5, open ? (QStringLiteral("Open until ") + fld(f, 8).mid(11)) : QStringLiteral("Closed"),
                open ? "#15803d" : "#b91c1c");
        if (fld(f, 1) == m_flightId) select = i;
    }
    if (select >= 0) m_flights->selectRow(select);
    m_flights->blockSignals(false);
}

void CheckInPage::loadPassengers()
{
    if (m_flightId.isEmpty()) return;
    const QString id = m_flightId;
    const QString date = isoDate();
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("CI_LIST"), id, date});
    connect(r, &PendingReply::finished, this, [this, id, date](const Reply &rep) {
        if (id != m_flightId || date != isoDate()) return;
        if (!rep.ok) return;
        m_open = (rep.headField(1) == QLatin1String("1"));
        m_closes = rep.headField(2);
        m_paxRows = rep.rowsOf(QStringLiteral("P"));
        fillPassengers();
    });
}

void CheckInPage::fillPassengers()
{
    const QString needle = m_filter->text().trimmed().toLower();
    QVector<QStringList> shown;
    for (int i = 0; i < m_paxRows.size(); ++i) {
        const QStringList &p = m_paxRows.at(i);
        if (!needle.isEmpty() && !(fld(p, 1).toLower().contains(needle) || fld(p, 2).toLower().contains(needle)
                                   || fld(p, 3).toLower().contains(needle)))
            continue;
        shown.append(p);
    }

    QString sig = (m_open ? QStringLiteral("1") : QStringLiteral("0")) + m_selPnr + m_selSeat + needle;
    for (int i = 0; i < shown.size(); ++i) sig += shown.at(i).join(QLatin1Char('|')) + QLatin1Char('\n');
    if (sig == m_paxSig) return;
    m_paxSig = sig;

    m_pax->blockSignals(true);
    m_pax->setRowCount(shown.size());
    int select = -1;
    for (int i = 0; i < shown.size(); ++i) {
        const QStringList &p = shown.at(i);
        const bool checked = (fld(p, 6) == QLatin1String("1"));
        setCell(m_pax, i, 0, fld(p, 1));
        setCell(m_pax, i, 1, fld(p, 2));
        setCell(m_pax, i, 2, fld(p, 3));
        setCell(m_pax, i, 3, fld(p, 4));
        setCell(m_pax, i, 4, fld(p, 5));
        setCell(m_pax, i, 5, checked ? (QString::fromUtf8("\xE2\x9C\x93  Checked in  ") + fld(p, 7).mid(11)) : QStringLiteral("Not checked in"),
                checked ? "#15803d" : "#b45309");
        if (fld(p, 1) == m_selPnr && fld(p, 2) == m_selSeat) select = i;
    }
    if (select >= 0) m_pax->selectRow(select);
    m_pax->blockSignals(false);
    updateForm();
}

void CheckInPage::updateForm()
{
    const QStringList *sel = nullptr;
    for (int i = 0; i < m_paxRows.size(); ++i)
        if (fld(m_paxRows.at(i), 1) == m_selPnr && fld(m_paxRows.at(i), 2) == m_selSeat) sel = &m_paxRows.at(i);

    if (m_flightId.isEmpty()) {
        m_selected->setText(QStringLiteral("<span style='color:#64748b'>Select a flight first.</span>"));
        m_checkBtn->setEnabled(false);
        return;
    }
    if (!sel) {
        m_selected->setText(QStringLiteral("<span style='color:#64748b'>Select a passenger from the list.</span>"));
        m_checkBtn->setEnabled(false);
        return;
    }
    const bool checked = (fld(*sel, 6) == QLatin1String("1"));
    QString text = QStringLiteral("<b style='font-size:12pt'>%1</b><br>Seat %2 &nbsp;|&nbsp; PNR %3 &nbsp;|&nbsp; age %4<br>Bag: %5")
                       .arg(fld(*sel, 3), fld(*sel, 2), fld(*sel, 1), fld(*sel, 4),
                            fld(*sel, 8).toInt() > 0 ? fld(*sel, 8) + QStringLiteral(" kg") : QStringLiteral("none"));
    if (checked) text += QStringLiteral("<br><span style='color:#15803d'><b>Already checked in.</b></span>");
    else if (!m_open) text += QStringLiteral("<br><span style='color:#b91c1c'><b>Check-in closed at ") + m_closes
                              + QStringLiteral(" (3 hours before departure).</b></span>");
    m_selected->setText(text);
    m_checkBtn->setEnabled(!checked && m_open);
}

void CheckInPage::doCheckIn()
{
    if (m_flightId.isEmpty() || m_selPnr.isEmpty()) return;
    const QString code = m_code->text().trimmed();
    if (code.isEmpty()) {
        Theme::setRole(m_msg, "error");
        m_msg->setText(QStringLiteral("Type the check-in code printed on the passenger's boarding pass."));
        m_msg->show();
        return;
    }
    m_checkBtn->setEnabled(false);
    const QString pnr = m_selPnr, seat = m_selSeat;
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("CI_DO"), app()->session().username, m_flightId,
                                                              isoDate(), pnr, seat, code});
    connect(r, &PendingReply::finished, this, [this](const Reply &rep) {
        if (!rep.ok) {
            Theme::setRole(m_msg, "error");
            m_msg->setText(rep.netOk ? rep.message : QStringLiteral("Cannot reach the server."));
            m_msg->show();
            updateForm();
            return;
        }
        Theme::setRole(m_msg, "ok");
        m_msg->setText(QString::fromUtf8("\xE2\x9C\x93  ") + rep.headField(1) + QStringLiteral(" (seat ") + rep.headField(2)
                       + QStringLiteral(") is checked in."));
        m_msg->show();
        m_code->clear();
        m_paxSig.clear();
        m_flightSig.clear();
        loadPassengers();
        loadFlights();
    });
}

// =====================================================================================
//  AdminPage
// =====================================================================================
AdminPage::AdminPage(MainWindow *mw, QWidget *parent) : BasePage(mw, parent)
{
    QHBoxLayout *root = new QHBoxLayout(this);
    root->setContentsMargins(40, 24, 40, 24);
    root->setSpacing(20);

    QFrame *form = Theme::card(this);
    form->setMaximumWidth(430);
    QVBoxLayout *fl = new QVBoxLayout(form);
    fl->setContentsMargins(28, 24, 28, 24);
    fl->setSpacing(8);
    fl->addWidget(Theme::label(QStringLiteral("Create staff account"), "h2", form));
    QLabel *sub = Theme::label(QStringLiteral("New accounts are written to staff_users.dat and can log in immediately."), "muted", form);
    sub->setWordWrap(true);
    fl->addWidget(sub);
    fl->addSpacing(10);

    fl->addWidget(Theme::label(QStringLiteral("USERNAME"), "caption", form));
    m_user = new QLineEdit(form);
    m_user->setPlaceholderText(QStringLiteral("e.g. checkin2"));
    fl->addWidget(m_user);
    fl->addWidget(Theme::label(QStringLiteral("PASSWORD"), "caption", form));
    m_pass = new QLineEdit(form);
    m_pass->setPlaceholderText(QStringLiteral("at least 3 characters"));
    fl->addWidget(m_pass);
    fl->addWidget(Theme::label(QStringLiteral("STAFF TYPE"), "caption", form));
    m_type = new QComboBox(form);
    m_type->addItem(QStringLiteral("Check-in staff"), QStringLiteral("CHECKIN"));
    m_type->addItem(QStringLiteral("Belt staff"), QStringLiteral("BELT"));
    fl->addWidget(m_type);
    m_msg = Theme::label(QString(), "error", form);
    m_msg->setWordWrap(true);
    m_msg->hide();
    fl->addWidget(m_msg);
    fl->addStretch(1);
    QPushButton *create = Theme::button(QStringLiteral("Create account"), "primary", form);
    create->setMinimumHeight(42);
    fl->addWidget(create);
    root->addWidget(form);

    QVBoxLayout *right = new QVBoxLayout();
    right->addWidget(Theme::label(QStringLiteral("Existing staff accounts"), "h2", this));
    m_table = makeTable(this, QStringList() << QStringLiteral("Username") << QStringLiteral("Staff type"));
    right->addWidget(m_table, 1);
    root->addLayout(right, 1);

    connect(create, &QPushButton::clicked, this, &AdminPage::createStaff);
    connect(m_pass, &QLineEdit::returnPressed, this, &AdminPage::createStaff);
}

void AdminPage::onEnter()
{
    m_msg->hide();
    loadStaff();
}

void AdminPage::loadStaff()
{
    const Session &a = app()->adminSession();
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("ADMIN_LISTSTAFF"), a.username, a.password});
    connect(r, &PendingReply::finished, this, [this](const Reply &rep) {
        if (!rep.ok) return;
        const QVector<QStringList> rows = rep.rowsOf(QStringLiteral("S"));
        m_table->setRowCount(rows.size());
        for (int i = 0; i < rows.size(); ++i) {
            setCell(m_table, i, 0, fld(rows.at(i), 1));
            setCell(m_table, i, 1, fld(rows.at(i), 2) == QLatin1String("BELT") ? QStringLiteral("Belt staff") : QStringLiteral("Check-in staff"));
        }
    });
}

void AdminPage::createStaff()
{
    const QString user = m_user->text().trimmed();
    const QString pass = m_pass->text();
    if (user.isEmpty() || pass.isEmpty()) {
        Theme::setRole(m_msg, "error");
        m_msg->setText(QStringLiteral("Enter a username and a password."));
        m_msg->show();
        return;
    }
    const Session &a = app()->adminSession();
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("ADMIN_ADDSTAFF"), a.username, a.password, user, pass,
                                                              m_type->currentData().toString()});
    connect(r, &PendingReply::finished, this, [this, user](const Reply &rep) {
        if (!rep.ok) {
            Theme::setRole(m_msg, "error");
            m_msg->setText(rep.netOk ? rep.message : QStringLiteral("Cannot reach the server."));
            m_msg->show();
            return;
        }
        Theme::setRole(m_msg, "ok");
        m_msg->setText(QStringLiteral("Staff account \"%1\" created.").arg(user));
        m_msg->show();
        m_user->clear();
        m_pass->clear();
        loadStaff();
    });
}
