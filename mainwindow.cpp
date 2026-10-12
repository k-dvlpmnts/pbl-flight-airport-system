// mainwindow.cpp
#include "mainwindow.h"

#include <QStackedWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
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
#include <QSet>
#include <QDebug>

static const QString SERVER_BASE = QStringLiteral("http://172.16.71.220:8080");

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      stack(nullptr),
      menuPage(nullptr),
      passengerLoginPage(nullptr),
      staffLoginPage(nullptr),
      passengerPortalPage(nullptr),
      staffPortalPage(nullptr),
      ticketing(nullptr),
      fromCombo(nullptr),
      toCombo(nullptr),
      dateEdit(nullptr),
      searchBtn(nullptr),
      bookBtn(nullptr),
      flightView(nullptr),
      flightModel(nullptr),
      seatWidget(nullptr),
      routeLabel(nullptr),
      fareLabel(nullptr),
      pUser(nullptr),
      pPass(nullptr),
      sUser(nullptr),
      sPass(nullptr),
      net(new QNetworkAccessManager(this)),
      currentFlightFare(0.0)
{
    setWindowTitle("Flight and Airport Management System");
    resize(1000, 650);

    // Task 6: Modern Qt GUI Design (SaaS aviation palette, rounded cards, sleek borders)
    setStyleSheet(R"(
        QWidget {
            background-color: #0F172A;
            font-family: 'Segoe UI', Arial;
            color: #F8FAFC;
        }
        QPushButton {
            background-color: #3B82F6;
            color: white;
            padding: 10px 18px;
            border-radius: 6px;
            font-weight: 600;
            border: none;
        }
        QPushButton:hover {
            background-color: #2563EB;
        }
        QPushButton#backBtn {
            background-color: #334155;
            color: #CBD5E1;
            padding: 6px 12px;
            font-size: 12px;
        }
        QPushButton#backBtn:hover {
            background-color: #475569;
            color: white;
        }
        QLineEdit, QComboBox, QDateEdit {
            background-color: #1E293B;
            border: 1px solid #475569;
            border-radius: 6px;
            padding: 8px;
            color: #F8FAFC;
        }
        QLabel {
            font-size: 14px;
            color: #E2E8F0;
        }
        QTableView {
            background-color: #1E293B;
            alternate-background-color: #0F172A;
            color: #F8FAFC;
            gridline-color: #334155;
            border: 1px solid #334155;
            border-radius: 6px;
        }
        QHeaderView::section {
            background-color: #334155;
            color: #F8FAFC;
            padding: 6px;
            border: none;
            font-weight: bold;
        }
    )");

    stack = new QStackedWidget(this);
    setCentralWidget(stack);

    // Helper lambda for Task 1: Top-left back arrow button on subpages
    auto createBackArrow = [this]() {
        QPushButton *backBtn = new QPushButton("← Back", this);
        backBtn->setObjectName("backBtn");
        backBtn->setFixedWidth(80);
        connect(backBtn, &QPushButton::clicked, this, &MainWindow::goBack);
        return backBtn;
    };

    // --- Menu Page ---
    menuPage = new QWidget;
    QVBoxLayout *menuLayout = new QVBoxLayout(menuPage);
    QLabel *menuTitle = new QLabel("Flight & Airport Management System");
    menuTitle->setAlignment(Qt::AlignCenter);
    menuTitle->setStyleSheet("font-size: 26px; font-weight: bold; color: #38BDF8; margin-bottom: 20px;");
    menuLayout->addWidget(menuTitle);

    QPushButton *passengerBtn = new QPushButton("Passenger Portal");
    QPushButton *staffBtn = new QPushButton("Staff Portal");
    QPushButton *exitBtn = new QPushButton("Exit Application");
    
    passengerBtn->setMinimumHeight(45);
    staffBtn->setMinimumHeight(45);
    exitBtn->setMinimumHeight(45);

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
    pLayout->addWidget(createBackArrow(), 0, Qt::AlignLeft);
    
    QLabel *pTitle = new QLabel("Passenger Login");
    pTitle->setAlignment(Qt::AlignCenter);
    pTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #38BDF8;");
    pLayout->addWidget(pTitle);

    pLayout->addWidget(new QLabel("Username:"));
    pUser = new QLineEdit;
    pLayout->addWidget(pUser);

    pLayout->addWidget(new QLabel("Password:"));
    pPass = new QLineEdit;
    pPass->setEchoMode(QLineEdit::Password);
    pLayout->addWidget(pPass);

    QPushButton *pLogin = new QPushButton("Login");
    QPushButton *pRegister = new QPushButton("Create Account");
    pLayout->addWidget(pLogin);
    pLayout->addWidget(pRegister);

    connect(pLogin, &QPushButton::clicked, this, [this]() { validateUser(pUser->text().trimmed(), pPass->text(), false); });
    connect(pRegister, &QPushButton::clicked, this, [this]() { registerUser(pUser->text().trimmed(), pPass->text(), false); });
    stack->addWidget(passengerLoginPage);

    // --- Staff Login Page ---
    staffLoginPage = new QWidget;
    QVBoxLayout *sLayout = new QVBoxLayout(staffLoginPage);
    sLayout->addWidget(createBackArrow(), 0, Qt::AlignLeft);

    QLabel *sTitle = new QLabel("Staff Login");
    sTitle->setAlignment(Qt::AlignCenter);
    sTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #38BDF8;");
    sLayout->addWidget(sTitle);

    sLayout->addWidget(new QLabel("Username:"));
    sUser = new QLineEdit;
    sLayout->addWidget(sUser);

    sLayout->addWidget(new QLabel("Password:"));
    sPass = new QLineEdit;
    sPass->setEchoMode(QLineEdit::Password);
    sLayout->addWidget(sPass);

    QPushButton *sLogin = new QPushButton("Login");
    QPushButton *sRegister = new QPushButton("Create Account");
    sLayout->addWidget(sLogin);
    sLayout->addWidget(sRegister);

    connect(sLogin, &QPushButton::clicked, this, [this]() { validateUser(sUser->text().trimmed(), sPass->text(), true); });
    connect(sRegister, &QPushButton::clicked, this, [this]() { registerUser(sUser->text().trimmed(), sPass->text(), true); });
    stack->addWidget(staffLoginPage);

    // -- Passenger Portal Page --
    passengerPortalPage = new QWidget;
    QVBoxLayout *plLayout = new QVBoxLayout(passengerPortalPage);
    plLayout->addWidget(createBackArrow(), 0, Qt::AlignLeft);

    QPushButton *book = new QPushButton("Book Ticket & Search Routes");
    QPushButton *track = new QPushButton("Track Luggage");
    book->setMinimumHeight(45);
    track->setMinimumHeight(45);
    plLayout->addWidget(book);
    plLayout->addWidget(track);
    connect(book, &QPushButton::clicked, this, &MainWindow::ticket);
    stack->addWidget(passengerPortalPage);

    // -- Staff Portal Page --
    staffPortalPage = new QWidget;
    QVBoxLayout *slLayout = new QVBoxLayout(staffPortalPage);
    slLayout->addWidget(createBackArrow(), 0, Qt::AlignLeft);

    QLabel *staffLabel = new QLabel("Staff Management Portal (Active)");
    staffLabel->setAlignment(Qt::AlignCenter);
    staffLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #38BDF8;");
    slLayout->addWidget(staffLabel);
    stack->addWidget(staffPortalPage);

    // -- Ticketing Page (Task 2 & 4 & 6) --
    ticketing = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout(ticketing);
    mainLayout->addWidget(createBackArrow(), 0, Qt::AlignLeft);

    fromCombo = new QComboBox;
    toCombo = new QComboBox;
    dateEdit = new QDateEdit(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    searchBtn = new QPushButton("Find Optimal Route");
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

    // Graph route display label
    routeLabel = new QLabel("Optimal Route Path: -");
    routeLabel->setStyleSheet("color: #38BDF8; font-weight: bold; font-size: 15px;");
    mainLayout->addWidget(routeLabel);

    flightView = new QTableView;
    flightModel = new FlightModel(this);
    flightView->setModel(flightModel);
    flightView->setSelectionBehavior(QAbstractItemView::SelectRows);
    flightView->setSelectionMode(QAbstractItemView::SingleSelection);
    flightView->horizontalHeader()->setStretchLastSection(true);
    mainLayout->addWidget(flightView);

    // Seat widget & booking footer
    QHBoxLayout *bottomBookingLayout = new QHBoxLayout;
    seatWidget = new SeatWidget;
    seatWidget->setVisible(false);
    
    QVBoxLayout *bookingActionLayout = new QVBoxLayout;
    fareLabel = new QLabel("Total Fare: $0.00");
    fareLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #10B981;");
    bookBtn = new QPushButton("Confirm Booking");
    bookBtn->setVisible(false);
    bookBtn->setMinimumHeight(40);

    bookingActionLayout->addWidget(fareLabel);
    bookingActionLayout->addWidget(bookBtn);
    bookingActionLayout->setAlignment(Qt::AlignCenter);

    bottomBookingLayout->addWidget(seatWidget);
    bottomBookingLayout->addLayout(bookingActionLayout);
    mainLayout->addLayout(bottomBookingLayout);

    connect(searchBtn, &QPushButton::clicked, this, &MainWindow::onSearchFlights);
    connect(flightView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFlightSelected);
    connect(seatWidget, &SeatWidget::seatToggled, this, &MainWindow::onSeatToggled);
    connect(bookBtn, &QPushButton::clicked, this, &MainWindow::bookSelectedFlight);

    stack->addWidget(ticketing);
    stack->setCurrentWidget(menuPage);

    populateStopsFromServer();
}

