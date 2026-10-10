// mainwindow.cpp
#include "mainwindow.h"

#include <QStackedWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QApplication>
#include <QString>
#include <QMessageBox>

#include "flightmodel.h"
#include "seatwidget.h"

#include <QComboBox>
#include <QDateEdit>
#include <QTableView>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QItemSelection>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QSet>
#include <QDebug>

static const QString SERVER_BASE = QStringLiteral("http://127.0.0.1:8080"); // replace with your server IP/host

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      stack(nullptr),
      menuPage(nullptr),
      passengerLoginPage(nullptr),
      staffLoginPage(nullptr),
      passengerPortalPage(nullptr),
      ticketing(nullptr),
      fromCombo(nullptr),
      toCombo(nullptr),
      dateEdit(nullptr),
      searchBtn(nullptr),
      flightView(nullptr),
      flightModel(nullptr),
      seatWidget(nullptr),
      pUser(nullptr),
      pPass(nullptr),
      sUser(nullptr),
      sPass(nullptr),
      net(new QNetworkAccessManager(this))
{
    setWindowTitle("Flight and Airport Management System");
    resize(900, 500);

    // Basic stylesheet
    setStyleSheet(R"(
        QWidget {
            background-color: #F5F0EC;
            font-family: 'Segoe UI';
            color: #333;
        }
        QPushButton {
            background-color: #B06058;
            color: white;
            padding: 10px 20px;
            border-radius: 8px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #CFC0C0;
            color: #222;
        }
        QLineEdit {
            background-color: #FFF;
            border: 2px solid #B06058;
            border-radius: 6px;
            padding: 6px;
        }
        QLabel {
            font-size: 14px;
            font-weight: 600;
        }
    )");

    // Central stacked widget
    stack = new QStackedWidget(this);
    setCentralWidget(stack);

    // --- Menu Page ---
    menuPage = new QWidget;
    QVBoxLayout *menuLayout = new QVBoxLayout(menuPage);
    QLabel *menuTitle = new QLabel("Welcome to Flight Airport System");
    menuTitle->setAlignment(Qt::AlignCenter);
    menuTitle->setStyleSheet("font-size: 22px; font-weight: bold; color: #B06058;");
    menuLayout->addWidget(menuTitle);

    QPushButton *passengerBtn = new QPushButton("Passenger Portal");
    QPushButton *staffBtn = new QPushButton("Staff Portal");
    QPushButton *exitBtn = new QPushButton("Exit");
    menuLayout->addWidget(passengerBtn);
    menuLayout->addWidget(staffBtn);
    menuLayout->addWidget(exitBtn);
    menuLayout->setAlignment(Qt::AlignCenter);

    connect(passengerBtn, &QPushButton::clicked, this, &MainWindow::passenger);
    connect(staffBtn, &QPushButton::clicked, this, &MainWindow::staff);
    connect(exitBtn, &QPushButton::clicked, qApp, &QApplication::quit);

    stack->addWidget(menuPage);

    // --- Passenger Login Page ---
    passengerLoginPage = new QWidget;
    QVBoxLayout *pLayout = new QVBoxLayout(passengerLoginPage);
    QLabel *pTitle = new QLabel("Passenger Login");
    pTitle->setAlignment(Qt::AlignCenter);
    pTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #B06058;");
    pLayout->addWidget(pTitle);

    pLayout->addWidget(new QLabel("Username:"));
    pUser = new QLineEdit;
    pLayout->addWidget(pUser);

    pLayout->addWidget(new QLabel("Password:"));
    pPass = new QLineEdit;
    pPass->setEchoMode(QLineEdit::Password);
    pLayout->addWidget(pPass);

    QPushButton *pLogin = new QPushButton("Login");
    QPushButton *pBack = new QPushButton("Back");
    pLayout->addWidget(pLogin);
    pLayout->addWidget(pBack);

    connect(pBack, &QPushButton::clicked, this, &MainWindow::goBack);
    connect(pLogin, &QPushButton::clicked, this, [this]() {
        QString user = pUser->text().trimmed();
        QString pass = pPass->text();
        validateUser(user, pass, /*isStaff=*/false);
    });

    stack->addWidget(passengerLoginPage);

    // --- Staff Login Page ---
    staffLoginPage = new QWidget;
    QVBoxLayout *sLayout = new QVBoxLayout(staffLoginPage);
    QLabel *sTitle = new QLabel("Staff Login");
    sTitle->setAlignment(Qt::AlignCenter);
    sTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #B06058;");
    sLayout->addWidget(sTitle);

    sLayout->addWidget(new QLabel("Username:"));
    sUser = new QLineEdit;
    sLayout->addWidget(sUser);

    sLayout->addWidget(new QLabel("Password:"));
    sPass = new QLineEdit;
    sPass->setEchoMode(QLineEdit::Password);
    sLayout->addWidget(sPass);

    QPushButton *sLogin = new QPushButton("Login");
    QPushButton *sBack = new QPushButton("Back");
    sLayout->addWidget(sLogin);
    sLayout->addWidget(sBack);

    connect(sBack, &QPushButton::clicked, this, &MainWindow::goBack);
    connect(sLogin, &QPushButton::clicked, this, [this]() {
        QString user = sUser->text().trimmed();
        QString pass = sPass->text();
        validateUser(user, pass, /*isStaff=*/true);
    });

    stack->addWidget(staffLoginPage);

    // -- Passenger Portal Page --
    passengerPortalPage = new QWidget;
    QVBoxLayout *plLayout = new QVBoxLayout(passengerPortalPage);
    QPushButton *book = new QPushButton("Book Ticket");
    QPushButton *track = new QPushButton("Track Luggage");
    plLayout->addWidget(book);
    plLayout->addWidget(track);
    connect(book, &QPushButton::clicked, this, &MainWindow::ticket);
    stack->addWidget(passengerPortalPage);

    // -- Ticketing Page --
    ticketing = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout(ticketing);

    fromCombo = new QComboBox;
    toCombo = new QComboBox;
    dateEdit = new QDateEdit(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    searchBtn = new QPushButton("Search");
    searchBtn->setDefault(true);

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(new QLabel("From"));
    topLayout->addWidget(fromCombo);
    topLayout->addWidget(new QLabel("To"));
    topLayout->addWidget(toCombo);
    topLayout->addWidget(new QLabel("Date"));
    topLayout->addWidget(dateEdit);
    topLayout->addWidget(searchBtn);

    mainLayout->addLayout(topLayout);

    // Flight list
    flightView = new QTableView;
    flightModel = new FlightModel(this);
    flightView->setModel(flightModel);
    flightView->setSelectionBehavior(QAbstractItemView::SelectRows);
    flightView->setSelectionMode(QAbstractItemView::SingleSelection);
    flightView->horizontalHeader()->setStretchLastSection(true);
    mainLayout->addWidget(flightView);

    // Seat widget
    seatWidget = new SeatWidget;
    seatWidget->setVisible(false);
    mainLayout->addWidget(seatWidget);

    connect(searchBtn, &QPushButton::clicked, this, &MainWindow::onSearchFlights);
    connect(flightView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onFlightSelected);

    stack->addWidget(ticketing);

    // Start at menu
    stack->setCurrentWidget(menuPage);

    // Populate stops from server (async). If server unreachable, you can fallback to local file.
    populateStopsFromServer();
}

MainWindow::~MainWindow() {}

void MainWindow::passenger() { stack->setCurrentWidget(passengerLoginPage); }
void MainWindow::staff() { stack->setCurrentWidget(staffLoginPage); }
void MainWindow::goBack() { stack->setCurrentWidget(menuPage); }
void MainWindow::ticket() { stack->setCurrentWidget(ticketing); }

void MainWindow::populateStopsFromServer()
{
    // Request all flights (empty filter) and extract unique stop names
    QNetworkRequest req(QUrl(SERVER_BASE + "/api/search"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = net->post(req, QJsonDocument(QJsonObject()).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "populateStopsFromServer: network error:" << reply->errorString();
            reply->deleteLater();
            return;
        }
        QSet<QString> stops;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isArray()) {
            reply->deleteLater();
            return;
        }
        QJsonArray arr = doc.array();
        for (const QJsonValue &v : arr) {
            if (!v.isObject()) continue;
            QJsonArray sarr = v.toObject().value("stops").toArray();
            for (const QJsonValue &sv : sarr) {
                if (!sv.isObject()) continue;
                QString name = sv.toObject().value("name").toString();
                if (!name.isEmpty()) stops.insert(name);
            }
        }
        QStringList list;
        list.reserve(stops.size());
        for (const QString &s : stops)
            list.append(s);
        list.sort(Qt::CaseInsensitive);

        fromCombo->clear();
        toCombo->clear();
        fromCombo->addItems(list);
        toCombo->addItems(list);
        reply->deleteLater();
    });
}

