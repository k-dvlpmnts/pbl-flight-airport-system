#include "simulatorpages.h"

#include "mainwindow.h"
#include "serverapi.h"
#include "theme.h"
#include "homepages.h"

#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
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

// "yyyy-MM-dd HH:mm:ss" without depending on the locale
QString isoText(const QDateTime &dt)
{
    return QStringLiteral("%1-%2-%3 %4:%5:%6")
        .arg(dt.date().year(), 4, 10, QLatin1Char('0'))
        .arg(dt.date().month(), 2, 10, QLatin1Char('0'))
        .arg(dt.date().day(), 2, 10, QLatin1Char('0'))
        .arg(dt.time().hour(), 2, 10, QLatin1Char('0'))
        .arg(dt.time().minute(), 2, 10, QLatin1Char('0'))
        .arg(dt.time().second(), 2, 10, QLatin1Char('0'));
}

QString statusText(const QString &s)
{
    if (s == QLatin1String("NONE")) return QStringLiteral("At check-in - not on the belt");
    if (s == QLatin1String("ONBELT")) return QStringLiteral("On the belt");
    if (s == QLatin1String("DELIVERED")) return QStringLiteral("Delivered");
    if (s == QLatin1String("LOST")) return QStringLiteral("LOST");
    if (s == QLatin1String("RECOVERED")) return QStringLiteral("Recovered");
    return QStringLiteral("No bag");
}

const char *statusColor(const QString &s)
{
    if (s == QLatin1String("NONE")) return "#b45309";
    if (s == QLatin1String("ONBELT")) return "#1d4ed8";
    if (s == QLatin1String("DELIVERED")) return "#15803d";
    if (s == QLatin1String("LOST")) return "#b91c1c";
    if (s == QLatin1String("RECOVERED")) return "#b45309";
    return "#64748b";
}

} // namespace

// =====================================================================================
//  SimFlightList
// =====================================================================================
SimFlightList::SimFlightList(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(6);
    l->addWidget(Theme::label(QStringLiteral("Flights departing within 3 hours of the system time"), "h2", this));
    m_table = makeTable(this, QStringList() << QStringLiteral("Flight") << QStringLiteral("Route") << QStringLiteral("Date")
                                            << QStringLiteral("Departs") << QStringLiteral("Checked in")
                                            << QStringLiteral("Bags waiting") << QStringLiteral("On belt"));
    m_table->setMinimumHeight(150);
    l->addWidget(m_table, 1);
    m_info = Theme::label(QString(), "muted", this);
    m_info->setWordWrap(true);
    l->addWidget(m_info);

    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]() {
        const int row = m_table->currentRow();
        if (row < 0 || !m_table->item(row, 0) || !m_table->item(row, 2)) return;
        const QString f = m_table->item(row, 0)->text();
        const QString d = m_table->item(row, 2)->text();
        if (f == m_flight && d == m_date) return;
        m_flight = f;
        m_date = d;
        emit selectionChanged();
    });
}

void SimFlightList::forgetSelection()
{
    m_flight.clear();
    m_date.clear();
    m_signature.clear();
    m_table->clearSelection();
}

void SimFlightList::refresh()
{
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("SIM_FLIGHTS")});
    connect(r, &PendingReply::finished, this, [this](const Reply &rep) {
        if (!rep.ok) return;
        fill(rep.rowsOf(QStringLiteral("F")));
    });
}

