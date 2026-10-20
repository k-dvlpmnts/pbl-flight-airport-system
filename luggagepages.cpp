#include "luggagepages.h"

#include "mainwindow.h"
#include "serverapi.h"
#include "theme.h"
#include "beltwidget.h"
#include "loginwindow.h"

#include <QBrush>
#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

// gives a page a scrollable content area and returns the layout to fill
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

QFrame *makeAlertBox(QWidget *parent, QLabel **textOut)
{
    QFrame *box = new QFrame(parent);
    box->setObjectName(QStringLiteral("alertBox"));
    QHBoxLayout *l = new QHBoxLayout(box);
    l->setContentsMargins(18, 12, 18, 12);
    QLabel *text = Theme::label(QString(), "error", box);
    text->setWordWrap(true);
    l->addWidget(text);
    box->hide();
    *textOut = text;
    return box;
}

QScrollArea *beltScroller(BeltWidget *belt, QWidget *parent)
{
    QScrollArea *sa = new QScrollArea(parent);
    sa->setWidgetResizable(true);
    sa->setWidget(belt);
    sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sa->setFixedHeight(belt->minimumHeight() + 22);
    Theme::transparentScroll(sa);
    return sa;
}

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

QColor stateTextColor(const QString &state)
{
    if (state == QLatin1String("DELIVERED")) return QColor("#15803d");
    if (state == QLatin1String("LOST")) return QColor("#b91c1c");
    if (state == QLatin1String("RECOVERED")) return QColor("#b45309");
    return QColor("#1d4ed8");
}

QString stateWord(const BagItem &b)
{
    if (b.state == QLatin1String("DELIVERED")) return QStringLiteral("Delivered");
    if (b.state == QLatin1String("LOST")) return QString::fromUtf8("\xE2\x9A\xA0  LOST");
    if (b.state == QLatin1String("RECOVERED")) return QStringLiteral("Recovered - safe");
    return QStringLiteral("On the belt");
}

void setCell(QTableWidget *t, int row, int col, const QString &text, const QColor *color = nullptr)
{
    QTableWidgetItem *item = new QTableWidgetItem(text);
    if (color) item->setForeground(QBrush(*color));
    t->setItem(row, col, item);
}

} // namespace

// =====================================================================================
//  TrackLuggagePage  (passenger)
// =====================================================================================
TrackLuggagePage::TrackLuggagePage(MainWindow *mw, QWidget *parent) : BasePage(mw, parent)
{
    QVBoxLayout *root = scrollRoot(this);
    root->setContentsMargins(28, 18, 28, 18);
    root->setSpacing(12);

    m_alertBox = makeAlertBox(this, &m_alertText);
    root->addWidget(m_alertBox);

    QFrame *beltCard = Theme::card(this);
    QVBoxLayout *bl = new QVBoxLayout(beltCard);
    bl->setContentsMargins(18, 14, 18, 10);
    bl->addWidget(Theme::label(QStringLiteral("Live baggage belt"), "h2", beltCard));
    QLabel *hint = Theme::label(QStringLiteral("You only see your own bags. Each bag is a node in the belt's doubly linked list; "
                                               "a missed sensor reading means the bag fell off."), "muted", beltCard);
    hint->setWordWrap(true);
    bl->addWidget(hint);
    m_belt = new BeltWidget(beltCard);
    bl->addWidget(beltScroller(m_belt, beltCard));
    root->addWidget(beltCard);

    root->addWidget(Theme::label(QStringLiteral("Your bags"), "h2", this));
    m_table = makeTable(this, QStringList() << QStringLiteral("Bag tag") << QStringLiteral("Passenger")
                                            << QStringLiteral("Flight") << QStringLiteral("Status") << QStringLiteral("Details"));
    m_table->setMinimumHeight(220);
    root->addWidget(m_table, 1);
    m_empty = Theme::label(QStringLiteral("No bags yet. Bags appear here after you book a ticket with a checked bag."), "muted", this);
    m_empty->hide();
    root->addWidget(m_empty);

    connect(mw, &MainWindow::beltUpdated, this, [this](const BeltSnapshot &snap) { render(snap); });
}

void TrackLuggagePage::onEnter()
{
    render(app()->belt());
}