void MainWindow::onSearchFlights()
{
    // Build request body from UI
    QJsonObject body;
    body["from"] = fromCombo->currentText();
    body["to"] = toCombo->currentText();
    body["date"] = dateEdit->date().toString(Qt::ISODate);

    QNetworkRequest req(QUrl(SERVER_BASE + "/api/search"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = net->post(req, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, "Network Error", reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isArray()) {
            QMessageBox::warning(this, "Server Error", "Unexpected response from server.");
            reply->deleteLater();
            return;
        }

        QJsonArray arr = doc.array();
        QVector<FlightRecord> flights;
        flights.reserve(arr.size());
        for (const QJsonValue &v : arr) {
            if (!v.isObject()) continue;
            QJsonObject obj = v.toObject();
            FlightRecord fr;
            fr.id = obj.value("flightNumber").toString();
            QJsonArray stops = obj.value("stops").toArray();
            if (!stops.isEmpty()) {
                fr.dep = stops.first().toObject().value("name").toString();
                fr.arr = stops.last().toObject().value("name").toString();
            } else {
                fr.dep = QString();
                fr.arr = QString();
            }
            fr.duration = QString::number(obj.value("totalDistanceKm").toDouble());
            fr.fare = obj.value("baseFare").toDouble();
            fr.seatsSoldCount = 0; // server can provide seat info if extended
            flights.append(fr);
        }

        flightModel->setFlights(flights);
        seatWidget->setVisible(false);
        reply->deleteLater();
    });
}