void SimFlightList::fill(const QVector<QStringList> &rows)
{
    QString sig;
    for (int i = 0; i < rows.size(); ++i) sig += rows.at(i).join(QLatin1Char('|')) + QLatin1Char('\n');
    if (sig == m_signature) return;
    m_signature = sig;

    m_table->blockSignals(true);
    m_table->setRowCount(rows.size());
    int select = -1;
    for (int i = 0; i < rows.size(); ++i) {
        const QStringList &f = rows.at(i);
        // F|flight|origin|dest|date|dep|booked|checkedIn|pending|onBelt
        setCell(m_table, i, 0, fld(f, 1));
        setCell(m_table, i, 1, fld(f, 2) + QString::fromUtf8(" \xE2\x86\x92 ") + fld(f, 3));
        setCell(m_table, i, 2, fld(f, 4));
        setCell(m_table, i, 3, fld(f, 5));
        setCell(m_table, i, 4, fld(f, 7) + QStringLiteral(" / ") + fld(f, 6));
        setCell(m_table, i, 5, fld(f, 8));
        setCell(m_table, i, 6, fld(f, 9));
        if (fld(f, 1) == m_flight && fld(f, 4) == m_date) select = i;
    }
    if (select >= 0) m_table->selectRow(select);
    m_table->blockSignals(false);

    m_info->setText(rows.isEmpty()
        ? QStringLiteral("No flight departs within the next 3 hours of the system time. Change the time on the "
                         "\"Date and time\" screen (for example 2 hours before a flight).")
        : QString());
    if (select < 0 && !m_flight.isEmpty()) {            // the selected flight left the window
        m_flight.clear();
        m_date.clear();
        emit selectionChanged();
    }
}

// =====================================================================================
//  SimulatorHomePage
// =====================================================================================
SimulatorHomePage::SimulatorHomePage(MainWindow *mw, QWidget *parent) : BasePage(mw, parent)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(40, 24, 40, 24);
    root->addStretch(1);

    QLabel *title = Theme::label(QStringLiteral("Simulator"), "h1", this);
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);
    m_clock = Theme::label(QString(), "chip", this);
    root->addWidget(m_clock, 0, Qt::AlignHCenter);
    root->addSpacing(22);

    QHBoxLayout *row = new QHBoxLayout();
    row->setSpacing(24);
    row->addStretch(1);
    OptionCard *time = new OptionCard(QString::fromUtf8("\xE2\x97\xB7"), QStringLiteral("Date and time"),
        QStringLiteral("Move the whole system (booking window, check-in cut-off, belt) to any date and time you like."),
        QStringLiteral("Set date and time"), QStringLiteral("#dbeafe"), QStringLiteral("#2563eb"), this);
    OptionCard *push = new OptionCard(QString::fromUtf8("\xE2\x96\xB6"), QStringLiteral("Put luggage on the belt"),
        QStringLiteral("Pick a flight departing within 3 hours and a checked-in passenger; the bag crosses the start RFID."),
        QStringLiteral("Put luggage on belt"), QStringLiteral("#ccfbf1"), QStringLiteral("#0f766e"), this);
    OptionCard *loss = new OptionCard(QString::fromUtf8("\xE2\x9A\xA0"), QStringLiteral("Bag loss"),
        QStringLiteral("Choose a bag that is on the belt and the sensor that will not detect it. Both portals show the loss."),
        QStringLiteral("Simulate bag loss"), QStringLiteral("#fee2e2"), QStringLiteral("#b91c1c"), this);
    row->addWidget(time);
    row->addWidget(push);
    row->addWidget(loss);
    row->addStretch(1);
    root->addLayout(row);
    root->addStretch(2);

    connect(time, &OptionCard::activated, this, [this]() { app()->navigateTo(app()->simTime()); });
    connect(push, &OptionCard::activated, this, [this]() { app()->navigateTo(app()->simPush()); });
    connect(loss, &OptionCard::activated, this, [this]() { app()->navigateTo(app()->simLoss()); });
}

void SimulatorHomePage::onEnter()
{
    onPoll();
}

void SimulatorHomePage::onPoll()
{
    m_clock->setText(QStringLiteral("System time  ") + isoText(app()->serverNow()) + (app()->serverSimulated() ? QStringLiteral("   (simulated)") : QString()));
}

