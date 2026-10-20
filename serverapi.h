#pragma once
// ---------------------------------------------------------------------------
//  Talks to server.cpp.  One request per TCP connection:
//      "COMMAND|field|field\n"  ->  "OK|...\n" or "ERR|code|message\n" + data rows
//  Requests are asynchronous (the GUI never freezes if the server is slow).
// ---------------------------------------------------------------------------
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QByteArray>
#include <QAbstractSocket>

class QTcpSocket;
class QTimer;

// returns field i of a row, or an empty string when the row is shorter
inline QString fld(const QStringList &row, int i) { return i >= 0 && i < row.size() ? row.at(i) : QString(); }

struct Reply {
    bool netOk;                      // false: the server could not be reached
    bool ok;                         // the server answered "OK"
    QString code;                    // "network" or the server's error code
    QString message;                 // readable error text
    QVector<QStringList> rows;       // every row, split on '|'  (rows[0] is the OK/ERR row)

    Reply() : netOk(false), ok(false) {}
    QStringList head() const { return rows.isEmpty() ? QStringList() : rows.first(); }
    QString headField(int i) const { return fld(head(), i); }
    QVector<QStringList> rowsOf(const QString &tag) const {
        QVector<QStringList> out;
        for (int i = 0; i < rows.size(); ++i)
            if (!rows.at(i).isEmpty() && rows.at(i).at(0) == tag) out.append(rows.at(i));
        return out;
    }
};
Q_DECLARE_METATYPE(Reply)

class PendingReply : public QObject {
    Q_OBJECT
public:
    PendingReply(const QString &host, quint16 port, const QByteArray &request, QObject *parent = nullptr);

signals:
    void finished(const Reply &reply);

private slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void onTimeout();

private:
    void finish(bool netOk, const QString &errorText);

    QTcpSocket *m_sock;
    QTimer *m_timer;
    QByteArray m_request;
    QByteArray m_buffer;
    bool m_done;
};

class ServerApi : public QObject {
    Q_OBJECT
public:
    static ServerApi &instance();

    void setEndpoint(const QString &host, quint16 port);
    QString host() const { return m_host; }
    quint16 port() const { return m_port; }

    // fire a request; connect to PendingReply::finished to receive the answer
    PendingReply *send(const QStringList &fields);

private:
    ServerApi();
    QString m_host;
    quint16 m_port;
};