void TrackLuggagePage::render(const BeltSnapshot &snap)
{
    m_belt->setSnapshot(snap, false);

    QStringList lost;
    for (int i = 0; i < snap.bags.size(); ++i)
        if (snap.bags.at(i).isLost())
            lost.append(QStringLiteral("%1 (tag %2) was not detected by \"%3\" at %4")
                            .arg(snap.bags.at(i).name, snap.bags.at(i).tag, snap.sensorName(snap.bags.at(i).lostAt), snap.bags.at(i).lostTime));
    if (lost.isEmpty()) {
        m_alertBox->hide();
    } else {
        m_alertText->setText(QString::fromUtf8("\xE2\x9A\xA0  LUGGAGE LOSS:  ") + lost.join(QStringLiteral(";  ")) +
                             QStringLiteral(".  The belt staff have been alerted and will pick the bag up."));
        m_alertBox->show();
    }

    const int rows = snap.bags.size() + snap.pending.size();
    m_table->setRowCount(rows);
    int r = 0;
    for (int i = snap.bags.size() - 1; i >= 0; --i, ++r) {                  // newest first
        const BagItem &b = snap.bags.at(i);
        const QColor col = stateTextColor(b.state);
        QString details;
        if (b.state == QLatin1String("ONBELT")) {
            const int next = b.passed + 1;
            details = QStringLiteral("Passed %1.  Next sensor (%2) expected at %3")
                          .arg(snap.sensorName(b.passed), snap.sensorName(next), expectedTimeText(snap, b, next));
        } else if (b.state == QLatin1String("DELIVERED")) {
            details = QStringLiteral("Reached the end RFID at ") + b.deliveredTime;
        } else if (b.state == QLatin1String("LOST")) {
            details = QStringLiteral("Not detected by %1 at %2 - staff alerted").arg(snap.sensorName(b.lostAt), b.lostTime);
        } else {
            details = QStringLiteral("Picked up by ") + b.recoveredInfo;
        }
        setCell(m_table, r, 0, b.tag);
        setCell(m_table, r, 1, b.name);
        setCell(m_table, r, 2, b.flight + QStringLiteral("  ") + b.date);
        setCell(m_table, r, 3, stateWord(b), &col);
        setCell(m_table, r, 4, details);
    }
    for (int i = 0; i < snap.pending.size(); ++i, ++r) {
        const PendingBag &p = snap.pending.at(i);
        const QColor grey("#64748b");
        setCell(m_table, r, 0, p.tag);
        setCell(m_table, r, 1, p.name);
        setCell(m_table, r, 2, p.flight + QStringLiteral("  ") + p.date + QStringLiteral("  ") + p.dep);
        setCell(m_table, r, 3, QStringLiteral("Not on the belt yet"), &grey);
        setCell(m_table, r, 4, p.checkedIn ? QStringLiteral("Checked in - waiting to be put on the belt")
                                           : QStringLiteral("Not checked in yet (check-in closes 3 hours before departure)"));
    }
    m_empty->setVisible(rows == 0);
}

