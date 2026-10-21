#include "bookingpages.h"

#include "mainwindow.h"
#include "serverapi.h"
#include "theme.h"
#include "seatwidget.h"
#include "ticketpages.h"
#include "homepages.h"

#include <QComboBox>
#include <QDateEdit>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTableView>
#include <QVBoxLayout>

static const char *ARROW = "\xE2\x86\x92";                         // →

static QVBoxLayout *captioned(const QString &caption, QWidget *w, QWidget *parent)
{
    QVBoxLayout *l = new QVBoxLayout();
    l->setSpacing(4);
    l->addWidget(Theme::label(caption, "caption", parent));
    l->addWidget(w);
    return l;
}

// =====================================================================================
//  BookTicketPage
// =====================================================================================
BookTicketPage::BookTicketPage(MainWindow *mw, QWidget *parent)
    : BasePage(mw, parent), m_hasCur(false), m_bookable(false), m_stopsLoaded(false), m_pollCount(0)
{
    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget *content = new QWidget();
    scroll->setWidget(content);
    Theme::transparentScroll(scroll);
    outer->addWidget(scroll);

    QVBoxLayout *root = new QVBoxLayout(content);
    root->setContentsMargins(28, 18, 28, 22);
    root->setSpacing(14);

    // ---------------------------------------------------------------- search card
    QFrame *sc = Theme::card(content);
    QHBoxLayout *sl = new QHBoxLayout(sc);
    sl->setContentsMargins(20, 16, 20, 16);
    sl->setSpacing(12);

    m_from = new QComboBox(sc);
    m_from->setMinimumWidth(230);
    m_to = new QComboBox(sc);
    m_to->setMinimumWidth(230);
    QPushButton *swap = Theme::button(QString::fromUtf8("\xE2\x87\x84"), "nav", sc);               // ⇄
    swap->setFixedWidth(48);
    swap->setToolTip(QStringLiteral("Swap From and To"));
    m_date = new QDateEdit(sc);
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat(QStringLiteral("dd MMM yyyy"));
    m_date->setMinimumWidth(150);
    m_searchBtn = Theme::button(QStringLiteral("Search flights"), "primary", sc);
    m_searchBtn->setMinimumHeight(40);

    sl->addLayout(captioned(QStringLiteral("FROM"), m_from, sc), 1);
    QVBoxLayout *swapCol = new QVBoxLayout();
    swapCol->addSpacing(18);
    swapCol->addWidget(swap);
    sl->addLayout(swapCol);
    sl->addLayout(captioned(QStringLiteral("TO"), m_to, sc), 1);
    sl->addLayout(captioned(QStringLiteral("DATE  (up to one month ahead)"), m_date, sc));
    QVBoxLayout *btnCol = new QVBoxLayout();
    btnCol->addSpacing(18);
    btnCol->addWidget(m_searchBtn);
    sl->addLayout(btnCol);
    root->addWidget(sc);

    // ---------------------------------------------------------------- result
    m_routeLabel = Theme::label(QStringLiteral("Choose where you want to fly"), "h2", content);
    m_routeInfo = Theme::label(QStringLiteral("The shortest route is found with a graph search over all flights in flights.dat."), "muted", content);
    root->addWidget(m_routeLabel);
    root->addWidget(m_routeInfo);

    m_model = new FlightModel(this);
    m_table = new QTableView(content);
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setSortingEnabled(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(34);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
    m_table->setMinimumHeight(200);
    m_table->setMaximumHeight(240);
    root->addWidget(m_table);

    // ---------------------------------------------------------------- seats + summary
    QHBoxLayout *bottom = new QHBoxLayout();
    bottom->setSpacing(14);

    QFrame *seatCard = Theme::card(content);
    QVBoxLayout *seatLayout = new QVBoxLayout(seatCard);
    seatLayout->setContentsMargins(20, 16, 20, 16);
    m_seatTitle = Theme::label(QStringLiteral("Seat matrix"), "h2", seatCard);
    seatLayout->addWidget(m_seatTitle);
    m_seatHint = Theme::label(QStringLiteral("Select a flight in the table above to see its seats."), "muted", seatCard);
    m_seatHint->setWordWrap(true);
    seatLayout->addWidget(m_seatHint);
    m_seats = new SeatWidget(seatCard);
    m_seats->hide();
    seatLayout->addWidget(m_seats, 0, Qt::AlignHCenter);
    seatLayout->addStretch(1);
    bottom->addWidget(seatCard, 3);

    QFrame *sumCard = Theme::card(content);
    QVBoxLayout *sumLayout = new QVBoxLayout(sumCard);
    sumLayout->setContentsMargins(22, 18, 22, 18);
    sumLayout->setSpacing(8);
    sumLayout->addWidget(Theme::label(QStringLiteral("Your selection"), "h2", sumCard));
    m_summary = new QLabel(sumCard);
    m_summary->setTextFormat(Qt::RichText);
    m_summary->setWordWrap(true);
    sumLayout->addWidget(m_summary);
    m_notice = Theme::label(QString(), "error", sumCard);
    m_notice->setWordWrap(true);
    m_notice->hide();
    sumLayout->addWidget(m_notice);
    sumLayout->addStretch(1);
    m_continue = Theme::button(QStringLiteral("Continue to passenger details"), "primary", sumCard);
    m_continue->setMinimumHeight(44);
    m_continue->setEnabled(false);
    sumLayout->addWidget(m_continue);
    bottom->addWidget(sumCard, 2);
    root->addLayout(bottom);
    root->addStretch(1);

    updateSummary();

    // ---------------------------------------------------------------- wiring
    connect(m_searchBtn, &QPushButton::clicked, this, &BookTicketPage::search);
    connect(swap, &QPushButton::clicked, this, [this]() {
        const int a = m_from->currentIndex();
        m_from->setCurrentIndex(m_to->currentIndex());
        m_to->setCurrentIndex(a);
    });
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this]() { onFlightSelected(); });
    connect(m_seats, &SeatWidget::selectionChanged, this, [this]() { updateSummary(); });
    connect(m_seats, &SeatWidget::seatTaken, this, [this](const QString &label) {
        m_notice->setText(QStringLiteral("Seat %1 was just booked by another passenger and was removed from your selection.").arg(label));
        m_notice->show();
    });
    connect(m_continue, &QPushButton::clicked, this, &BookTicketPage::continueToDetails);
}

