#pragma once

#include <QWidget>
#include <QString>
#include "beltdata.h"

class BeltWidget : public QWidget {
public:
    explicit BeltWidget(QWidget *parent = nullptr);

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
