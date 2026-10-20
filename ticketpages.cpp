#include "ticketpages.h"

#include "mainwindow.h"
#include "serverapi.h"
#include "theme.h"

#include <QBrush>
#include <QColor>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

// =====================================================================================
//  BarcodeWidget
// =====================================================================================
BarcodeWidget::BarcodeWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(46);
}

void BarcodeWidget::setText(const QString &text)
{
    m_text = text;
    update();
}

void BarcodeWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    // tiny deterministic generator seeded with the text -> the same pass always gives the same bars
    unsigned int seed = 2166136261u;
    for (int i = 0; i < m_text.size(); ++i) seed = (seed ^ static_cast<unsigned int>(m_text.at(i).unicode())) * 16777619u;

    p.fillRect(rect(), QColor("#ffffff"));
    int x = 4;
    const int h = height() - 6;
    while (x < width() - 4) {
        seed = seed * 1664525u + 1013904223u;
        const int barW = 1 + static_cast<int>((seed >> 24) % 4);
        const int gap = 1 + static_cast<int>((seed >> 20) % 3);
        p.fillRect(x, 3, barW, h, QColor("#0f172a"));
        x += barW + gap;
    }
}

// =====================================================================================
//  BoardingPassCard
// =====================================================================================
BoardingPassCard::BoardingPassCard(QWidget *parent) : QFrame(parent)
{
    setObjectName(QStringLiteral("pass"));
    setStyleSheet(QStringLiteral(
        "QFrame#pass { background: #ffffff; border: 1px solid #cbd5e1; border-radius: 16px; }"
        "QFrame#passHead { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1d4ed8, stop:1 #38bdf8);"
        "                  border: none; border-top-left-radius: 16px; border-top-right-radius: 16px; }"
        "QFrame#passHead QLabel { color: #ffffff; font-weight: 700; background: transparent; }"));

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    QFrame *head = new QFrame(this);
    head->setObjectName(QStringLiteral("passHead"));
    QHBoxLayout *hl = new QHBoxLayout(head);
    hl->setContentsMargins(22, 12, 22, 12);
    hl->addWidget(new QLabel(QString::fromUtf8("\xE2\x9C\x88  SKYPORT AIRLINES"), head));
    hl->addStretch(1);
    hl->addWidget(new QLabel(QStringLiteral("BOARDING PASS"), head));
    root->addWidget(head);

    QWidget *body = new QWidget(this);
    QVBoxLayout *b = new QVBoxLayout(body);
    b->setContentsMargins(24, 18, 24, 18);
    b->setSpacing(8);

    QHBoxLayout *route = new QHBoxLayout();
    m_from = new QLabel(body);
    m_to = new QLabel(body);
    m_from->setStyleSheet(QStringLiteral("font-size: 30pt; font-weight: 800; color: #0f172a;"));
    m_to->setStyleSheet(QStringLiteral("font-size: 30pt; font-weight: 800; color: #0f172a;"));
    QLabel *plane = new QLabel(QString::fromUtf8("\xE2\x9C\x88"), body);
    plane->setStyleSheet(QStringLiteral("font-size: 22pt; color: #2563eb;"));
    route->addWidget(m_from);
    route->addSpacing(16);
    route->addWidget(plane);
    route->addSpacing(16);
    route->addWidget(m_to);
    route->addStretch(1);
    m_status = Theme::label(QString(), "chip", body);
    route->addWidget(m_status, 0, Qt::AlignTop);
    b->addLayout(route);

    QHBoxLayout *who = new QHBoxLayout();
    QVBoxLayout *whoText = new QVBoxLayout();
    whoText->setSpacing(0);
    m_name = new QLabel(body);
    m_name->setStyleSheet(QStringLiteral("font-size: 15pt; font-weight: 700;"));
    m_sub = Theme::label(QString(), "muted", body);
    whoText->addWidget(m_name);
    whoText->addWidget(m_sub);
    who->addLayout(whoText, 1);
    m_bagStatus = Theme::label(QString(), "chip", body);
    who->addWidget(m_bagStatus, 0, Qt::AlignVCenter);
    b->addLayout(who);
    b->addSpacing(4);

    QGridLayout *g = new QGridLayout();
    g->setHorizontalSpacing(22);
    g->setVerticalSpacing(2);
    m_flight = addField(g, 0, 0, QStringLiteral("FLIGHT"));
    m_date = addField(g, 0, 1, QStringLiteral("DATE"));
    m_dep = addField(g, 0, 2, QStringLiteral("DEPARTS"));
    m_boarding = addField(g, 0, 3, QStringLiteral("BOARDING"));
    m_gate = addField(g, 0, 4, QStringLiteral("GATE"));
    m_seat = addField(g, 0, 5, QStringLiteral("SEAT"));
    m_pnr = addField(g, 2, 0, QStringLiteral("PNR (BOOKING REF)"));
    m_code = addField(g, 2, 1, QStringLiteral("CHECK-IN CODE"));
    m_tag = addField(g, 2, 2, QStringLiteral("BAG TAG"));
    m_bag = addField(g, 2, 4, QStringLiteral("CHECKED BAG"));
    b->addLayout(g);
    b->addSpacing(6);

    m_barcode = new BarcodeWidget(body);
    b->addWidget(m_barcode);
    QLabel *note = Theme::label(QStringLiteral("Give the PNR and the check-in code to the check-in staff. "
                                               "Check-in closes 3 hours before departure."), "muted", body);
    note->setWordWrap(true);
    b->addWidget(note);
    root->addWidget(body);
}

