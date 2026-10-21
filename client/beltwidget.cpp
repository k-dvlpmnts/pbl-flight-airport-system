#include "beltwidget.h"

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>
#include <QPolygonF>

namespace {

const double kNodeW = 168.0;
const double kNodeH = 96.0;
const double kGap = 58.0;
const double kPillW = 58.0;
const double kMargin = 20.0;
const double kHeight = 346.0;

QColor stateColor(const QString &state)
{
    if (state == QLatin1String("DELIVERED")) return QColor("#22c55e");
    if (state == QLatin1String("LOST")) return QColor("#ef4444");
    if (state == QLatin1String("RECOVERED")) return QColor("#f59e0b");
    return QColor("#3b82f6");                                  // ONBELT
}

QString initials(const QString &name)
{
    QString out;
    const QStringList parts = name.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (int i = 0; i < parts.size() && out.size() < 2; ++i) out += parts.at(i).at(0).toUpper();
    return out.isEmpty() ? QStringLiteral("?") : out;
}

void drawSuitcase(QPainter &p, const QPointF &c, const QColor &color, const QString &text, bool selected)
{
    p.save();
    if (selected) {
        p.setPen(QPen(QColor("#facc15"), 3));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(c.x() - 21, c.y() - 19, 42, 34), 9, 9);
    }
    p.setPen(QPen(color.darker(130), 2));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(c.x() - 6, c.y() - 15, 12, 7), 2, 2);          // handle
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawRoundedRect(QRectF(c.x() - 15, c.y() - 10, 30, 22), 5, 5);        // body
    QFont f = p.font();
    f.setPointSize(7);
    f.setBold(true);
    p.setFont(f);
    p.setPen(QColor("#ffffff"));
    p.drawText(QRectF(c.x() - 15, c.y() - 10, 30, 22), int(Qt::AlignCenter), text);
    p.restore();
}

void drawArrow(QPainter &p, double x1, double x2, double y, const QColor &color)
{
    const double dir = (x2 >= x1) ? 1.0 : -1.0;
    p.setPen(QPen(color, 1.6));
    p.drawLine(QPointF(x1, y), QPointF(x2, y));
    QPolygonF head;
    head << QPointF(x2, y) << QPointF(x2 - dir * 8, y - 4.5) << QPointF(x2 - dir * 8, y + 4.5);
    p.setBrush(color);
    p.drawPolygon(head);
}

void drawPill(QPainter &p, const QRectF &r, const QString &text, const QColor &bg, const QColor &fg)
{
    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(r, r.height() / 2, r.height() / 2);
    QFont f = p.font();
    f.setBold(true);
    f.setPointSize(8);
    p.setFont(f);
    p.setPen(fg);
    p.drawText(r, int(Qt::AlignCenter), text);
    p.restore();
}

QString stateText(const BagItem &b)
{
    if (b.state == QLatin1String("DELIVERED")) return QStringLiteral("Delivered");
    if (b.state == QLatin1String("LOST")) return QStringLiteral("LOST at sensor %1").arg(b.lostAt);
    if (b.state == QLatin1String("RECOVERED")) return QStringLiteral("Recovered - safe");
    return QStringLiteral("On belt (sensor %1)").arg(b.passed);
}

} // namespace

BeltWidget::BeltWidget(QWidget *parent) : QWidget(parent), m_staff(true)
{
    setMinimumHeight(static_cast<int>(kHeight));
}

int BeltWidget::neededWidth() const
{
    const double n = m_snap.bags.size();
    double w = kMargin * 2 + kPillW * 2 + (n + 1) * kGap + n * kNodeW;
    if (w < 940) w = 940;
    return static_cast<int>(w);
}

QSize BeltWidget::sizeHint() const
{
    return QSize(neededWidth(), static_cast<int>(kHeight));
}

void BeltWidget::setSnapshot(const BeltSnapshot &snap, bool staffView)
{
    m_snap = snap;
    m_staff = staffView;
    setMinimumWidth(neededWidth());
    updateGeometry();
    update();
}

void BeltWidget::setSelectedTag(const QString &tag)
{
    m_selected = tag;
    update();
}

void BeltWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QFont base = font();
    base.setPointSize(9);
    QFont bold = base;
    bold.setBold(true);
    QFont small = base;
    small.setPointSize(8);

    const double W = width();
    const double trackLeft = 90.0;
    double trackRight = W - 90.0;
    if (trackRight > 1100.0) trackRight = 1100.0;
    if (trackRight < trackLeft + 300.0) trackRight = trackLeft + 300.0;
    const double trackW = trackRight - trackLeft;
    const double trackTop = 92.0;
    const double trackH = 34.0;
    const double length = m_snap.length > 0 ? m_snap.length : 25.0;

    // ------------------------------------------------------------------ heading
    p.setFont(bold);
    p.setPen(QColor("#0f172a"));
    p.drawText(QPointF(kMargin, 24), QString::fromUtf8("Conveyor belt   \xE2\x86\x92   bags travel from the start RFID to the end RFID"));
    if (!m_snap.valid) {
        p.setFont(base);
        p.setPen(QColor("#64748b"));
        p.drawText(QRectF(0, 100, W, 60), int(Qt::AlignCenter), QStringLiteral("Waiting for data from the server..."));
        return;
    }

    // ------------------------------------------------------------------ belt body
    p.setPen(QPen(QColor("#94a3b8"), 1.5));
    p.setBrush(QColor("#e2e8f0"));
    p.drawRoundedRect(QRectF(trackLeft - 26, trackTop, trackW + 52, trackH), 17, 17);
    p.setPen(QPen(QColor("#cbd5e1"), 1));
    for (double x = trackLeft - 12; x < trackRight + 12; x += 16)
        p.drawLine(QPointF(x, trackTop + 6), QPointF(x, trackTop + trackH - 6));

    // ------------------------------------------------------------------ sensors
    for (int i = 0; i < m_snap.sensors.size(); ++i) {
        const SensorInfo &s = m_snap.sensors.at(i);
        const double x = trackLeft + s.distance / length * trackW;
        const bool rfid = (s.kind == QLatin1String("RFID"));
        const QColor c = rfid ? QColor("#6366f1") : QColor("#f59e0b");

        bool missed = false;
        for (int b = 0; b < m_snap.bags.size(); ++b)
            if ((m_snap.bags.at(b).isLost() || m_snap.bags.at(b).state == QLatin1String("RECOVERED"))
                && m_snap.bags.at(b).lostAt == s.index) missed = true;

        p.setPen(QPen(c, 2, Qt::DashLine));
        p.drawLine(QPointF(x, trackTop - 12), QPointF(x, trackTop + trackH + 6));
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        if (rfid) p.drawRoundedRect(QRectF(x - 15, trackTop - 40, 30, 26), 7, 7);
        else p.drawEllipse(QPointF(x, trackTop - 27), 13, 13);
        p.setFont(small);
        p.setPen(QColor("#ffffff"));
        p.drawText(QRectF(x - 15, trackTop - 40, 30, 26), int(Qt::AlignCenter), rfid ? QStringLiteral("RF") : QString::number(s.index));
        if (missed) {
            p.setPen(QPen(QColor("#ef4444"), 3));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QPointF(x, trackTop - 27), 19, 19);
        }
        p.setPen(QColor("#475569"));
        p.setFont(small);
        p.drawText(QRectF(x - 60, trackTop + trackH + 8, 120, 16), int(Qt::AlignCenter), s.name);
    }

    // ------------------------------------------------------------------ bags on / beside the belt
    int delivered = 0;
    for (int i = 0; i < m_snap.bags.size(); ++i) {
        const BagItem &b = m_snap.bags.at(i);
        const bool sel = (b.tag == m_selected);
        if (b.state == QLatin1String("DELIVERED")) { ++delivered; continue; }
        if (b.pos < 0) continue;                                  // fell off, not noticed by the system yet
        const double x = trackLeft + b.pos / length * trackW;
        if (b.state == QLatin1String("ONBELT")) {
            drawSuitcase(p, QPointF(x, trackTop + trackH / 2 + 1), stateColor(b.state), initials(b.name), sel);
        } else {                                                  // LOST / RECOVERED: lying beside the belt
            const QPointF c(x, trackTop + trackH + 58);
            drawSuitcase(p, c, stateColor(b.state), b.state == QLatin1String("LOST") ? QString::fromUtf8("\xE2\x9C\x95") : QString::fromUtf8("\xE2\x9C\x93"), sel);
        }
    }
    p.setFont(small);
    p.setPen(QColor("#475569"));
    p.drawText(QRectF(trackRight + 30, trackTop - 2, 80, trackH + 4), int(Qt::AlignCenter),
               QStringLiteral("%1\nloaded").arg(delivered));

    // ------------------------------------------------------------------ doubly linked list
    const double nodesTop = 226.0;
    p.setFont(bold);
    p.setPen(QColor("#0f172a"));
    p.drawText(QPointF(kMargin, nodesTop - 16),
               m_staff ? QStringLiteral("Doubly linked list of bags - in belt order (a node is added when a bag crosses the start RFID)")
                       : QStringLiteral("Your bags in the belt's doubly linked list"));

    const double midY = nodesTop + kNodeH / 2;
    double x = kMargin;
    drawPill(p, QRectF(x, midY - 14, kPillW, 28), QStringLiteral("HEAD"), QColor("#e2e8f0"), QColor("#475569"));
    x += kPillW;

    if (m_snap.bags.isEmpty()) {
        drawArrow(p, x + 4, x + kGap - 4, midY, QColor("#64748b"));
        x += kGap;
        drawPill(p, QRectF(x, midY - 14, kPillW, 28), QStringLiteral("NULL"), QColor("#e2e8f0"), QColor("#475569"));
        p.setFont(base);
        p.setPen(QColor("#64748b"));
        p.drawText(QRectF(x + kPillW + 20, midY - 14, 520, 28), int(Qt::AlignLeft | Qt::AlignVCenter),
                   m_staff ? QStringLiteral("No bag has crossed the start RFID yet.")
                           : QStringLiteral("None of your bags is on the belt yet."));
        return;
    }

    for (int i = 0; i < m_snap.bags.size(); ++i) {
        const BagItem &b = m_snap.bags.at(i);
        const QColor pointer("#64748b");
        if (i == 0) {
            drawArrow(p, x + 4, x + kGap - 4, midY, pointer);                      // HEAD -> node
        } else {
            drawArrow(p, x + 4, x + kGap - 4, midY - 8, pointer);                  // next
            drawArrow(p, x + kGap - 4, x + 4, midY + 8, pointer);                  // prev
        }
        x += kGap;

        const QRectF r(x, nodesTop, kNodeW, kNodeH);
        const QColor c = stateColor(b.state);
        const bool sel = (b.tag == m_selected);
        p.setPen(QPen(c, sel ? 3.5 : 2));
        p.setBrush(QColor("#ffffff"));
        p.drawRoundedRect(r, 10, 10);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(QRectF(r.left(), r.top(), 7, r.height()), 3, 3);

        p.setFont(bold);
        p.setPen(QColor("#0f172a"));
        p.drawText(QPointF(r.left() + 16, r.top() + 22), b.tag);
        p.setFont(base);
        QFontMetrics fm(base);
        p.setPen(QColor("#334155"));
        p.drawText(QPointF(r.left() + 16, r.top() + 42), fm.elidedText(b.name, Qt::ElideRight, static_cast<int>(kNodeW - 28)));
        p.setPen(QColor("#64748b"));
        p.drawText(QPointF(r.left() + 16, r.top() + 60), QStringLiteral("%1  |  seat %2").arg(b.flight, b.seat));
        p.setFont(bold);
        p.setPen(c.darker(115));
        p.drawText(QPointF(r.left() + 16, r.top() + 82), fm.elidedText(stateText(b), Qt::ElideRight, static_cast<int>(kNodeW - 28)));
        x += kNodeW;
    }
    drawArrow(p, x + 4, x + kGap - 4, midY, QColor("#64748b"));                    // last node -> NULL
    x += kGap;
    drawPill(p, QRectF(x, midY - 14, kPillW, 28), QStringLiteral("NULL"), QColor("#e2e8f0"), QColor("#475569"));
}
