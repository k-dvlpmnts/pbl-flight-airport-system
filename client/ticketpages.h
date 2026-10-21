#pragma once
// Boarding passes of the logged in passenger.
#include "basepage.h"
#include <QFrame>
#include <QStringList>
#include <QVector>

class QLabel;
class QPushButton;
class QTableWidget;
class QGridLayout;

// pseudo barcode drawn from the booking details
class BarcodeWidget : public QWidget {
public:
    explicit BarcodeWidget(QWidget *parent = nullptr);
    void setText(const QString &text);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QString m_text;
};

// the printed-looking pass
class BoardingPassCard : public QFrame {
public:
    explicit BoardingPassCard(QWidget *parent = nullptr);
    void setPass(const QStringList &bp);      // one "BP" row sent by the server

private:
    QLabel *addField(QGridLayout *grid, int row, int col, const QString &caption);

    QLabel *m_from, *m_to, *m_name, *m_sub, *m_status, *m_bagStatus;
    QLabel *m_flight, *m_date, *m_dep, *m_boarding, *m_gate, *m_seat;
    QLabel *m_pnr, *m_code, *m_tag, *m_bag;
    BarcodeWidget *m_barcode;
};

class BoardingPassPage : public BasePage {
    Q_OBJECT
public:
    explicit BoardingPassPage(MainWindow *mw, QWidget *parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("My boarding passes"); }
    void onEnter() override;
    void onPoll() override;
    void showPnr(const QString &pnr);          // select the passes of a booking (after booking)

private slots:
    void saveImage();

private:
    void load();
    void fill(const QVector<QStringList> &passes);
    void showSelected();

    QTableWidget *m_table;
    BoardingPassCard *m_card;
    QLabel *m_empty;
    QPushButton *m_save;
    QVector<QStringList> m_passes;
    QString m_wantPnr;
    QString m_signature;
    int m_pollCount;
};