// =====================================================================================
//  SimTimePage
// =====================================================================================
SimTimePage::SimTimePage(MainWindow *mw, QWidget *parent) : BasePage(mw, parent)
{
    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(30, 24, 30, 24);
    outer->addStretch(1);
    QHBoxLayout *mid = new QHBoxLayout();
    mid->addStretch(1);

    QFrame *card = Theme::card(this);
    card->setFixedWidth(640);
    QVBoxLayout *l = new QVBoxLayout(card);
    l->setContentsMargins(34, 28, 34, 28);
    l->setSpacing(10);
    l->addWidget(Theme::label(QStringLiteral("System date and time"), "h2", card));
    QLabel *info = Theme::label(QStringLiteral("Everything uses this clock: the one-month booking window, the 3-hour check-in cut-off, "
                                               "the 3-hour belt window and the belt sensors' time stamps. All laptops follow it at once."), "muted", card);
    info->setWordWrap(true);
    l->addWidget(info);
    l->addSpacing(6);

    m_now = Theme::label(QString(), "chip", card);
    l->addWidget(m_now, 0, Qt::AlignLeft);
    l->addSpacing(6);

    l->addWidget(Theme::label(QStringLiteral("NEW SYSTEM DATE AND TIME"), "caption", card));
    m_edit = new QDateTimeEdit(card);
    m_edit->setCalendarPopup(true);
    m_edit->setDisplayFormat(QStringLiteral("yyyy-MM-dd  HH:mm:ss"));
    m_edit->setMinimumHeight(40);
    l->addWidget(m_edit);

    QHBoxLayout *quick = new QHBoxLayout();
    quick->setSpacing(8);
    const char *labels[] = {"-1 day", "-1 hour", "-10 min", "+10 min", "+1 hour", "+1 day"};
    const qint64 deltas[] = {-86400, -3600, -600, 600, 3600, 86400};
    for (int i = 0; i < 6; ++i) {
        QPushButton *b = Theme::button(QString::fromLatin1(labels[i]), "nav", card);
        const qint64 d = deltas[i];
        connect(b, &QPushButton::clicked, this, [this, d]() { shift(d); });
        quick->addWidget(b);
    }
    l->addLayout(quick);
    l->addSpacing(8);

    QHBoxLayout *btns = new QHBoxLayout();
    QPushButton *apply = Theme::button(QStringLiteral("Apply simulated time"), "primary", card);
    apply->setMinimumHeight(42);
    QPushButton *reset = Theme::button(QStringLiteral("Back to real time"), nullptr, card);
    reset->setMinimumHeight(42);
    btns->addWidget(apply, 1);
    btns->addWidget(reset);
    l->addLayout(btns);

    m_msg = Theme::label(QString(), "ok", card);
    m_msg->setWordWrap(true);
    m_msg->hide();
    l->addWidget(m_msg);

    mid->addWidget(card);
    mid->addStretch(1);
    outer->addLayout(mid);
    outer->addStretch(2);

    connect(apply, &QPushButton::clicked, this, &SimTimePage::apply);
    connect(reset, &QPushButton::clicked, this, &SimTimePage::resetToRealTime);
}

void SimTimePage::onEnter()
{
    m_msg->hide();
    m_edit->setDateTime(app()->serverNow());
    onPoll();
}

void SimTimePage::onPoll()
{
    m_now->setText(QStringLiteral("Current system time  ") + isoText(app()->serverNow())
                   + (app()->serverSimulated() ? QStringLiteral("   (simulated)") : QStringLiteral("   (real time)")));
}

void SimTimePage::shift(qint64 seconds)
{
    m_edit->setDateTime(m_edit->dateTime().addSecs(seconds));
}

void SimTimePage::apply()
{
    const QString text = isoText(m_edit->dateTime());
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("SIM_SETTIME"), text});
    connect(r, &PendingReply::finished, this, [this, text](const Reply &rep) {
        if (!rep.ok) {
            Theme::setRole(m_msg, "error");
            m_msg->setText(rep.netOk ? rep.message : QStringLiteral("Cannot reach the server."));
        } else {
            Theme::setRole(m_msg, "ok");
            m_msg->setText(QStringLiteral("System time is now %1 on every connected laptop.").arg(text));
        }
        m_msg->show();
    });
}

