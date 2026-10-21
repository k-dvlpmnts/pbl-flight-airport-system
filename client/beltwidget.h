#pragma once
// Draws the baggage belt: RFID / photon sensors, the bags travelling on it and, underneath,
// the doubly linked list of bag nodes (HEAD <-> bag <-> bag <-> ... <-> NULL).
#include <QWidget>
#include <QString>
#include "beltdata.h"

class BeltWidget : public QWidget {
public:
    explicit BeltWidget(QWidget *parent = nullptr);

    // staffView = true : every bag.   false : only the passenger's own bags (already filtered by the server)
    void setSnapshot(const BeltSnapshot &snap, bool staffView);
    void setSelectedTag(const QString &tag);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int neededWidth() const;

    BeltSnapshot m_snap;
    bool m_staff;
    QString m_selected;
};