// =====================================================================================
//  BeltStaffPage
// =====================================================================================
BeltStaffPage::BeltStaffPage(MainWindow *mw, QWidget *parent) : BasePage(mw, parent)
{
    QVBoxLayout *root = scrollRoot(this);
    root->setContentsMargins(28, 14, 28, 18);
    root->setSpacing(10);

    QHBoxLayout *top = new QHBoxLayout();
    top->addWidget(Theme::label(QStringLiteral("Baggage belt"), "h2", this));
    top->addStretch(1);
    QPushButton *admin = Theme::button(QStringLiteral("Admin"), "ghost", this);
    top->addWidget(admin);
    root->addLayout(top);
    connect(admin, &QPushButton::clicked, this, [this]() { app()->navigateTo(app()->adminLogin()); });

    m_alertBox = makeAlertBox(this, &m_alertText);
    root->addWidget(m_alertBox);

    QFrame *beltCard = Theme::card(this);
    QVBoxLayout *bl = new QVBoxLayout(beltCard);
    bl->setContentsMargins(18, 10, 18, 8);
    m_belt = new BeltWidget(beltCard);
    bl->addWidget(beltScroller(m_belt, beltCard));
    root->addWidget(beltCard);

    QHBoxLayout *bottom = new QHBoxLayout();
    bottom->setSpacing(14);
    QVBoxLayout *left = new QVBoxLayout();
    left->addWidget(Theme::label(QStringLiteral("All bags (newest first)"), "h2", this));
    m_table = makeTable(this, QStringList() << QStringLiteral("#") << QStringLiteral("Bag tag") << QStringLiteral("Passenger")
                                            << QStringLiteral("Flight") << QStringLiteral("Seat") << QStringLiteral("Kg")
                                            << QStringLiteral("Status") << QStringLiteral("Last sensor"));
    m_table->setMinimumHeight(260);
    left->addWidget(m_table, 1);
    bottom->addLayout(left, 5);

    QFrame *detailCard = Theme::card(this);
    detailCard->setMinimumHeight(300);
    QVBoxLayout *dl = new QVBoxLayout(detailCard);
    dl->setContentsMargins(20, 16, 20, 16);
    dl->setSpacing(8);
    dl->addWidget(Theme::label(QStringLiteral("Selected bag"), "h2", detailCard));
    m_detail = new QLabel(detailCard);
    m_detail->setTextFormat(Qt::RichText);
    m_detail->setWordWrap(true);
    m_detail->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    dl->addWidget(m_detail, 1);
    m_msg = Theme::label(QString(), "ok", detailCard);
    m_msg->setWordWrap(true);
    m_msg->hide();
    dl->addWidget(m_msg);
    m_safeBtn = Theme::button(QString::fromUtf8("\xE2\x9C\x93  Mark bag safe (picked up)"), "primary", detailCard);
    m_safeBtn->setMinimumHeight(42);
    m_safeBtn->setEnabled(false);
    dl->addWidget(m_safeBtn);
    bottom->addWidget(detailCard, 4);
    root->addLayout(bottom, 1);

    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]() {
        const int row = m_table->currentRow();
        if (row >= 0 && m_table->item(row, 1)) m_selectedTag = m_table->item(row, 1)->text();
        m_belt->setSelectedTag(m_selectedTag);
        showDetail();
    });
    connect(m_safeBtn, &QPushButton::clicked, this, &BeltStaffPage::markSafe);
    connect(mw, &MainWindow::beltUpdated, this, [this](const BeltSnapshot &snap) { render(snap); });
}

void BeltStaffPage::onEnter()
{
    m_msg->hide();
    m_signature.clear();
    render(app()->belt());
}

void BeltStaffPage::render(const BeltSnapshot &snap)
{
    m_snap = snap;
    m_belt->setSnapshot(snap, true);
    m_belt->setSelectedTag(m_selectedTag);

    // ---- alert banner: only bags that fell off the belt
    QStringList lost;
    for (int i = 0; i < snap.bags.size(); ++i) {
        const BagItem &b = snap.bags.at(i);
        if (b.isLost())
            lost.append(QStringLiteral("%1 - %2 (flight %3, seat %4) missed by \"%5\" at %6")
                            .arg(b.tag, b.name, b.flight, b.seat, snap.sensorName(b.lostAt), b.lostTime));
    }
    if (lost.isEmpty()) {
        m_alertBox->hide();
    } else {
        m_alertText->setText(QString::fromUtf8("\xE2\x9A\xA0  BAG FELL OFF THE BELT:  ") + lost.join(QStringLiteral("   |   ")) +
                             QStringLiteral("   -   pick it up, then select it and press \"Mark bag safe\"."));
        m_alertBox->show();
    }

    // ---- table (only rebuilt when something relevant changed)
    QString sig;
    for (int i = 0; i < snap.bags.size(); ++i) {
        const BagItem &b = snap.bags.at(i);
        sig += b.tag + b.state + QString::number(b.passed) + QLatin1Char('/') + QString::number(b.lostAt) + QLatin1Char(';');
    }
    if (sig != m_signature) {
        m_signature = sig;
        m_table->blockSignals(true);
        m_table->setRowCount(snap.bags.size());
        int select = -1;
        int r = 0;
        for (int i = snap.bags.size() - 1; i >= 0; --i, ++r) {
            const BagItem &b = snap.bags.at(i);
            const QColor col = stateTextColor(b.state);
            setCell(m_table, r, 0, QString::number(i + 1));
            setCell(m_table, r, 1, b.tag);
            setCell(m_table, r, 2, b.name);
            setCell(m_table, r, 3, b.flight);
            setCell(m_table, r, 4, b.seat);
            setCell(m_table, r, 5, QString::number(b.weight));
            setCell(m_table, r, 6, stateWord(b), &col);
            setCell(m_table, r, 7, b.state == QLatin1String("LOST") || b.state == QLatin1String("RECOVERED")
                                       ? QStringLiteral("missed ") + snap.sensorName(b.lostAt) : snap.sensorName(b.passed));
            if (b.tag == m_selectedTag) select = r;
        }
        if (select >= 0) m_table->selectRow(select);
        m_table->blockSignals(false);
    }
    showDetail();
}