void SimTimePage::resetToRealTime()
{
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("SIM_RESET")});
    connect(r, &PendingReply::finished, this, [this](const Reply &rep) {
        if (!rep.ok) {
            Theme::setRole(m_msg, "error");
            m_msg->setText(rep.netOk ? rep.message : QStringLiteral("Cannot reach the server."));
        } else {
            Theme::setRole(m_msg, "ok");
            m_msg->setText(QStringLiteral("The system follows the real clock again."));
            m_edit->setDateTime(QDateTime::currentDateTime());
        }
        m_msg->show();
    });
}

// =====================================================================================
//  SimPushPage
// =====================================================================================
SimPushPage::SimPushPage(MainWindow *mw, QWidget *parent) : BasePage(mw, parent), m_pollCount(0)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(28, 14, 28, 18);
    root->setSpacing(10);

    m_clock = Theme::label(QString(), "chip", this);
    root->addWidget(m_clock, 0, Qt::AlignLeft);

    m_flights = new SimFlightList(this);
    root->addWidget(m_flights, 2);

    root->addWidget(Theme::label(QStringLiteral("Checked-in passengers of the selected flight"), "h2", this));
    m_pax = makeTable(this, QStringList() << QStringLiteral("Passenger") << QStringLiteral("Seat") << QStringLiteral("PNR")
                                          << QStringLiteral("Bag tag") << QStringLiteral("Kg") << QStringLiteral("Bag status"));
    m_pax->setMinimumHeight(130);
    root->addWidget(m_pax, 2);

    QHBoxLayout *low = new QHBoxLayout();
    low->setSpacing(14);
    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setMaximumHeight(120);
    m_log->setPlaceholderText(QStringLiteral("The time stamps at which every sensor must detect the bag appear here."));
    low->addWidget(m_log, 1);
    m_pushBtn = Theme::button(QString::fromUtf8("\xE2\x96\xB6  Put bag on the belt"), "primary", this);
    m_pushBtn->setMinimumSize(240, 48);
    m_pushBtn->setEnabled(false);
    low->addWidget(m_pushBtn, 0, Qt::AlignBottom);
    root->addLayout(low);

    connect(m_flights, &SimFlightList::selectionChanged, this, [this]() {
        m_selPnr.clear();
        m_selSeat.clear();
        m_signature.clear();
        m_pax->setRowCount(0);
        loadPassengers();
        updateButton();
    });
    connect(m_pax, &QTableWidget::itemSelectionChanged, this, [this]() {
        const int row = m_pax->currentRow();
        if (row >= 0 && m_pax->item(row, 1) && m_pax->item(row, 2)) {
            m_selSeat = m_pax->item(row, 1)->text();
            m_selPnr = m_pax->item(row, 2)->text();
            m_selStatus = m_pax->item(row, 5) ? m_pax->item(row, 5)->data(Qt::UserRole).toString() : QString();
        }
        updateButton();
    });
    connect(m_pushBtn, &QPushButton::clicked, this, &SimPushPage::push);
}

void SimPushPage::onEnter()
{
    m_pollCount = 0;
    m_signature.clear();
    m_flights->refresh();
    loadPassengers();
    onPoll();
}

void SimPushPage::onPoll()
{
    m_clock->setText(QStringLiteral("System time  ") + isoText(app()->serverNow()) + (app()->serverSimulated() ? QStringLiteral("   (simulated)") : QString()));
    if (++m_pollCount % 2 == 0) {
        m_flights->refresh();
        loadPassengers();
    }
}

void SimPushPage::loadPassengers()
{
    if (!m_flights->hasSelection()) {
        m_pax->setRowCount(0);
        m_signature.clear();
        updateButton();
        return;
    }
    const QString f = m_flights->flightId(), d = m_flights->date();
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("SIM_CHECKEDIN"), f, d});
    connect(r, &PendingReply::finished, this, [this, f, d](const Reply &rep) {
        if (!rep.ok || f != m_flights->flightId() || d != m_flights->date()) return;
        fillPassengers(rep.rowsOf(QStringLiteral("P")));
    });
}

