#include "seatwidget.h"
#include <QGridLayout>
#include <QToolButton>
#include <QLayoutItem>
#include <QStyle>
#include <QLabel>

SeatWidget::SeatWidget(QWidget *parent) : QWidget(parent) {
    grid = new QGridLayout(this);
    grid->setSpacing(6);
    grid->setContentsMargins(6,6,6,6);
}

void SeatWidget::clearLayout() {
    // Remove widgets from layout and delete them safely
    while (QLayoutItem *item = grid->takeAt(0)) {
        QWidget *w = item->widget();
        if (w) {
            grid->removeWidget(w);
            delete w;
        }
        delete item;
    }
    buttons.clear(); // buttons were deleted above, so just clear the list
}

void SeatWidget::populate(const QVector<bool> &sold) {
    clearLayout();
    const int cols = 6;
    for (int i = 0; i < sold.size(); ++i) {
        // Parent the button to this widget so ownership is clear
        QToolButton *b = new QToolButton(this);
        b->setText(QString::number(i+1));
        b->setCheckable(true);
        b->setMinimumSize(48, 36);
        if (sold[i]) {
            b->setEnabled(false);
            b->setText("Sold");
            b->setStyleSheet("background: #d3d3d3; color: #777;");
        } else {
            b->setStyleSheet("background: #444; color: white;");
            connect(b, &QToolButton::toggled, this, [this, i](bool on){
                emit seatToggled(i, on);
            });
        }
        grid->addWidget(b, i / cols, i % cols);
        buttons.append(b);
    }
}