void BeltStaffPage::showDetail()
{
    const BagItem *bag = nullptr;
    for (int i = 0; i < m_snap.bags.size(); ++i)
        if (m_snap.bags.at(i).tag == m_selectedTag) bag = &m_snap.bags.at(i);

    if (!bag) {
        m_detail->setText(QStringLiteral("<span style='color:#64748b'>Select a bag in the table to see the time stamps at which every sensor "
                                         "must detect it.</span>"));
        m_safeBtn->setEnabled(false);
        return;
    }
    QString html = QStringLiteral("<b>%1</b> &nbsp; %2<br>Flight %3 &nbsp;|&nbsp; seat %4 &nbsp;|&nbsp; %5 kg<br>PNR %6<br><br>"
                                  "<b>Sensor time stamps</b> (belt speed %7 m/s)<table cellspacing='4'>")
                       .arg(bag->tag, bag->name, bag->flight, bag->seat, QString::number(bag->weight), bag->pnr,
                            QString::number(m_snap.speed, 'f', 2));
    for (int i = 0; i < m_snap.sensors.size(); ++i) {
        const SensorInfo &s = m_snap.sensors.at(i);
        QString mark = QStringLiteral("<span style='color:#64748b'>waiting</span>");
        if (bag->lostAt == s.index) mark = QString::fromUtf8("<span style='color:#b91c1c'><b>\xE2\x9C\x95 MISSED</b></span>");
        else if (bag->lostAt >= 0 && s.index > bag->lostAt) mark = QStringLiteral("<span style='color:#94a3b8'>-</span>");
        else if (s.index <= bag->passed) mark = QString::fromUtf8("<span style='color:#15803d'>\xE2\x9C\x93 detected</span>");
        html += QStringLiteral("<tr><td>%1</td><td><b>%2</b></td><td>%3</td></tr>")
                    .arg(s.name, expectedTimeText(m_snap, *bag, s.index), mark);
    }
    html += QStringLiteral("</table>");
    if (bag->state == QLatin1String("LOST"))
        html += QStringLiteral("<br><span style='color:#b91c1c'><b>Lost</b> - identified through the linked list as the bag that follows "
                               "the last bag this sensor saw.</span>");
    if (bag->state == QLatin1String("RECOVERED"))
        html += QStringLiteral("<br><span style='color:#15803d'>Recovered: ") + bag->recoveredInfo + QStringLiteral("</span>");
    m_detail->setText(html);
    m_safeBtn->setEnabled(bag->state == QLatin1String("LOST"));
}

void BeltStaffPage::markSafe()
{
    if (m_selectedTag.isEmpty()) return;
    const QString tag = m_selectedTag;
    m_safeBtn->setEnabled(false);
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("BELT_SAFE"), app()->session().username, tag});
    connect(r, &PendingReply::finished, this, [this, tag](const Reply &rep) {
        if (!rep.ok) {
            Theme::setRole(m_msg, "error");
            m_msg->setText(rep.netOk ? rep.message : QStringLiteral("Cannot reach the server."));
        } else {
            Theme::setRole(m_msg, "ok");
            m_msg->setText(QStringLiteral("Bag %1 marked safe. The passenger portal has been updated.").arg(tag));
        }
        m_msg->show();
        showDetail();
    });
}
