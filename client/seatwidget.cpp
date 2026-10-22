#include "seatwidget.h"

#include <QGridLayout>
#include <QToolButton>
#include <QLayoutItem>
#include <QLabel>

SeatWidget::SeatWidget(QWidget *parent) : QWidget(parent), m_interactive(true)
{
    m_grid = new QGridLayout(this);
    m_grid->setHorizontalSpacing(6);
    m_grid->setVerticalSpacing(6);
    m_grid->setContentsMargins(8, 8, 8, 8);
    setStyleSheet(QStringLiteral(
        "QToolButton { background: #e0f2fe; border: 1px solid #7dd3fc; border-radius: 8px; font-weight: 600; color: #075985; }"
        "QToolButton:hover { background: #bae6fd; }"
        "QToolButton:checked { background: #2563eb; border: 1px solid #1d4ed8; color: #ffffff; }"
        "QToolButton:disabled { background: #e5e7eb; border: 1px solid #d1d5db; color: #9ca3af; }"));
}

QString SeatWidget::labelFor(int index)
{
    static const char letters[] = "ABCDEF";
    return QString::number(index / 6 + 1) + QLatin1Char(letters[index % 6]);
}

void SeatWidget::clearLayout()
{
    while (QLayoutItem *item = m_grid->takeAt(0)) {
        if (QWidget *w = item->widget()) delete w;
        delete item;
    }
    m_buttons.clear();
    m_sold.clear();
}

void SeatWidget::setSeatCount(int total)
{
    if (total == m_buttons.size()) return;
    rebuild(total);
}

void SeatWidget::rebuild(int total)
{
    clearLayout();
    if (total <= 0) return;

    QLabel *nose = new QLabel(QString::fromUtf8("\xE2\x96\xB2  FRONT OF AIRCRAFT"), this);   // ▲
    nose->setAlignment(Qt::AlignCenter);
    nose->setStyleSheet(QStringLiteral("color: #64748b; font-weight: 600; font-size: 8pt;"));
    m_grid->addWidget(nose, 0, 0, 1, 7);

    static const char letters[] = "ABCDEF";
    for (int l = 0; l < 6; ++l) {
        QLabel *h = new QLabel(QString(QLatin1Char(letters[l])), this);
        h->setAlignment(Qt::AlignCenter);
        h->setStyleSheet(QStringLiteral("color: #64748b; font-weight: 700;"));
        m_grid->addWidget(h, 1, l < 3 ? l : l + 1);
    }

    const int rows = (total + 5) / 6;
    for (int r = 0; r < rows; ++r) {
        QLabel *rowNo = new QLabel(QString::number(r + 1), this);
        rowNo->setAlignment(Qt::AlignCenter);
        rowNo->setMinimumWidth(28);
        rowNo->setStyleSheet(QStringLiteral("color: #94a3b8; font-size: 8pt;"));
        m_grid->addWidget(rowNo, r + 2, 3);
    }

    m_sold.fill(false, total);
    for (int i = 0; i < total; ++i) {
        QToolButton *b = new QToolButton(this);
        b->setText(labelFor(i));
        b->setCheckable(true);
        b->setMinimumSize(46, 38);
        b->setCursor(Qt::PointingHandCursor);
        b->setToolTip(QStringLiteral("Seat ") + labelFor(i));
        connect(b, &QToolButton::toggled, this, [this, i](bool on) {
            emit seatToggled(i, on);
            emit selectionChanged();
        });
        const int letter = i % 6;
        m_grid->addWidget(b, i / 6 + 2, letter < 3 ? letter : letter + 1);
        m_buttons.append(b);
    }

    QLabel *legend = new QLabel(QString::fromUtf8(
        "<span style='color:#38bdf8'>\xE2\x96\xA0</span> Available &nbsp;&nbsp; "
        "<span style='color:#2563eb'>\xE2\x96\xA0</span> Selected &nbsp;&nbsp; "
        "<span style='color:#9ca3af'>\xE2\x96\xA0</span> Sold"), this);
    legend->setTextFormat(Qt::RichText);
    legend->setAlignment(Qt::AlignCenter);
    legend->setStyleSheet(QStringLiteral("color: #475569; font-size: 8pt;"));
    m_grid->addWidget(legend, rows + 2, 0, 1, 7);
    applyStates();
}

void SeatWidget::applyStates()
{
    for (int i = 0; i < m_buttons.size(); ++i) {
        QToolButton *b = m_buttons.at(i);
        if (m_sold.value(i)) {
            b->blockSignals(true);
            b->setChecked(false);
            b->blockSignals(false);
            b->setEnabled(false);
            b->setText(QString::fromUtf8("\xE2\x9C\x95"));                                    
            b->setToolTip(QStringLiteral("Seat ") + labelFor(i) + QStringLiteral(" - sold"));
        } else {
            b->setEnabled(m_interactive);
            b->setText(labelFor(i));
            b->setToolTip(QStringLiteral("Seat ") + labelFor(i));
        }
    }
}

void SeatWidget::setSold(const QStringList &soldLabels)
{
    bool changedSelection = false;
    for (int i = 0; i < m_buttons.size(); ++i) {
        const bool nowSold = soldLabels.contains(labelFor(i));
        if (nowSold && m_buttons.at(i)->isChecked()) {
            emit seatTaken(labelFor(i));
            changedSelection = true;
        }
        m_sold[i] = nowSold;
    }
    applyStates();
    if (changedSelection) emit selectionChanged();
}

void SeatWidget::setInteractive(bool on)
{
    m_interactive = on;
    applyStates();
}

QStringList SeatWidget::selectedLabels() const
{
    QStringList out;
    for (int i = 0; i < m_buttons.size(); ++i)
        if (m_buttons.at(i)->isChecked()) out.append(labelFor(i));
    return out;
}

void SeatWidget::clearSelection()
{
    bool any = false;
    for (int i = 0; i < m_buttons.size(); ++i) {
        if (!m_buttons.at(i)->isChecked()) continue;
        m_buttons.at(i)->blockSignals(true);
        m_buttons.at(i)->setChecked(false);
        m_buttons.at(i)->blockSignals(false);
        any = true;
    }
    if (any) emit selectionChanged();
}
