#pragma once
// Light, friendly look shared by every screen.
#include <QString>

class QApplication;
class QWidget;
class QLabel;
class QFrame;
class QPushButton;
class QScrollArea;

namespace Theme {

void apply(QApplication &app);                       // Fusion style + light palette + style sheet

QFrame *card(QWidget *parent = nullptr);             // white rounded panel
// role: "h1", "h2", "muted", "caption", "ok", "error", "chip"
QLabel *label(const QString &text, const char *role = nullptr, QWidget *parent = nullptr);
// role: "primary", "danger", "ghost" or nullptr for the default outline button
QPushButton *button(const QString &text, const char *role = nullptr, QWidget *parent = nullptr);
void setRole(QWidget *w, const char *role);          // changes the look of an existing widget
void transparentScroll(QScrollArea *area);           // scroll area that blends into its parent

QString money(double amount);                        // "₹4,500"
QString duration(int minutes);                       // "2h 10m"

} // namespace Theme