QLabel *BoardingPassCard::addField(QGridLayout *grid, int row, int col, const QString &caption)
{
    QLabel *cap = Theme::label(caption, "caption", this);
    QLabel *val = new QLabel(this);
    val->setStyleSheet(QStringLiteral("font-size: 12pt; font-weight: 700;"));
    grid->addWidget(cap, row, col);
    grid->addWidget(val, row + 1, col);
    return val;
}

void BoardingPassCard::setPass(const QStringList &bp)
{
    // BP|pnr|seat|name|age|gender|flight|origin|dest|date|dep|boarding|gate|code|bagTag|bagKg|checkedIn|bagStatus|bookedAt
    m_from->setText(fld(bp, 7));
    m_to->setText(fld(bp, 8));
    m_name->setText(fld(bp, 3).toUpper());
    m_sub->setText(QStringLiteral("Age %1  |  %2").arg(fld(bp, 4), fld(bp, 5)));
    m_flight->setText(fld(bp, 6));
    m_date->setText(fld(bp, 9));
    m_dep->setText(fld(bp, 10));
    m_boarding->setText(fld(bp, 11));
    m_gate->setText(fld(bp, 12));
    m_seat->setText(fld(bp, 2));
    m_pnr->setText(fld(bp, 1));
    m_code->setText(fld(bp, 13));
    const bool hasBag = fld(bp, 14) != QLatin1String("-");
    m_tag->setText(hasBag ? fld(bp, 14) : QStringLiteral("-"));
    m_bag->setText(hasBag ? fld(bp, 15) + QStringLiteral(" kg") : QStringLiteral("No bag"));

    const bool checked = (fld(bp, 16) == QLatin1String("1"));
    m_status->setText(checked ? QString::fromUtf8("\xE2\x9C\x93  CHECKED IN") : QStringLiteral("NOT CHECKED IN"));
    Theme::setRole(m_status, checked ? "chipOk" : "chip");

    const QString bs = fld(bp, 17);
    QString bagText;
    const char *bagRole = "chip";
    if (!hasBag) { bagText = QStringLiteral("No checked bag"); }
    else if (bs == QLatin1String("NONE")) { bagText = QStringLiteral("Bag: not on the belt yet"); }
    else if (bs == QLatin1String("ONBELT")) { bagText = QStringLiteral("Bag: on the belt"); }
    else if (bs == QLatin1String("DELIVERED")) { bagText = QStringLiteral("Bag: loaded"); bagRole = "chipOk"; }
    else if (bs == QLatin1String("LOST")) { bagText = QString::fromUtf8("\xE2\x9A\xA0  Bag: LOST on the belt"); bagRole = "chipWarn"; }
    else if (bs == QLatin1String("RECOVERED")) { bagText = QStringLiteral("Bag: recovered, safe"); bagRole = "chipOk"; }
    else { bagText = bs; }
    m_bagStatus->setText(bagText);
    Theme::setRole(m_bagStatus, bagRole);

    m_barcode->setText(fld(bp, 1) + fld(bp, 6) + fld(bp, 2) + fld(bp, 13));
}