void BookTicketPage::onEnter()
{
    const QDate today = app()->serverDate();
    m_date->setDateRange(today, today.addDays(30));
    if (!m_stopsLoaded) loadStops();
    if (m_hasCur) loadSeats();
}

void BookTicketPage::onPoll()
{
    // calendar limits follow the (possibly simulated) system date
    const QDate today = app()->serverDate();
    if (m_date->minimumDate() != today) m_date->setDateRange(today, today.addDays(30));
    if (!m_stopsLoaded) loadStops();
    // seats booked by other laptops show up here within a few seconds
    if (m_hasCur && ++m_pollCount % 2 == 0) loadSeats();
}

void BookTicketPage::clearSelection()
{
    m_seats->clearSelection();
    m_notice->hide();
    updateSummary();
}

void BookTicketPage::loadStops()
{
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("STOPS")});
    connect(r, &PendingReply::finished, this, [this](const Reply &rep) {
        if (!rep.ok || m_stopsLoaded) return;
        const QVector<QStringList> rows = rep.rowsOf(QStringLiteral("A"));
        if (rows.isEmpty()) return;
        m_from->clear();
        m_to->clear();
        for (int i = 0; i < rows.size(); ++i) {
            const QString text = fld(rows.at(i), 1) + QStringLiteral("  -  ") + fld(rows.at(i), 2);
            m_from->addItem(text, fld(rows.at(i), 1));
            m_to->addItem(text, fld(rows.at(i), 1));
        }
        m_from->setCurrentIndex(0);
        m_to->setCurrentIndex(rows.size() > 1 ? 1 : 0);
        m_stopsLoaded = true;
    });
}

