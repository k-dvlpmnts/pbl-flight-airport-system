#include "serverapi.h"

#include <QTcpSocket>
#include <QTimer>

// ------------------------------------------------------------------ PendingReply
PendingReply::PendingReply(const QString &host, quint16 port, const QByteArray &request, QObject *parent)
    : QObject(parent), m_request(request), m_done(false)
{
    m_sock = new QTcpSocket(this);
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);

    connect(m_sock, &QTcpSocket::connected, this, &PendingReply::onConnected);
    connect(m_sock, &QTcpSocket::readyRead, this, &PendingReply::onReadyRead);
    connect(m_sock, &QTcpSocket::disconnected, this, &PendingReply::onDisconnected);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_sock, &QAbstractSocket::errorOccurred, this, &PendingReply::onError);
#else
    connect(m_sock, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &PendingReply::onError);
#endif
    connect(m_timer, &QTimer::timeout, this, &PendingReply::onTimeout);

    m_timer->start(6000);
    m_sock->connectToHost(host, port);
}

void PendingReply::onConnected()
{
    m_sock->write(m_request);
    m_sock->flush();
}

void PendingReply::onReadyRead()
{
    m_buffer.append(m_sock->readAll());
}

void PendingReply::onDisconnected()
{
    m_buffer.append(m_sock->readAll());
    if (m_buffer.isEmpty())
        finish(false, tr("The server closed the connection without answering."));
    else
        finish(true, QString());
}

void PendingReply::onError(QAbstractSocket::SocketError error)
{
    // a normal close by the server is reported as RemoteHostClosedError; the
    // answer is then processed in onDisconnected()
    if (error == QAbstractSocket::RemoteHostClosedError) return;
    finish(false, m_sock->errorString());
}

void PendingReply::onTimeout()
{
    finish(false, tr("The server did not answer in time."));
}

void PendingReply::finish(bool netOk, const QString &errorText)
{
    if (m_done) return;
    m_done = true;
    m_timer->stop();

    Reply r;
    r.netOk = netOk;
    if (!netOk) {
        r.code = QStringLiteral("network");
        r.message = errorText;
    } else {
        const QString text = QString::fromUtf8(m_buffer);
        const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (int i = 0; i < lines.size(); ++i) {
            QString line = lines.at(i);
            if (line.endsWith(QLatin1Char('\r'))) line.chop(1);
            r.rows.append(line.split(QLatin1Char('|'), Qt::KeepEmptyParts));
        }
        if (r.rows.isEmpty()) {
            r.netOk = false;
            r.code = QStringLiteral("network");
            r.message = tr("Empty answer from the server.");
        } else if (r.rows.first().value(0) == QLatin1String("OK")) {
            r.ok = true;
        } else {
            r.code = r.rows.first().value(1);
            r.message = r.rows.first().value(2);
            if (r.message.isEmpty()) r.message = tr("The server reported an error.");
        }
    }

    m_sock->abort();
    emit finished(r);
    deleteLater();
}

// ------------------------------------------------------------------ ServerApi
ServerApi::ServerApi() : QObject(nullptr), m_host(QStringLiteral("127.0.0.1")), m_port(8080) {}

ServerApi &ServerApi::instance()
{
    static ServerApi api;
    return api;
}

void ServerApi::setEndpoint(const QString &host, quint16 port)
{
    m_host = host.trimmed();
    m_port = port;
}

PendingReply *ServerApi::send(const QStringList &fields)
{
    QStringList clean;
    for (int i = 0; i < fields.size(); ++i) {
        QString f = fields.at(i);
        f.replace(QLatin1Char('|'), QLatin1Char(' '));
        f.replace(QLatin1Char('\n'), QLatin1Char(' '));
        f.replace(QLatin1Char('\r'), QLatin1Char(' '));
        clean.append(f);
    }
    QByteArray line = clean.join(QLatin1Char('|')).toUtf8();
    line.append('\n');
    return new PendingReply(m_host, m_port, line, this);
}