MainWindow::~MainWindow() {}

void MainWindow::passenger() { stack->setCurrentWidget(passengerLoginPage); }
void MainWindow::staff() { stack->setCurrentWidget(staffLoginPage); }
void MainWindow::goBack() { stack->setCurrentWidget(menuPage); }
void MainWindow::ticket() { stack->setCurrentWidget(ticketing); }

void MainWindow::populateStopsFromServer()
{
    QNetworkRequest req(QUrl(SERVER_BASE + "/api/search_stops"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = net->post(req, QJsonDocument(QJsonObject()).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();
            return;
        }
        QSet<QString> stops;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isArray()) {
            for (const QJsonValue &v : doc.array()) {
                if (!v.toString().isEmpty()) stops.insert(v.toString());
            }
        }
        QStringList list = stops.values();
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
            reply->deleteLater();
            return;
        }

        QJsonArray arr = doc.array();
        QVector<FlightRecord> flights;
        QStringList routePathStops;
        
        for (const QJsonValue &v : arr) {
            if (!v.isObject()) continue;
            QJsonObject obj = v.toObject();
            FlightRecord fr;
            fr.id = obj.value("flightNumber").toString();
            fr.dep = obj.value("origin").toString();
            fr.arr = obj.value("destination").toString();
            fr.duration = obj.value("departureTime").toString();
            fr.fare = obj.value("baseFare").toDouble();
            fr.seatsSoldCount = obj.value("seatsSoldCount").toInt();
            flights.append(fr);

            if (routePathStops.isEmpty()) routePathStops.append(fr.dep);
            routePathStops.append(fr.arr);
        }

        if (!routePathStops.isEmpty()) {
            routeLabel->setText("Optimal Route Path: " + routePathStops.join(" → "));
        } else {
            routeLabel->setText("Optimal Route Path: No route found");
        }

        flightModel->setFlights(flights);
        seatWidget->setVisible(false);
        bookBtn->setVisible(false);
        fareLabel->setText("Total Fare: $0.00");
        reply->deleteLater();
    });
}