void BookTicketPage::search()
{
    const QString from = m_from->currentData().toString();
    const QString to = m_to->currentData().toString();
    if (from.isEmpty() || to.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Book ticket"),
                                 QStringLiteral("The airport list could not be loaded. Check the server connection."));
        return;
    }
    if (from == to) {
        QMessageBox::warning(this, QStringLiteral("Book ticket"), QStringLiteral("From and To must be different airports."));
        return;
    }
    m_searchBtn->setEnabled(false);
    m_searchBtn->setText(QStringLiteral("Searching..."));
    m_hasCur = false;
    clearSeatPanel();

    PendingReply *r = ServerApi::instance().send(
        QStringList{QStringLiteral("SEARCH"), from, to, m_date->date().toString(Qt::ISODate)});
    connect(r, &PendingReply::finished, this, [this](const Reply &rep) {
        m_searchBtn->setEnabled(true);
        m_searchBtn->setText(QStringLiteral("Search flights"));
        if (!rep.netOk) {
            m_routeLabel->setText(QStringLiteral("Cannot reach the server"));
            m_routeInfo->setText(rep.message);
            return;
        }
        if (!rep.ok) {
            m_routeLabel->setText(rep.message);
            m_routeInfo->setText(QString());
            m_model->setFlights(QVector<FlightRecord>());
            return;
        }

        QVector<FlightRecord> list;
        const QVector<QStringList> legs = rep.rowsOf(QStringLiteral("LEG"));
        for (int i = 0; i < legs.size(); ++i) {
            const QStringList &l = legs.at(i);
            FlightRecord f;
            f.kind = fld(l, 1);
            f.index = fld(l, 2).toInt();
            f.id = fld(l, 3);
            f.origin = fld(l, 4);
            f.dest = fld(l, 5);
            f.date = fld(l, 6);
            f.depTime = fld(l, 7);
            f.arrText = fld(l, 8);
            f.durationMin = fld(l, 9).toInt();
            f.fare = fld(l, 10).toDouble();
            f.seatsTotal = fld(l, 11).toInt();
            f.seatsAvail = fld(l, 12).toInt();
            f.bookable = (fld(l, 13) == QLatin1String("1"));
            f.reason = fld(l, 14);
            list.append(f);
        }
        m_model->setFlights(list);

        const QVector<QStringList> route = rep.rowsOf(QStringLiteral("ROUTE"));
        if (!route.isEmpty()) {
            QString path = fld(route.first(), 1);
            path.replace(QLatin1Char('>'), QString::fromUtf8(" ") + QString::fromUtf8(ARROW) + QString::fromUtf8(" "));
            const int n = fld(route.first(), 4).toInt();
            m_routeLabel->setText(QStringLiteral("Shortest route:  ") + path);
            m_routeInfo->setText(QStringLiteral("%1 in total  |  %2 for all flights  |  %3")
                                     .arg(Theme::duration(fld(route.first(), 2).toInt()),
                                          Theme::money(fld(route.first(), 3).toDouble()),
                                          n > 1 ? QStringLiteral("%1 flights - select a flight and book each one in turn").arg(n)
                                                : QStringLiteral("direct flight")));
        } else {
            m_routeLabel->setText(QStringLiteral("No route available on this date"));
            m_routeInfo->setText(list.isEmpty() ? QStringLiteral("Try another date or another pair of airports.")
                                                : QStringLiteral("Other direct flights between these airports are listed below (for example already departed)."));
        }
        if (!list.isEmpty()) m_table->selectRow(0);
    });
}

void BookTicketPage::onFlightSelected()
{
    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    m_cur = m_model->flightAt(rows.first().row());
    m_hasCur = true;
    m_bookable = m_cur.bookable;
    m_seats->clearSelection();
    m_notice->hide();
    m_seatTitle->setText(QStringLiteral("Seat matrix  -  %1  %2 %3 %4  |  %5").arg(m_cur.id, m_cur.origin, QString::fromUtf8(ARROW), m_cur.dest, m_cur.date));
    m_seatHint->setText(QStringLiteral("Loading seats..."));
    m_seatHint->show();
    updateSummary();
    loadSeats();
}

void BookTicketPage::loadSeats()
{
    if (!m_hasCur) return;
    const QString id = m_cur.id;
    const QString date = m_cur.date;
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("SEATS"), id, date});
    connect(r, &PendingReply::finished, this, [this, id, date](const Reply &rep) {
        if (!m_hasCur || m_cur.id != id || m_cur.date != date) return;      // the user selected another flight meanwhile
        if (!rep.ok) {
            if (rep.netOk) {
                m_seatHint->setText(rep.message);
                m_seatHint->show();
            }
            return;
        }
        applySeats(rep);
    });
}

void BookTicketPage::applySeats(const Reply &rep)
{
    const int total = rep.headField(7).toInt();
    m_bookable = (rep.headField(8) == QLatin1String("1"));
    const QString reason = rep.headField(9);
    m_cur.fare = rep.headField(6).toDouble();

    QStringList sold;
    const QVector<QStringList> soldRows = rep.rowsOf(QStringLiteral("SOLD"));
    if (!soldRows.isEmpty()) {
        sold = soldRows.first();
        sold.removeFirst();                                                   // drop the "SOLD" tag
    }
    m_seats->setSeatCount(total);
    m_seats->setInteractive(m_bookable);
    m_seats->setSold(sold);
    m_seats->show();
    m_seatHint->hide();

    if (!m_bookable) {
        m_notice->setText(reason.isEmpty() ? QStringLiteral("This flight cannot be booked.") : reason);
        m_notice->show();
    } else if (m_notice->text().startsWith(QStringLiteral("Booking")) || m_notice->text().startsWith(QStringLiteral("Flight"))) {
        m_notice->hide();
    }
    updateSummary();
}