void SimPushPage::fillPassengers(const QVector<QStringList> &all)
{
    // P|pnr|seat|name|age|gender|bagTag|bagKg|bagStatus|checkInTime   - only passengers who have a bag
    QVector<QStringList> rows;
    for (int i = 0; i < all.size(); ++i)
        if (fld(all.at(i), 7).toInt() > 0) rows.append(all.at(i));

    QString sig = m_selPnr + m_selSeat;
    for (int i = 0; i < rows.size(); ++i) sig += rows.at(i).join(QLatin1Char('|')) + QLatin1Char('\n');
    if (sig == m_signature) return;
    m_signature = sig;

    m_pax->blockSignals(true);
    m_pax->setRowCount(rows.size());
    int select = -1;
    for (int i = 0; i < rows.size(); ++i) {
        const QStringList &p = rows.at(i);
        const QString st = fld(p, 8);
        setCell(m_pax, i, 0, fld(p, 3));
        setCell(m_pax, i, 1, fld(p, 2));
        setCell(m_pax, i, 2, fld(p, 1));
        setCell(m_pax, i, 3, fld(p, 6));
        setCell(m_pax, i, 4, fld(p, 7));
        setCell(m_pax, i, 5, statusText(st), statusColor(st));
        m_pax->item(i, 5)->setData(Qt::UserRole, st);
        if (fld(p, 1) == m_selPnr && fld(p, 2) == m_selSeat) {
            select = i;
            m_selStatus = st;
        }
    }
    if (select >= 0) m_pax->selectRow(select);
    m_pax->blockSignals(false);
    updateButton();
}

void SimPushPage::updateButton()
{
    m_pushBtn->setEnabled(m_flights->hasSelection() && !m_selPnr.isEmpty() && m_selStatus == QLatin1String("NONE"));
}

void SimPushPage::push()
{
    if (!m_flights->hasSelection() || m_selPnr.isEmpty()) return;
    m_pushBtn->setEnabled(false);
    const QString seat = m_selSeat;
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("SIM_PUSH"), m_flights->flightId(), m_flights->date(),
                                                              m_selPnr, m_selSeat});
    connect(r, &PendingReply::finished, this, [this, seat](const Reply &rep) {
        if (!rep.ok) {
            m_log->appendPlainText(QStringLiteral("[!] ") + (rep.netOk ? rep.message : QStringLiteral("Cannot reach the server.")));
            updateButton();
            return;
        }
        const QString tag = rep.headField(1);
        const QVector<QStringList> et = rep.rowsOf(QStringLiteral("ET"));
        m_log->appendPlainText(QStringLiteral("Bag %1 (seat %2) crossed the start RFID and was inserted into the doubly linked list.").arg(tag, seat));
        for (int i = 0; i < et.size(); ++i)
            m_log->appendPlainText(QStringLiteral("    %1: %2  detects the bag at  %3   (+%4 s)")
                                       .arg(fld(et.at(i), 3) == QLatin1String("RFID") ? QStringLiteral("RFID  ") : QStringLiteral("Photon"),
                                            fld(et.at(i), 2), fld(et.at(i), 4), fld(et.at(i), 5)));
        m_signature.clear();
        m_selStatus = QStringLiteral("ONBELT");
        m_flights->refresh();
        loadPassengers();
    });
}