void MainWindow::onFlightSelected(const QItemSelection &selected, const QItemSelection &)
{
    if (selected.indexes().isEmpty()) return;
    int row = selected.indexes().first().row();
    FlightRecord f = flightModel->flightAt(row);
    currentFlightFare = f.fare;

    QVector<bool> sold(30, false);
    for (int i = 0; i < qMin(f.seatsSoldCount, sold.size()); ++i)
        sold[i] = true;

    selectedSeats.clear();
    seatWidget->populate(sold);
    seatWidget->setVisible(true);
    bookBtn->setVisible(true);
    fareLabel->setText("Total Fare: $0.00");
}

void MainWindow::onSeatToggled(int seatIndex, bool selected)
{
    if (selected) {
        if (!selectedSeats.contains(seatIndex)) selectedSeats.append(seatIndex);
    } else {
        selectedSeats.removeAll(seatIndex);
    }
    double total = selectedSeats.size() * currentFlightFare;
    fareLabel->setText(QString("Total Fare (%1 seats): $%2").arg(selectedSeats.size()).arg(total, 0, 'f', 2));
}

void MainWindow::bookSelectedFlight()
{
    if (selectedSeats.isEmpty()) {
        QMessageBox::warning(this, "Booking", "Please select at least one seat from the matrix.");
        return;
    }

    QModelIndexList indexes = flightView->selectionModel()->selectedRows();
    if (indexes.isEmpty()) return;
    FlightRecord f = flightModel->flightAt(indexes.first().row());

    QJsonObject body;
    body["username"] = loggedInUsername.isEmpty() ? "guest_user" : loggedInUsername;
    body["flightNumber"] = f.id;
    body["seatCount"] = selectedSeats.size();
    body["totalFare"] = selectedSeats.size() * currentFlightFare;

    QNetworkRequest req(QUrl(SERVER_BASE + "/api/book"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = net->post(req, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, "Network Error", reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("ok").toBool(false);
        QString error = doc.object().value("error").toString();

        if (ok) {
            QMessageBox::information(this, "Success", "Ticket booked successfully! Details saved to server records.");
            stack->setCurrentWidget(passengerPortalPage);
        } else {
            if (error == "booking_not_open_yet") {
                QMessageBox::warning(this, "Booking Restricted", "Booking opens only one month prior to departure.");
            } else if (error == "seats_full") {
                QMessageBox::warning(this, "Booking Failed", "Selected seats are sold out.");
            } else {
                QMessageBox::warning(this, "Booking Failed", "Could not complete booking.");
            }
        }
        reply->deleteLater();
    });
}

void MainWindow::validateUser(const QString &username, const QString &password, bool isStaff)
{
    if (username.trimmed().isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Please enter username and password.");
        return;
    }

    loggedInUsername = username;
    QJsonObject body;
    body["role"] = isStaff ? "Staff" : "Passenger";
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
        bool ok = doc.object().value("ok").toBool(false);
        if (ok) {
            if (isStaff) stack->setCurrentWidget(staffPortalPage);
            else stack->setCurrentWidget(passengerPortalPage);
        } else {
            QMessageBox::warning(this, "Login Failed", "Invalid credentials.");
        }

        pUser->clear(); pPass->clear();
        sUser->clear(); sPass->clear();
        reply->deleteLater();
    });
}

void MainWindow::registerUser(const QString &username, const QString &password, bool isStaff)
{
    if (username.trimmed().isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Please enter username and password to register.");
        return;
    }

    QJsonObject body;
    body["role"] = isStaff ? "Staff" : "Passenger";
    body["username"] = username;
    body["password"] = password;

    QNetworkRequest req(QUrl(SERVER_BASE + "/api/register"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = net->post(req, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, "Network Error", reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("ok").toBool(false);
        QString error = doc.object().value("error").toString();

        if (ok) {
            QMessageBox::information(this, "Success", "Account created successfully! You can now log in.");
        } else {
            if (error == "user_exists") {
                QMessageBox::warning(this, "Registration Failed", "Username already exists.");
            } else {
                QMessageBox::warning(this, "Registration Failed", "Could not create account.");
            }
        }

        pUser->clear(); pPass->clear();
        sUser->clear(); sPass->clear();
        reply->deleteLater();
    });
}