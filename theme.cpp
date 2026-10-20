#include "theme.h"

#include <QApplication>
#include <QColor>
#include <QFrame>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QFont>

namespace Theme {

static const char *kStyleSheet = R"CSS(
* { font-family: "Segoe UI", "Helvetica Neue", "Noto Sans", Arial, sans-serif; font-size: 10pt; color: #0f172a; }
QMainWindow, QDialog { background: #f3f6fb; }
QLabel { background: transparent; }
QMessageBox { background: #ffffff; }

QFrame#card { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 14px; }
QFrame#topBar { background: #ffffff; border: none; border-bottom: 1px solid #e2e8f0; }
QFrame#alertBox { background: #fef2f2; border: 1px solid #fecaca; border-radius: 12px; }
QFrame#okBox { background: #f0fdf4; border: 1px solid #bbf7d0; border-radius: 12px; }
QFrame#infoBox { background: #eff6ff; border: 1px solid #bfdbfe; border-radius: 12px; }

QLabel[role="h1"] { font-size: 24pt; font-weight: 700; color: #0f172a; }
QLabel[role="h2"] { font-size: 14pt; font-weight: 600; color: #0f172a; }
QLabel[role="muted"] { color: #64748b; }
QLabel[role="caption"] { color: #64748b; font-size: 8pt; font-weight: 600; }
QLabel[role="ok"] { color: #15803d; font-weight: 600; }
QLabel[role="error"] { color: #b91c1c; font-weight: 600; }
QLabel[role="chip"] { background: #e0f2fe; color: #0369a1; border-radius: 10px; padding: 3px 10px; font-weight: 600; }
QLabel[role="chipWarn"] { background: #fee2e2; color: #b91c1c; border-radius: 10px; padding: 3px 10px; font-weight: 600; }
QLabel[role="chipSim"] { background: #fef3c7; color: #b45309; border-radius: 10px; padding: 3px 10px; font-weight: 600; }
QLabel[role="chipOk"] { background: #dcfce7; color: #15803d; border-radius: 10px; padding: 3px 10px; font-weight: 600; }

QPushButton { background: #ffffff; border: 1px solid #cbd5e1; border-radius: 9px; padding: 8px 18px; min-height: 20px; }
QPushButton:hover { background: #f1f5f9; border-color: #94a3b8; }
QPushButton:pressed { background: #e2e8f0; }
QPushButton:disabled { color: #94a3b8; background: #f1f5f9; border-color: #e2e8f0; }
QPushButton[role="primary"] { background: #2563eb; color: #ffffff; border: 1px solid #2563eb; font-weight: 600; }
QPushButton[role="primary"]:hover { background: #1d4ed8; border-color: #1d4ed8; }
QPushButton[role="primary"]:pressed { background: #1e40af; }
QPushButton[role="primary"]:disabled { background: #bfdbfe; border-color: #bfdbfe; color: #ffffff; }
QPushButton[role="danger"] { background: #ffffff; color: #b91c1c; border: 1px solid #fca5a5; font-weight: 600; }
QPushButton[role="danger"]:hover { background: #fef2f2; }
QPushButton[role="ghost"] { background: transparent; border: 1px solid transparent; color: #2563eb; font-weight: 600; }
QPushButton[role="ghost"]:hover { background: #eff6ff; }
QPushButton[role="nav"] { background: #f1f5f9; border: 1px solid #e2e8f0; color: #0f172a; font-weight: 600; padding: 7px 16px; }
QPushButton[role="nav"]:hover { background: #e2e8f0; }

QLineEdit, QComboBox, QDateEdit, QDateTimeEdit, QSpinBox, QDoubleSpinBox {
    background: #ffffff; border: 1px solid #cbd5e1; border-radius: 9px; padding: 6px 10px; min-height: 22px;
    selection-background-color: #bfdbfe; selection-color: #0f172a;
}
QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QDateTimeEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus { border: 1px solid #2563eb; }
QLineEdit:disabled, QComboBox:disabled { background: #f1f5f9; color: #94a3b8; }
QComboBox QAbstractItemView { background: #ffffff; border: 1px solid #cbd5e1; selection-background-color: #dbeafe; selection-color: #0f172a; outline: none; }

QTableView, QTableWidget {
    background: #ffffff; alternate-background-color: #f8fafc; border: 1px solid #e2e8f0; border-radius: 10px;
    gridline-color: #eef2f7; selection-background-color: #dbeafe; selection-color: #0f172a;
}
QTableView::item { padding: 4px 6px; }
QHeaderView::section { background: #f1f5f9; color: #475569; border: none; border-bottom: 1px solid #e2e8f0; padding: 8px 8px; font-weight: 600; }
QTableCornerButton::section { background: #f1f5f9; border: none; }

QPlainTextEdit { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 10px; padding: 6px; font-family: Consolas, "Courier New", monospace; }
QScrollArea { border: none; background: transparent; }
QScrollBar:vertical { background: transparent; width: 12px; margin: 2px; }
QScrollBar::handle:vertical { background: #cbd5e1; border-radius: 5px; min-height: 30px; }
QScrollBar::handle:vertical:hover { background: #94a3b8; }
QScrollBar:horizontal { background: transparent; height: 12px; margin: 2px; }
QScrollBar::handle:horizontal { background: #cbd5e1; border-radius: 5px; min-width: 30px; }
QScrollBar::handle:horizontal:hover { background: #94a3b8; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0px; height: 0px; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
QToolTip { background: #0f172a; color: #ffffff; border: none; padding: 4px 8px; }
)CSS";

void apply(QApplication &app)
{
    app.setStyle(QStringLiteral("Fusion"));

    QPalette p;
    p.setColor(QPalette::Window, QColor("#f3f6fb"));
    p.setColor(QPalette::WindowText, QColor("#0f172a"));
    p.setColor(QPalette::Base, QColor("#ffffff"));
    p.setColor(QPalette::AlternateBase, QColor("#f8fafc"));
    p.setColor(QPalette::Text, QColor("#0f172a"));
    p.setColor(QPalette::Button, QColor("#ffffff"));
    p.setColor(QPalette::ButtonText, QColor("#0f172a"));
    p.setColor(QPalette::ToolTipBase, QColor("#0f172a"));
    p.setColor(QPalette::ToolTipText, QColor("#ffffff"));
    p.setColor(QPalette::Highlight, QColor("#2563eb"));
    p.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    p.setColor(QPalette::PlaceholderText, QColor("#94a3b8"));
    app.setPalette(p);

    QFont f = app.font();
    f.setPointSize(10);
    app.setFont(f);
    app.setStyleSheet(QString::fromUtf8(kStyleSheet));
}

QFrame *card(QWidget *parent)
{
    QFrame *f = new QFrame(parent);
    f->setObjectName(QStringLiteral("card"));
    return f;
}

void setRole(QWidget *w, const char *role)
{
    w->setProperty("role", QString::fromLatin1(role ? role : ""));
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

QLabel *label(const QString &text, const char *role, QWidget *parent)
{
    QLabel *l = new QLabel(text, parent);
    if (role) l->setProperty("role", QString::fromLatin1(role));
    return l;
}

QPushButton *button(const QString &text, const char *role, QWidget *parent)
{
    QPushButton *b = new QPushButton(text, parent);
    if (role) b->setProperty("role", QString::fromLatin1(role));
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

void transparentScroll(QScrollArea *area)
{
    area->setFrameShape(QFrame::NoFrame);
    area->viewport()->setAutoFillBackground(false);
    if (area->widget()) area->widget()->setAutoFillBackground(false);
}

QString money(double amount)
{
    QString digits = QString::number(static_cast<qlonglong>(amount + 0.5));
    QString out;
    int count = 0;
    for (int i = digits.size() - 1; i >= 0; --i) {
        out.prepend(digits.at(i));
        if (digits.at(i).isDigit() && ++count % 3 == 0 && i > 0) out.prepend(QLatin1Char(','));
    }
    return QString::fromUtf8("\xE2\x82\xB9") + out;          // ₹ (U+20B9)
}

QString duration(int minutes)
{
    const int h = minutes / 60, m = minutes % 60;
    if (h == 0) return QStringLiteral("%1m").arg(m);
    return QStringLiteral("%1h %2m").arg(h).arg(m, 2, 10, QLatin1Char('0'));
}

} // namespace Theme