void MainWindow::onFlightSelected(const QItemSelection &selected, const QItemSelection &)
{
    if (selected.indexes().isEmpty()) return;
    int row = selected.indexes().first().row();
    FlightRecord f = flightModel->flightAt(row);

    QVector<bool> sold(30, false);
    for (int i = 0; i < qMin(f.seatsSoldCount, sold.size()); ++i)
        sold[i] = true;

    seatWidget->populate(sold);
    seatWidget->setVisible(true);
}

void MainWindow::validateUser(const QString &username, const QString &password, bool isStaff)
{
    if (username.trimmed().isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Please enter username and password.");
        return;
    }

    QJsonObject body;
    body["username"] = username;
    body["password"] = password;

    QNetworkRequest req(QUrl(SERVER_BASE + "/api/validate"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = net->post(req, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, isStaff]() {
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, "Network Error", reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            QMessageBox::warning(this, "Server Error", "Invalid response from server.");
            reply->deleteLater();
            return;
        }

        bool ok = doc.object().value("ok").toBool(false);
        if (ok) {
            if (isStaff) stack->setCurrentWidget(staffPortalPage);
            else stack->setCurrentWidget(passengerPortalPage);
        } else {
            QMessageBox::warning(this, "Login Failed", "Invalid credentials.");
        }

        // clear fields
        pUser->clear();
        pPass->clear();
        sUser->clear();
        sPass->clear();

        reply->deleteLater();
    });
}