void BookTicketPage::clearSeatPanel()
{
    m_seats->clearSelection();
    m_seats->hide();
    m_seatTitle->setText(QStringLiteral("Seat matrix"));
    m_seatHint->setText(QStringLiteral("Select a flight in the table above to see its seats."));
    m_seatHint->show();
    m_notice->hide();
    updateSummary();
}

void BookTicketPage::updateSummary()
{
    const QStringList sel = m_seats->selectedLabels();
    if (!m_hasCur) {
        m_summary->setText(QStringLiteral("<span style='color:#64748b'>No flight selected yet.</span>"));
        m_continue->setEnabled(false);
        return;
    }
    QString html = QStringLiteral("<b>%1</b> &nbsp; %2 %3 %4<br>%5, departs %6<br>Fare per seat: %7<br><br>")
                       .arg(m_cur.id, m_cur.origin, QString::fromUtf8(ARROW), m_cur.dest, m_cur.date, m_cur.depTime,
                            Theme::money(m_cur.fare));
    if (sel.isEmpty()) {
        html += QStringLiteral("<span style='color:#64748b'>Pick one or more seats on the seat map.</span>");
    } else {
        html += QStringLiteral("Seats: <b>%1</b><br>Total: <b style='font-size:14pt'>%2</b>")
                    .arg(sel.join(QStringLiteral(", ")), Theme::money(m_cur.fare * sel.size()));
    }
    m_summary->setText(html);
    m_continue->setEnabled(!sel.isEmpty() && m_bookable);
}

void BookTicketPage::continueToDetails()
{
    const QStringList sel = m_seats->selectedLabels();
    if (!m_hasCur || sel.isEmpty()) return;
    app()->detailsPage()->setBooking(m_cur, sel);
    app()->navigateTo(app()->detailsPage());
}

// =====================================================================================
//  PassengerDetailsPage
// =====================================================================================
PassengerDetailsPage::PassengerDetailsPage(MainWindow *mw, QWidget *parent) : BasePage(mw, parent)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(28, 18, 28, 18);
    root->setSpacing(12);

    QFrame *sumCard = Theme::card(this);
    QVBoxLayout *sl = new QVBoxLayout(sumCard);
    sl->setContentsMargins(22, 14, 22, 14);
    m_summary = new QLabel(sumCard);
    m_summary->setTextFormat(Qt::RichText);
    m_summary->setWordWrap(true);
    sl->addWidget(m_summary);
    root->addWidget(sumCard);

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_formHost = new QWidget();
    m_formLayout = new QVBoxLayout(m_formHost);
    m_formLayout->setContentsMargins(0, 0, 6, 0);
    m_formLayout->setSpacing(12);
    scroll->setWidget(m_formHost);
    Theme::transparentScroll(scroll);
    root->addWidget(scroll, 1);

    m_msg = Theme::label(QString(), "error", this);
    m_msg->setWordWrap(true);
    m_msg->hide();
    root->addWidget(m_msg);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    m_confirm = Theme::button(QStringLiteral("Confirm booking"), "primary", this);
    m_confirm->setMinimumSize(220, 44);
    btnRow->addWidget(m_confirm);
    root->addLayout(btnRow);

    connect(m_confirm, &QPushButton::clicked, this, &PassengerDetailsPage::confirm);
}