// =====================================================================================
//  SimLossPage
// =====================================================================================
SimLossPage::SimLossPage(MainWindow *mw, QWidget *parent) : BasePage(mw, parent), m_comboForPassed(-99), m_pollCount(0)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(28, 14, 28, 18);
    root->setSpacing(10);

    m_clock = Theme::label(QString(), "chip", this);
    root->addWidget(m_clock, 0, Qt::AlignLeft);

    m_flights = new SimFlightList(this);
    root->addWidget(m_flights, 2);

    root->addWidget(Theme::label(QStringLiteral("Passengers whose bags are on the belt"), "h2", this));
    m_bags = makeTable(this, QStringList() << QStringLiteral("Passenger") << QStringLiteral("Seat") << QStringLiteral("PNR")
                                           << QStringLiteral("Bag tag") << QStringLiteral("Bag status") << QStringLiteral("Last sensor"));
    m_bags->setMinimumHeight(130);
    root->addWidget(m_bags, 2);

    QFrame *ctl = Theme::card(this);
    QHBoxLayout *cl = new QHBoxLayout(ctl);
    cl->setContentsMargins(20, 14, 20, 14);
    cl->setSpacing(12);
    QVBoxLayout *sensorCol = new QVBoxLayout();
    sensorCol->setSpacing(4);
    sensorCol->addWidget(Theme::label(QStringLiteral("SENSOR THAT WILL NOT DETECT THE BAG"), "caption", ctl));
    m_sensor = new QComboBox(ctl);
    m_sensor->setMinimumWidth(300);
    sensorCol->addWidget(m_sensor);
    cl->addLayout(sensorCol);
    m_lossBtn = Theme::button(QString::fromUtf8("\xE2\x9A\xA0  Simulate bag loss"), "danger", ctl);
    m_lossBtn->setMinimumHeight(42);
    m_lossBtn->setEnabled(false);
    cl->addWidget(m_lossBtn, 0, Qt::AlignBottom);
    m_msg = Theme::label(QString(), "ok", ctl);
    m_msg->setWordWrap(true);
    m_msg->hide();
    cl->addWidget(m_msg, 1);
    root->addWidget(ctl);

    connect(m_flights, &SimFlightList::selectionChanged, this, [this]() {
        m_selPnr.clear();
        m_selSeat.clear();
        m_signature.clear();
        m_rows.clear();
        m_bags->setRowCount(0);
        loadBags();
        updateControls();
    });
    connect(m_bags, &QTableWidget::itemSelectionChanged, this, [this]() {
        const int row = m_bags->currentRow();
        if (row >= 0 && m_bags->item(row, 1) && m_bags->item(row, 2)) {
            m_selSeat = m_bags->item(row, 1)->text();
            m_selPnr = m_bags->item(row, 2)->text();
        }
        m_comboForPassed = -99;
        m_msg->hide();
        updateControls();
    });
    connect(m_lossBtn, &QPushButton::clicked, this, &SimLossPage::triggerLoss);
}

void SimLossPage::onEnter()
{
    m_pollCount = 0;
    m_signature.clear();
    m_msg->hide();
    // learn the sensors of the belt
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("BELT"), QStringLiteral("STAFF"), QStringLiteral("simulator")});
    connect(r, &PendingReply::finished, this, [this](const Reply &rep) {
        const BeltSnapshot snap = BeltSnapshot::fromReply(rep);
        if (!snap.valid) return;
        m_sensors = snap.sensors;
        m_comboForPassed = -99;
        updateControls();
    });
    m_flights->refresh();
    loadBags();
    onPoll();
}

void SimLossPage::onPoll()
{
    m_clock->setText(QStringLiteral("System time  ") + isoText(app()->serverNow()) + (app()->serverSimulated() ? QStringLiteral("   (simulated)") : QString()));
    if (++m_pollCount % 2 == 0) {
        m_flights->refresh();
        loadBags();
    }
}

void SimLossPage::loadBags()
{
    if (!m_flights->hasSelection()) {
        m_bags->setRowCount(0);
        m_rows.clear();
        m_signature.clear();
        updateControls();
        return;
    }
    const QString f = m_flights->flightId(), d = m_flights->date();
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("SIM_ONBELT"), f, d});
    connect(r, &PendingReply::finished, this, [this, f, d](const Reply &rep) {
        if (!rep.ok || f != m_flights->flightId() || d != m_flights->date()) return;
        fillBags(rep.rowsOf(QStringLiteral("B")));
    });
}

