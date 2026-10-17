#include "loginwindow.h"

#include "mainwindow.h"
#include "serverapi.h"
#include "theme.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginPage::LoginPage(MainWindow *mw, const QString &role, QWidget *parent)
    : BasePage(mw, parent), m_role(role), m_registerBtn(nullptr), m_adminBtn(nullptr)
{
    QString title, subtitle, icon, iconBg, iconFg;
    if (role == QLatin1String("PASSENGER")) {
        title = QStringLiteral("Passenger login");
        subtitle = QStringLiteral("Book flights, get boarding passes and track your luggage.");
        icon = QString::fromUtf8("\xE2\x9C\x88");                                         // ✈
        iconBg = QStringLiteral("#dbeafe"); iconFg = QStringLiteral("#2563eb");
    } else if (role == QLatin1String("STAFF")) {
        title = QStringLiteral("Staff login");
        subtitle = QStringLiteral("Check-in counter and baggage belt staff.");
        icon = QString::fromUtf8("\xE2\x9A\x99");                                         // ⚙
        iconBg = QStringLiteral("#ccfbf1"); iconFg = QStringLiteral("#0f766e");
    } else {
        title = QStringLiteral("Administrator login");
        subtitle = QStringLiteral("Administrators create the staff accounts.");
        icon = QString::fromUtf8("\xE2\x98\x85");                                         // ★
        iconBg = QStringLiteral("#fef3c7"); iconFg = QStringLiteral("#b45309");
    }
    m_title = title;

    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(30, 20, 30, 20);
    outer->addStretch(1);
    QHBoxLayout *mid = new QHBoxLayout();
    mid->addStretch(1);

    QFrame *card = Theme::card(this);
    card->setFixedWidth(440);
    QVBoxLayout *c = new QVBoxLayout(card);
    c->setContentsMargins(34, 30, 34, 30);
    c->setSpacing(8);

    QLabel *badge = new QLabel(icon, card);
    badge->setFixedSize(56, 56);
    badge->setAlignment(Qt::AlignCenter);
    badge->setStyleSheet(QStringLiteral("background: %1; color: %2; border-radius: 28px; font-size: 22pt;").arg(iconBg, iconFg));
    c->addWidget(badge, 0, Qt::AlignHCenter);
    c->addSpacing(6);

    QLabel *t = Theme::label(title, "h2", card);
    t->setAlignment(Qt::AlignCenter);
    c->addWidget(t);
    QLabel *s = Theme::label(subtitle, "muted", card);
    s->setAlignment(Qt::AlignCenter);
    s->setWordWrap(true);
    c->addWidget(s);
    c->addSpacing(14);

    c->addWidget(Theme::label(QStringLiteral("USERNAME"), "caption", card));
    m_user = new QLineEdit(card);
    m_user->setPlaceholderText(QStringLiteral("Enter your username"));
    c->addWidget(m_user);
    c->addSpacing(4);
    c->addWidget(Theme::label(QStringLiteral("PASSWORD"), "caption", card));
    m_pass = new QLineEdit(card);
    m_pass->setEchoMode(QLineEdit::Password);
    m_pass->setPlaceholderText(QStringLiteral("Enter your password"));
    c->addWidget(m_pass);

    m_msg = Theme::label(QString(), "error", card);
    m_msg->setWordWrap(true);
    m_msg->setAlignment(Qt::AlignCenter);
    m_msg->hide();
    c->addSpacing(4);
    c->addWidget(m_msg);

    m_loginBtn = Theme::button(QStringLiteral("Login"), "primary", card);
    m_loginBtn->setMinimumHeight(40);
    c->addSpacing(6);
    c->addWidget(m_loginBtn);

    if (role == QLatin1String("PASSENGER")) {
        QHBoxLayout *reg = new QHBoxLayout();
        reg->addStretch(1);
        reg->addWidget(Theme::label(QStringLiteral("New passenger?"), "muted", card));
        m_registerBtn = Theme::button(QStringLiteral("Create account"), "ghost", card);
        reg->addWidget(m_registerBtn);
        reg->addStretch(1);
        c->addLayout(reg);
        connect(m_registerBtn, &QPushButton::clicked, this, &LoginPage::doRegister);
    } else if (role == QLatin1String("STAFF")) {
        m_adminBtn = Theme::button(QStringLiteral("Admin - create staff accounts"), "ghost", card);
        c->addWidget(m_adminBtn);
        connect(m_adminBtn, &QPushButton::clicked, this, [this]() { app()->navigateTo(app()->adminLogin()); });
    }

    mid->addWidget(card);
    mid->addStretch(1);
    outer->addLayout(mid);
    outer->addStretch(2);

    connect(m_loginBtn, &QPushButton::clicked, this, &LoginPage::doLogin);
    connect(m_user, &QLineEdit::returnPressed, this, [this]() { m_pass->setFocus(); });
    connect(m_pass, &QLineEdit::returnPressed, this, &LoginPage::doLogin);
}

QString LoginPage::pageTitle() const
{
    return m_title;
}

void LoginPage::onEnter()
{
    m_pass->clear();
    m_msg->hide();
    m_user->setFocus();
}

void LoginPage::showMessage(const QString &text, bool good)
{
    if (text.isEmpty()) {
        m_msg->hide();
        return;
    }
    Theme::setRole(m_msg, good ? "ok" : "error");
    m_msg->setText(text);
    m_msg->show();
}

void LoginPage::setBusy(bool busy)
{
    m_loginBtn->setEnabled(!busy);
    if (m_registerBtn) m_registerBtn->setEnabled(!busy);
    m_loginBtn->setText(busy ? QStringLiteral("Please wait...") : QStringLiteral("Login"));
}

static QString connectionProblem()
{
    return QStringLiteral("Cannot reach the server at %1:%2.\nMake sure the server is running and the address on the first page is correct.")
        .arg(ServerApi::instance().host()).arg(ServerApi::instance().port());
}

void LoginPage::doLogin()
{
    const QString u = m_user->text().trimmed();
    const QString p = m_pass->text();
    if (u.isEmpty() || p.isEmpty()) {
        showMessage(QStringLiteral("Please enter both username and password."), false);
        return;
    }
    setBusy(true);
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("LOGIN"), m_role, u, p});
    connect(r, &PendingReply::finished, this, [this, u, p](const Reply &rep) {
        setBusy(false);
        if (!rep.netOk) { showMessage(connectionProblem(), false); return; }
        if (!rep.ok) { showMessage(rep.message, false); return; }
        showMessage(QString(), true);
        m_pass->clear();
        app()->onLoggedIn(m_role, u, p, rep.headField(2));
    });
}

void LoginPage::doRegister()
{
    const QString u = m_user->text().trimmed();
    const QString p = m_pass->text();
    if (u.isEmpty() || p.isEmpty()) {
        showMessage(QStringLiteral("Choose a username and a password, then press Create account."), false);
        return;
    }
    setBusy(true);
    PendingReply *r = ServerApi::instance().send(QStringList{QStringLiteral("REGISTER"), u, p});
    connect(r, &PendingReply::finished, this, [this](const Reply &rep) {
        setBusy(false);
        if (!rep.netOk) { showMessage(connectionProblem(), false); return; }
        if (!rep.ok) { showMessage(rep.message, false); return; }
        showMessage(QStringLiteral("Account created. You can log in now."), true);
        m_pass->clear();
    });
}