void PassengerDetailsPage::setBooking(const FlightRecord &leg, const QStringList &seats)
{
    m_leg = leg;
    m_msg->hide();
    m_confirm->setEnabled(true);
    m_summary->setText(QStringLiteral("<span style='font-size:13pt'><b>%1</b> &nbsp; %2 %3 %4</span><br>"
                                      "%5 at %6 &nbsp;|&nbsp; Seats: <b>%7</b> &nbsp;|&nbsp; Total: <b>%8</b><br>"
                                      "<span style='color:#64748b'>Enter the details of every passenger who will sit in the selected seats.</span>")
                           .arg(leg.id, leg.origin, QString::fromUtf8(ARROW), leg.dest, leg.date, leg.depTime,
                                seats.join(QStringLiteral(", ")), Theme::money(leg.fare * seats.size())));

    // throw away the old form
    while (QLayoutItem *item = m_formLayout->takeAt(0)) {
        if (QWidget *w = item->widget()) delete w;
        delete item;
    }
    m_rows.clear();

    for (int i = 0; i < seats.size(); ++i) {
        FormRow row;
        row.seat = seats.at(i);

        QFrame *card = Theme::card(m_formHost);
        QGridLayout *g = new QGridLayout(card);
        g->setContentsMargins(20, 14, 20, 16);
        g->setHorizontalSpacing(14);
        g->setVerticalSpacing(4);
        g->addWidget(Theme::label(QStringLiteral("Passenger %1  -  Seat %2").arg(i + 1).arg(row.seat), "h2", card), 0, 0, 1, 4);

        row.name = new QLineEdit(card);
        row.name->setPlaceholderText(QStringLiteral("Full name as on the ID"));
        row.name->setMaxLength(40);
        row.age = new QSpinBox(card);
        row.age->setRange(0, 120);
        row.age->setValue(30);
        row.gender = new QComboBox(card);
        row.gender->addItem(QStringLiteral("Male"));
        row.gender->addItem(QStringLiteral("Female"));
        row.gender->addItem(QStringLiteral("Other"));
        row.bag = new QSpinBox(card);
        row.bag->setRange(0, 40);
        row.bag->setValue(15);
        row.bag->setSuffix(QStringLiteral(" kg"));
        row.bag->setSpecialValueText(QStringLiteral("No checked bag"));

        g->addWidget(Theme::label(QStringLiteral("FULL NAME"), "caption", card), 1, 0);
        g->addWidget(Theme::label(QStringLiteral("AGE"), "caption", card), 1, 1);
        g->addWidget(Theme::label(QStringLiteral("GENDER"), "caption", card), 1, 2);
        g->addWidget(Theme::label(QStringLiteral("CHECKED BAG"), "caption", card), 1, 3);
        g->addWidget(row.name, 2, 0);
        g->addWidget(row.age, 2, 1);
        g->addWidget(row.gender, 2, 2);
        g->addWidget(row.bag, 2, 3);
        g->setColumnStretch(0, 3);
        g->setColumnStretch(1, 1);
        g->setColumnStretch(2, 1);
        g->setColumnStretch(3, 1);

        m_formLayout->addWidget(card);
        m_rows.append(row);
    }
    m_formLayout->addStretch(1);
    if (!m_rows.isEmpty()) m_rows.first().name->setFocus();
}

void PassengerDetailsPage::confirm()
{
    m_msg->hide();
    QStringList req;
    req << QStringLiteral("BOOK") << app()->session().username << m_leg.id << m_leg.date << QString::number(m_rows.size());
    for (int i = 0; i < m_rows.size(); ++i) {
        const FormRow &r = m_rows.at(i);
        const QString name = r.name->text().trimmed();
        if (name.isEmpty()) {
            m_msg->setText(QStringLiteral("Please enter the name of the passenger in seat %1.").arg(r.seat));
            m_msg->show();
            r.name->setFocus();
            return;
        }
        req << r.seat << name << QString::number(r.age->value()) << r.gender->currentText() << QString::number(r.bag->value());
    }

    m_confirm->setEnabled(false);
    m_confirm->setText(QStringLiteral("Booking..."));
    PendingReply *p = ServerApi::instance().send(req);
    connect(p, &PendingReply::finished, this, [this](const Reply &rep) {
        m_confirm->setEnabled(true);
        m_confirm->setText(QStringLiteral("Confirm booking"));
        if (!rep.netOk) {
            m_msg->setText(QStringLiteral("Cannot reach the server: ") + rep.message);
            m_msg->show();
            return;
        }
        if (!rep.ok) {
            QMessageBox::warning(this, QStringLiteral("Booking failed"), rep.message);
            if (rep.code == QLatin1String("seat_taken") || rep.code == QLatin1String("not_open")
                || rep.code == QLatin1String("departed") || rep.code == QLatin1String("not_operating"))
                app()->goBack();                                              // back to the seat map, which refreshes
            else {
                m_msg->setText(rep.message);
                m_msg->show();
            }
            return;
        }
        const QString pnr = rep.headField(1);
        QMessageBox::information(this, QStringLiteral("Booking confirmed"),
            QStringLiteral("Your booking is confirmed.\n\nBooking reference (PNR):  %1\nTotal fare:  %2\n\n"
                           "Your boarding passes with the check-in codes are shown next.")
                .arg(pnr, Theme::money(rep.headField(2).toDouble())));
        app()->bookPage()->clearSelection();
        app()->boardingPage()->showPnr(pnr);
        app()->navigateReplacing(app()->boardingPage(), app()->passengerPortal());
    });
}