void SimLossPage::fillBags(const QVector<QStringList> &rows)
{
    // B|pnr|seat|name|tag|weight|state|passed|dropAt|lostAt
    m_rows = rows;
    QString sig = m_selPnr + m_selSeat;
    for (int i = 0; i < rows.size(); ++i) sig += rows.at(i).join(QLatin1Char('|')) + QLatin1Char('\n');
    if (sig == m_signature) return;
    m_signature = sig;

    m_bags->blockSignals(true);
    m_bags->setRowCount(rows.size());
    int select = -1;
    for (int i = 0; i < rows.size(); ++i) {
        const QStringList &b = rows.at(i);
        const QString st = fld(b, 6);
        QString last;
        for (int k = 0; k < m_sensors.size(); ++k)
            if (m_sensors.at(k).index == fld(b, 7).toInt()) last = m_sensors.at(k).name;
        if (last.isEmpty()) last = QStringLiteral("sensor ") + fld(b, 7);
        setCell(m_bags, i, 0, fld(b, 3));
        setCell(m_bags, i, 1, fld(b, 2));
        setCell(m_bags, i, 2, fld(b, 1));
        setCell(m_bags, i, 3, fld(b, 4));
        QString statusLabel = statusText(st);
        if (st == QLatin1String("ONBELT") && fld(b, 8).toInt() >= 0) statusLabel = QStringLiteral("On the belt - loss armed");
        setCell(m_bags, i, 4, statusLabel, statusColor(st));
        setCell(m_bags, i, 5, last);
        if (fld(b, 1) == m_selPnr && fld(b, 2) == m_selSeat) select = i;
    }
    if (select >= 0) m_bags->selectRow(select);
    m_bags->blockSignals(false);
    updateControls();
}

void SimLossPage::updateControls()
{
    const QStringList *sel = nullptr;
    for (int i = 0; i < m_rows.size(); ++i)
        if (fld(m_rows.at(i), 1) == m_selPnr && fld(m_rows.at(i), 2) == m_selSeat) sel = &m_rows.at(i);

    const bool onBelt = sel && fld(*sel, 6) == QLatin1String("ONBELT");
    const int passed = onBelt ? fld(*sel, 7).toInt() : -1;
    if (passed != m_comboForPassed) {                    // rebuild the sensor list only when needed
        m_comboForPassed = passed;
        m_sensor->clear();
        if (onBelt) {
            for (int i = 0; i < m_sensors.size(); ++i) {
                if (m_sensors.at(i).index <= passed) continue;          // the bag already went past this sensor
                m_sensor->addItem(QStringLiteral("%1   (%2 m from the start)").arg(m_sensors.at(i).name).arg(m_sensors.at(i).distance, 0, 'f', 0),
                                  m_sensors.at(i).index);
            }
        }
    }
    m_lossBtn->setEnabled(onBelt && m_sensor->count() > 0);
}

void SimLossPage::triggerLoss()
{
    if (!m_flights->hasSelection() || m_selPnr.isEmpty() || m_sensor->currentIndex() < 0) return;
    const int sensor = m_sensor->currentData().toInt();
    m_lossBtn->setEnabled(false);
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("SIM_LOSS"), m_flights->flightId(), m_flights->date(),
                                                              m_selPnr, m_selSeat, QString::number(sensor)});
    connect(r, &PendingReply::finished, this, [this](const Reply &rep) {
        if (!rep.ok) {
            Theme::setRole(m_msg, "error");
            m_msg->setText(rep.netOk ? rep.message : QStringLiteral("Cannot reach the server."));
        } else {
            Theme::setRole(m_msg, "ok");
            m_msg->setText(QStringLiteral("%1's bag will not be detected by \"%2\" (expected at %3). The loss appears on the passenger "
                                          "and belt staff portals as soon as that time stamp passes.")
                               .arg(rep.headField(1), rep.headField(2), rep.headField(3)));
        }
        m_msg->show();
        m_signature.clear();
        loadBags();
    });
}