// =====================================================================================
//  BoardingPassPage
// =====================================================================================
BoardingPassPage::BoardingPassPage(MainWindow *mw, QWidget *parent) : BasePage(mw, parent), m_pollCount(0)
{
    QHBoxLayout *root = new QHBoxLayout(this);
    root->setContentsMargins(28, 18, 28, 18);
    root->setSpacing(18);

    QVBoxLayout *left = new QVBoxLayout();
    left->addWidget(Theme::label(QStringLiteral("Your tickets"), "h2", this));
    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels(QStringList() << QStringLiteral("Date") << QStringLiteral("Flight")
                                       << QStringLiteral("Route") << QStringLiteral("Seat")
                                       << QStringLiteral("Passenger") << QStringLiteral("PNR") << QStringLiteral("Status"));
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(34);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    left->addWidget(m_table, 1);
    m_empty = Theme::label(QStringLiteral("You have no bookings yet. Use \"Book ticket\" to book your first flight."), "muted", this);
    m_empty->setWordWrap(true);
    m_empty->hide();
    left->addWidget(m_empty);
    root->addLayout(left, 5);

    QVBoxLayout *right = new QVBoxLayout();
    right->addWidget(Theme::label(QStringLiteral("Boarding pass"), "h2", this));
    m_card = new BoardingPassCard(this);
    m_card->hide();
    right->addWidget(m_card);
    m_save = Theme::button(QStringLiteral("Save boarding pass as image"), nullptr, this);
    m_save->hide();
    right->addWidget(m_save, 0, Qt::AlignLeft);
    right->addStretch(1);
    root->addLayout(right, 6);

    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]() { showSelected(); });
    connect(m_save, &QPushButton::clicked, this, &BoardingPassPage::saveImage);
}

void BoardingPassPage::showPnr(const QString &pnr)
{
    m_wantPnr = pnr;
    m_signature.clear();                       // force a rebuild so that the new booking is selected
}

void BoardingPassPage::onEnter()
{
    m_pollCount = 0;
    load();
}

void BoardingPassPage::onPoll()
{
    if (++m_pollCount % 3 == 0) load();        // check-in status changes show up by themselves
}

void BoardingPassPage::load()
{
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("MYPASSES"), app()->session().username});
    connect(r, &PendingReply::finished, this, [this](const Reply &rep) {
        if (!rep.ok) return;
        fill(rep.rowsOf(QStringLiteral("BP")));
    });
}

void BoardingPassPage::fill(const QVector<QStringList> &passes)
{
    QString sig;
    for (int i = 0; i < passes.size(); ++i) sig += passes.at(i).join(QLatin1Char('|')) + QLatin1Char('\n');
    if (sig == m_signature) return;            // nothing changed - do not disturb the user
    m_signature = sig;

    // remember what is selected now
    QString keepPnr = m_wantPnr, keepSeat;
    const int cur = m_table->currentRow();
    if (keepPnr.isEmpty() && cur >= 0 && cur < m_passes.size()) {
        keepPnr = fld(m_passes.at(cur), 1);
        keepSeat = fld(m_passes.at(cur), 2);
    }

    m_passes = passes;
    m_table->blockSignals(true);
    m_table->setRowCount(passes.size());
    int found = -1;
    for (int i = 0; i < passes.size(); ++i) {
        const QStringList &p = passes.at(i);
        const bool checked = (fld(p, 16) == QLatin1String("1"));
        QStringList cells;
        cells << fld(p, 9) + QStringLiteral("  ") + fld(p, 10) << fld(p, 6)
              << fld(p, 7) + QString::fromUtf8(" \xE2\x86\x92 ") + fld(p, 8) << fld(p, 2) << fld(p, 3) << fld(p, 1)
              << (checked ? QStringLiteral("Checked in") : QStringLiteral("Not checked in"));
        for (int c = 0; c < cells.size(); ++c) {
            QTableWidgetItem *item = new QTableWidgetItem(cells.at(c));
            if (c == 6) item->setForeground(QBrush(QColor(checked ? "#15803d" : "#b45309")));
            m_table->setItem(i, c, item);
        }
        if (found < 0 && !keepPnr.isEmpty() && fld(p, 1) == keepPnr && (keepSeat.isEmpty() || fld(p, 2) == keepSeat))
            found = i;
    }
    m_wantPnr.clear();
    const int select = (found >= 0) ? found : (passes.isEmpty() ? -1 : 0);
    if (select >= 0) m_table->selectRow(select);
    m_table->blockSignals(false);

    m_empty->setVisible(passes.isEmpty());
    showSelected();
}

void BoardingPassPage::showSelected()
{
    const int r = m_table->currentRow();
    if (r < 0 || r >= m_passes.size()) {
        m_card->hide();
        m_save->hide();
        return;
    }
    m_card->setPass(m_passes.at(r));
    m_card->show();
    m_save->show();
}

void BoardingPassPage::saveImage()
{
    const int r = m_table->currentRow();
    if (r < 0 || r >= m_passes.size()) return;
    const QString suggested = QStringLiteral("boarding_pass_%1_%2.png").arg(fld(m_passes.at(r), 1), fld(m_passes.at(r), 2));
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Save boarding pass"), suggested,
                                                      QStringLiteral("PNG image (*.png)"));
    if (path.isEmpty()) return;
    if (!m_card->grab().save(path))
        QMessageBox::warning(this, QStringLiteral("Save boarding pass"), QStringLiteral("The image could not be saved."));
}
