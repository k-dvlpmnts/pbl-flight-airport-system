#pragma once
// Luggage tracking: the passenger sees only his own bags, the belt staff sees all bags.
#include "basepage.h"
#include "beltdata.h"

class QFrame;
class QLabel;
class QPushButton;
class QTableWidget;
class BeltWidget;

class TrackLuggagePage : public BasePage {
    Q_OBJECT
public:
    explicit TrackLuggagePage(MainWindow *mw, QWidget *parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("Track luggage"); }
    void onEnter() override;

private:
    void render(const BeltSnapshot &snap);

    QFrame *m_alertBox;
    QLabel *m_alertText;
    BeltWidget *m_belt;
    QTableWidget *m_table;
    QLabel *m_empty;
};

class BeltStaffPage : public BasePage {
    Q_OBJECT
public:
    explicit BeltStaffPage(MainWindow *mw, QWidget *parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("Baggage belt - staff"); }
    void onEnter() override;

private slots:
    void markSafe();

private:
    void render(const BeltSnapshot &snap);
    void showDetail();

    QFrame *m_alertBox;
    QLabel *m_alertText;
    BeltWidget *m_belt;
    QTableWidget *m_table;
    QLabel *m_detail;
    QLabel *m_msg;
    QPushButton *m_safeBtn;
    BeltSnapshot m_snap;
    QString m_selectedTag;
    QString m_signature;
};
