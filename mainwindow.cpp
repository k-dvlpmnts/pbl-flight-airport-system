#include "mainwindow.h"
#include "flightmodel.h"
#include "seatwidget.h"
#include <QTimer>
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHeaderView>
#include <QListWidget>
#include <QEventLoop>
#include <QInputDialog>
#include <QDateTimeEdit>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QDateEdit>
#include <QStackedWidget>
#include <QTableView>
#include <QItemSelectionModel>
#include <QItemSelection>
#include <QModelIndex>
#include <QVector>

// ---------- helpers ----------
static QJsonObject postJson(QNetworkAccessManager* net, const QString& url, const QJsonObject& body) {
    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* reply = net->post(req, QJsonDocument(body).toJson());
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    QJsonObject out;
    if (reply->error() == QNetworkReply::NoError) {
        out = QJsonDocument::fromJson(reply->readAll()).object();
    }
    reply->deleteLater();
    return out;
}

static QJsonArray postJsonArr(QNetworkAccessManager* net, const QString& url, const QJsonObject& body) {
    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* reply = net->post(req, QJsonDocument(body).toJson());
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    QJsonArray out;
    if (reply->error() == QNetworkReply::NoError) {
        out = QJsonDocument::fromJson(reply->readAll()).array();
    }
    reply->deleteLater();
    return out;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Flight & Airport Operations System");
    resize(1100, 720);
    serverBase = "http://127.0.0.1:8080";
    net = new QNetworkAccessManager(this);

    setStyleSheet(R"(
        QWidget { background:#F5F7FB; color:#1F2937; font-family:'Segoe UI'; font-size:14px; }
        QPushButton {
            background:#3B82F6; color:white; padding:10px 18px; border-radius:8px; font-weight:600;
        }
        QPushButton:hover { background:#2563EB; }
        QPushButton#backBtn { background:#E5E7EB; color:#374151; padding:6px 12px; font-size:12px; }
        QPushButton#logoutBtn { background:#EF4444; color:white; padding:6px 12px; font-size:12px; }
        QLineEdit, QComboBox, QDateEdit, QDateTimeEdit {
            background:white; border:1px solid #CBD5E1; border-radius:6px; padding:8px;
        }
        QLabel#title { font-size:22px; font-weight:bold; color:#1E3A8A; }
        QTableView { background:white; border:1px solid #CBD5E1; border-radius:8px; }
        QHeaderView::section { background:#EFF6FF; color:#1E3A8A; padding:6px; border:none; font-weight:bold; }
        QListWidget { background:white; border:1px solid #CBD5E1; border-radius:8px; padding:6px; }
    )");

    stack = new QStackedWidget;
    setCentralWidget(stack);

    // ================= FIRST PAGE =================
    firstPage = new QWidget;
    {
        auto* l = new QVBoxLayout(firstPage);
        auto* t = new QLabel("Flight & Airport Operations System");
        t->setObjectName("title");
        t->setAlignment(Qt::AlignCenter);
        l->addWidget(t);
        l->addSpacing(30);
        auto* pBtn = new QPushButton("Passenger Login");
        auto* sBtn = new QPushButton("Staff Login");
        auto* simBtn = new QPushButton("Simulator");
        pBtn->setMinimumHeight(45);
        sBtn->setMinimumHeight(45);
        simBtn->setMinimumHeight(45);
        l->addWidget(pBtn);
        l->addWidget(sBtn);
        l->addWidget(simBtn);
        l->setAlignment(Qt::AlignCenter);
        connect(pBtn, &QPushButton::clicked, this, [this]{ pushAndGo(passengerLoginPage); });
        connect(sBtn, &QPushButton::clicked, this, [this]{ pushAndGo(staffLoginPage); });
        connect(simBtn, &QPushButton::clicked, this, &MainWindow::openSimulator);
    }
    stack->addWidget(firstPage);

    // ================= PASSENGER LOGIN =================
    passengerLoginPage = new QWidget;
    {
        auto* l = new QVBoxLayout(passengerLoginPage);
        auto* top = new QHBoxLayout;
        top->addWidget(backBtn());
        top->addStretch();
        top->addWidget(logoutBtn());
        l->addLayout(top);
        auto* t = new QLabel("Passenger Login");
        t->setObjectName("title");
        t->setAlignment(Qt::AlignCenter);
        l->addWidget(t);
        l->addSpacing(20);
        pUser = new QLineEdit; pUser->setPlaceholderText("Username");
        pPass = new QLineEdit; pPass->setPlaceholderText("Password"); pPass->setEchoMode(QLineEdit::Password);
        auto* login = new QPushButton("Login");
        auto* reg = new QPushButton("Create Account");
        l->addWidget(pUser);
        l->addWidget(pPass);
        l->addWidget(login);
        l->addWidget(reg);
        l->setAlignment(Qt::AlignCenter);
        connect(login, &QPushButton::clicked, this, &MainWindow::onPassengerLogin);
        connect(reg, &QPushButton::clicked, this, [this]{
            QJsonObject reqObj;
            reqObj["username"] = pUser->text();
            reqObj["password"] = pPass->text();
            auto r = postJson(net, serverBase + "/api/register", reqObj);
            QMessageBox::information(this, "Register",
                r.value("ok").toBool() ? "Account created." : "Failed.");
        });
    }
    stack->addWidget(passengerLoginPage);

    // ================= STAFF LOGIN =================
    staffLoginPage = new QWidget;
    {
        auto* l = new QVBoxLayout(staffLoginPage);
        auto* top = new QHBoxLayout;
        top->addWidget(backBtn());
        top->addStretch();
        top->addWidget(logoutBtn());
        l->addLayout(top);
        auto* t = new QLabel("Staff Login");
        t->setObjectName("title");
        t->setAlignment(Qt::AlignCenter);
        l->addWidget(t);
        l->addSpacing(20);
        sUser = new QLineEdit; sUser->setPlaceholderText("Username");
        sPass = new QLineEdit; sPass->setPlaceholderText("Password"); sPass->setEchoMode(QLineEdit::Password);
        auto* login = new QPushButton("Login");
        auto* adminBtn = new QPushButton("Admin Panel");
        adminBtn->setObjectName("backBtn");
        l->addWidget(sUser);
        l->addWidget(sPass);
        l->addWidget(login);
        l->addWidget(adminBtn);
        l->setAlignment(Qt::AlignCenter);
        connect(login, &QPushButton::clicked, this, &MainWindow::onStaffLogin);
        connect(adminBtn, &QPushButton::clicked, this, [this]{ pushAndGo(adminLoginPage); });
    }
    stack->addWidget(staffLoginPage);

    // ================= ADMIN LOGIN =================
    adminLoginPage = new QWidget;
    {
        auto* l = new QVBoxLayout(adminLoginPage);
        auto* top = new QHBoxLayout;
        top->addWidget(backBtn());
        top->addStretch();
        top->addWidget(logoutBtn());
        l->addLayout(top);
        auto* t = new QLabel("Admin Login");
        t->setObjectName("title");
        t->setAlignment(Qt::AlignCenter);
        l->addWidget(t);
        aUser = new QLineEdit; aUser->setPlaceholderText("Admin Username");
        aPass = new QLineEdit; aPass->setPlaceholderText("Password"); aPass->setEchoMode(QLineEdit::Password);
        auto* login = new QPushButton("Login");
        l->addWidget(aUser);
        l->addWidget(aPass);
        l->addWidget(login);
        l->setAlignment(Qt::AlignCenter);
        connect(login, &QPushButton::clicked, this, &MainWindow::onAdminLogin);
    }
    stack->addWidget(adminLoginPage);

    // ================= ADMIN PANEL =================
    adminPanelPage = new QWidget;
    {
        auto* l = new QVBoxLayout(adminPanelPage);
        auto* top = new QHBoxLayout;
        top->addWidget(backBtn());
        top->addStretch();
        top->addWidget(logoutBtn());
        l->addLayout(top);
        auto* t = new QLabel("Create Staff Account");
        t->setObjectName("title");
        t->setAlignment(Qt::AlignCenter);
        l->addWidget(t);
        newStaffUser = new QLineEdit; newStaffUser->setPlaceholderText("New staff username");
        newStaffPass = new QLineEdit; newStaffPass->setPlaceholderText("Password"); newStaffPass->setEchoMode(QLineEdit::Password);
        newStaffRole = new QComboBox; newStaffRole->addItems({"checkin", "belt"});
        auto* create = new QPushButton("Create Staff");
        l->addWidget(newStaffUser);
        l->addWidget(newStaffPass);
        l->addWidget(new QLabel("Role:"));
        l->addWidget(newStaffRole);
        l->addWidget(create);
        l->setAlignment(Qt::AlignCenter);
        connect(create, &QPushButton::clicked, this, [this]{
            QJsonObject reqObj;
            reqObj["admin"] = aUser->text();
            reqObj["adminPassword"] = aPass->text();
            reqObj["username"] = newStaffUser->text();
            reqObj["password"] = newStaffPass->text();
            reqObj["staffRole"] = newStaffRole->currentText();
            auto r = postJson(net, serverBase + "/api/admin/create_staff", reqObj);
            QMessageBox::information(this, "Create Staff",
                r.value("ok").toBool() ? "Staff created." : "Failed.");
        });
    }
    stack->addWidget(adminPanelPage);

    // ================= PASSENGER PORTAL =================
    passengerPortalPage = new QWidget;
    {
        auto* l = new QVBoxLayout(passengerPortalPage);
        auto* top = new QHBoxLayout;
        top->addWidget(backBtn());
        top->addStretch();
        top->addWidget(logoutBtn());
        l->addLayout(top);
        auto* t = new QLabel("Passenger Portal");
        t->setObjectName("title");
        t->setAlignment(Qt::AlignCenter);
        l->addWidget(t);
        auto* book = new QPushButton("Book Ticket");
        auto* track = new QPushButton("Track Luggage");
        auto* bp = new QPushButton("My Boarding Passes");
        l->addWidget(book);
        l->addWidget(track);
        l->addWidget(bp);
        l->setAlignment(Qt::AlignCenter);
        connect(book, &QPushButton::clicked, this, &MainWindow::openBooking);
        connect(track, &QPushButton::clicked, this, &MainWindow::openTrackLuggage);
        connect(bp, &QPushButton::clicked, this, [this]{
            boardingList->clear();
            QJsonObject reqObj;
            reqObj["username"] = loggedInUser;
            auto arr = postJsonArr(net, serverBase + "/api/my_boarding_pass", reqObj);
            for (auto v : arr) {
                auto o = v.toObject();
                boardingList->addItem(QString("%1 -> %2 | %3 | Seat %4 | Code %5")
                    .arg(o["origin"].toString(), o["destination"].toString(),
                         o["departureTime"].toString(), o["seatNo"].toString(),
                         o["boardingCode"].toString()));
            }
            pushAndGo(boardingPassPage);
        });
    }
    stack->addWidget(passengerPortalPage);

    // ================= BOARDING PASS PAGE =================
    boardingPassPage = new QWidget;
    {
        auto* l = new QVBoxLayout(boardingPassPage);
        auto* top = new QHBoxLayout;
        top->addWidget(backBtn());
        top->addStretch();
        top->addWidget(logoutBtn());
        l->addLayout(top);
        auto* t = new QLabel("My Boarding Passes");
        t->setObjectName("title");
        t->setAlignment(Qt::AlignCenter);
        l->addWidget(t);
        boardingList = new QListWidget;
        l->addWidget(boardingList);
    }
    stack->addWidget(boardingPassPage);

    // ================= STAFF PORTAL =================
    staffPortalPage = new QWidget;
    {
        auto* l = new QVBoxLayout(staffPortalPage);
        auto* top = new QHBoxLayout;
        top->addWidget(backBtn());
        top->addStretch();
        top->addWidget(logoutBtn());
        l->addLayout(top);
        auto* t = new QLabel("Staff Portal");
        t->setObjectName("title");
        t->setAlignment(Qt::AlignCenter);
        l->addWidget(t);

        auto* checkinLabel = new QLabel("Check-in Passenger (Boarding Code):");
        auto* ciCode = new QLineEdit; ciCode->setPlaceholderText("e.g. AI101-12A-00042");
        auto* ciBtn = new QPushButton("Check In");
        l->addWidget(checkinLabel);
        l->addWidget(ciCode);
        l->addWidget(ciBtn);

        connect(ciBtn, &QPushButton::clicked, this, [this, ciCode]{
            QJsonObject reqObj;
            reqObj["boardingCode"] = ciCode->text();
            reqObj["flightNumber"] = "";
            reqObj["date"] = "";
            auto r = postJson(net, serverBase + "/api/checkin", reqObj);
            QMessageBox::information(this, "Check-in",
                r.value("ok").toBool() ? "Checked in successfully."
                                       : "Failed: " + r.value("error").toString());
        });

        auto* beltBtn = new QPushButton("Belt Monitor");
        l->addWidget(beltBtn);
        connect(beltBtn, &QPushButton::clicked, this, [this]{
            refreshBagList();
            pushAndGo(trackPage);
        });

        l->setAlignment(Qt::AlignCenter);
    }
    stack->addWidget(staffPortalPage);

    // ================= BOOKING PAGE =================
    bookingPage = new QWidget;
    {
        auto* l = new QVBoxLayout(bookingPage);
        auto* top = new QHBoxLayout;
        top->addWidget(backBtn());
        top->addStretch();
        top->addWidget(logoutBtn());
        l->addLayout(top);

        auto* form = new QHBoxLayout;
        fromCombo = new QComboBox;
        toCombo = new QComboBox;
        dateEdit = new QDateEdit(QDate::currentDate());
        dateEdit->setCalendarPopup(true);
        auto* search = new QPushButton("Search");
        form->addWidget(new QLabel("From:"));
        form->addWidget(fromCombo);
        form->addWidget(new QLabel("To:"));
        form->addWidget(toCombo);
        form->addWidget(new QLabel("Date:"));
        form->addWidget(dateEdit);
        form->addWidget(search);
        l->addLayout(form);

        routeLabel = new QLabel("Route: -");
        l->addWidget(routeLabel);

        flightView = new QTableView;
        flightModel = new FlightModel(this);
        flightView->setModel(flightModel);
        flightView->setSelectionBehavior(QAbstractItemView::SelectRows);
        flightView->setSelectionMode(QAbstractItemView::SingleSelection);
        flightView->horizontalHeader()->setStretchLastSection(true);
        l->addWidget(flightView);

        auto* bottom = new QHBoxLayout;
        seatWidget = new SeatWidget;
        auto* rightCol = new QVBoxLayout;
        fareLabel = new QLabel("Fare: Rs 0.00");
        bookBtn = new QPushButton("Confirm Booking");
        bookBtn->setVisible(false);
        rightCol->addWidget(fareLabel);
        rightCol->addWidget(bookBtn);
        bottom->addWidget(seatWidget);
        bottom->addLayout(rightCol);
        l->addLayout(bottom);

        connect(search, &QPushButton::clicked, this, &MainWindow::onSearch);
        connect(flightView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, [this](const QItemSelection& s, const QItemSelection&){
            if (s.indexes().isEmpty()) return;
            auto f = flightModel->flightAt(s.indexes().first().row());
            currentFlight = f.id;
            currentFare = f.fare;
            currentDate = dateEdit->date().toString("yyyy-MM-dd");
            QVector<bool> sold(30, false);
            for (int i = 0; i < qMin(f.seatsSoldCount, 30); i++) sold[i] = true;
            selectedSeats.clear();
            seatWidget->populate(sold);
            seatWidget->setVisible(true);
            bookBtn->setVisible(true);
        });
        connect(seatWidget, &SeatWidget::seatToggled, this, [this](int idx, bool on){
            if (on) { if (!selectedSeats.contains(idx)) selectedSeats.append(idx); }
            else selectedSeats.removeAll(idx);
            fareLabel->setText(QString("Fare (%1 seats): Rs %2")
                .arg(selectedSeats.size())
                .arg(selectedSeats.size() * currentFare, 0, 'f', 2));
        });
        connect(bookBtn, &QPushButton::clicked, this, &MainWindow::onBookConfirm);

        // Load stops
        // Defer stops loading until after the window is shown
    QTimer::singleShot(0, this, [this]{
        QNetworkRequest req{QUrl(serverBase + "/api/search_stops")};
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkReply* reply = net->post(req, QJsonDocument(QJsonObject()).toJson());
        connect(reply, &QNetworkReply::finished, this, [this, reply]{
            if (reply->error() != QNetworkReply::NoError) {
                qDebug() << "search_stops error:" << reply->errorString();
                reply->deleteLater();
                return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isArray()) { reply->deleteLater(); return; }
        fromCombo->clear();
        toCombo->clear();
        for (auto v : doc.array()) {
            QString s = v.toString();
            if (!s.isEmpty()) {
                fromCombo->addItem(s);
                toCombo->addItem(s);
            }
        }
        qDebug() << "Loaded" << fromCombo->count() << "stops";
        reply->deleteLater();
    });
});
    }
    stack->addWidget(bookingPage);

    // ================= TRACK LUGGAGE PAGE =================
    trackPage = new QWidget;
    {
        auto* l = new QVBoxLayout(trackPage);
        auto* top = new QHBoxLayout;
        top->addWidget(backBtn());
        top->addStretch();
        top->addWidget(logoutBtn());
        l->addLayout(top);
        auto* t = new QLabel("Luggage Tracking");
        t->setObjectName("title");
        t->setAlignment(Qt::AlignCenter);
        l->addWidget(t);
        auto* refresh = new QPushButton("Refresh");
        l->addWidget(refresh);
        bagList = new QListWidget;
        l->addWidget(bagList);
        connect(refresh, &QPushButton::clicked, this, &MainWindow::refreshBagList);
    }
    stack->addWidget(trackPage);

    // ================= SIMULATOR PAGE =================
    simulatorPage = new QWidget;
    {
        auto* l = new QVBoxLayout(simulatorPage);
        auto* top = new QHBoxLayout;
        top->addWidget(backBtn());
        top->addStretch();
        top->addWidget(logoutBtn());
        l->addLayout(top);
        auto* t = new QLabel("Simulator");
        t->setObjectName("title");
        t->setAlignment(Qt::AlignCenter);
        l->addWidget(t);

        // Set time
        auto* timeRow = new QHBoxLayout;
        auto* dt = new QDateTimeEdit(QDateTime::currentDateTime());
        dt->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
        auto* setT = new QPushButton("Set Sim Time");
        timeRow->addWidget(new QLabel("Sim Time:"));
        timeRow->addWidget(dt);
        timeRow->addWidget(setT);
        l->addLayout(timeRow);

        connect(setT, &QPushButton::clicked, this, [this, dt]{
            QJsonObject reqObj;
            reqObj["iso"] = dt->dateTime().toString("yyyy-MM-ddTHH:mm:ss");
            auto r = postJson(net, serverBase + "/api/set_time", reqObj);
            QMessageBox::information(this, "Sim Time",
                r.value("ok").toBool() ? "Time set." : "Failed.");
        });

        auto* pushBtn = new QPushButton("Simulate Bag Push");
        auto* lossBtn = new QPushButton("Simulate Bag Loss");
        l->addWidget(pushBtn);
        l->addWidget(lossBtn);

        connect(pushBtn, &QPushButton::clicked, this, &MainWindow::onBagPush);
        connect(lossBtn, &QPushButton::clicked, this, &MainWindow::onBagLoss);

        simBagList = new QListWidget;
        l->addWidget(simBagList);

        auto* safeBtn = new QPushButton("Mark Selected Bag Safe");
        l->addWidget(safeBtn);
        connect(safeBtn, &QPushButton::clicked, this, &MainWindow::onBagSafe);

        l->setAlignment(Qt::AlignTop);
    }
    stack->addWidget(simulatorPage);
    // Auto-refresh bags every second so the belt "moves" visually
QTimer* bagTimer = new QTimer(this);
connect(bagTimer, &QTimer::timeout, this, [this]{
    if (stack->currentWidget() == trackPage ||
        stack->currentWidget() == simulatorPage) {
        refreshBagList();
    }
});
bagTimer->start(1000);
    stack->setCurrentWidget(firstPage);
}

// ---------- navigation ----------
QPushButton* MainWindow::backBtn() {
    auto* b = new QPushButton("< Back");
    b->setObjectName("backBtn");
    connect(b, &QPushButton::clicked, this, &MainWindow::goBack);
    return b;
}

QPushButton* MainWindow::logoutBtn() {
    auto* b = new QPushButton("Logout");
    b->setObjectName("logoutBtn");
    connect(b, &QPushButton::clicked, this, &MainWindow::logout);
    return b;
}

void MainWindow::pushAndGo(QWidget* w) {
    history.append(stack->currentWidget());
    stack->setCurrentWidget(w);
}

void MainWindow::goBack() {
    if (!history.isEmpty()) {
        QWidget* prev = history.takeLast();
        stack->setCurrentWidget(prev);
    } else {
        stack->setCurrentWidget(firstPage);
    }
}

void MainWindow::logout() {
    history.clear();
    loggedInUser.clear();
    loggedInRole.clear();
    stack->setCurrentWidget(firstPage);
}

// ---------- actions ----------
void MainWindow::onPassengerLogin() {
    QJsonObject reqObj;
    reqObj["role"] = "Passenger";
    reqObj["username"] = pUser->text();
    reqObj["password"] = pPass->text();
    auto r = postJson(net, serverBase + "/api/validate", reqObj);
    if (r.value("ok").toBool()) {
        loggedInUser = pUser->text();
        loggedInRole = "Passenger";
        pushAndGo(passengerPortalPage);
    } else {
        QMessageBox::warning(this, "Login", "Invalid credentials.");
    }
}

void MainWindow::onStaffLogin() {
    QJsonObject reqObj;
    reqObj["role"] = "Staff";
    reqObj["username"] = sUser->text();
    reqObj["password"] = sPass->text();
    auto r = postJson(net, serverBase + "/api/validate", reqObj);
    if (r.value("ok").toBool()) {
        loggedInUser = sUser->text();
        loggedInRole = "Staff";
        pushAndGo(staffPortalPage);
    } else {
        QMessageBox::warning(this, "Login", "Invalid credentials.");
    }
}

void MainWindow::onAdminLogin() {
    QJsonObject reqObj;
    reqObj["role"] = "Admin";
    reqObj["username"] = aUser->text();
    reqObj["password"] = aPass->text();
    auto r = postJson(net, serverBase + "/api/validate", reqObj);
    if (r.value("ok").toBool()) pushAndGo(adminPanelPage);
    else QMessageBox::warning(this, "Login", "Invalid admin.");
}

void MainWindow::openSimulator() { pushAndGo(simulatorPage); }
void MainWindow::openBooking() { pushAndGo(bookingPage); }
void MainWindow::openTrackLuggage() { refreshBagList(); pushAndGo(trackPage); }

void MainWindow::onSearch() {
    QJsonObject reqObj;
    reqObj["from"] = fromCombo->currentText();
    reqObj["to"] = toCombo->currentText();
    auto arr = postJsonArr(net, serverBase + "/api/search", reqObj);

    QVector<FlightRecord> flights;
    QStringList routeStops;
    for (auto v : arr) {
        auto o = v.toObject();
        FlightRecord fr;
        fr.id = o["flightNumber"].toString();
        fr.dep = o["origin"].toString();
        fr.arr = o["destination"].toString();
        fr.duration = o["departureTime"].toString();
        fr.fare = o["baseFare"].toDouble();
        fr.seatsSoldCount = o["seatsSoldCount"].toInt();
        flights.append(fr);
        if (routeStops.isEmpty()) routeStops.append(fr.dep);
        routeStops.append(fr.arr);
    }
    routeLabel->setText("Route: " + routeStops.join(" -> "));
    flightModel->setFlights(flights);
    seatWidget->setVisible(false);
    bookBtn->setVisible(false);
}

void MainWindow::onBookConfirm() {
    if (selectedSeats.isEmpty()) {
        QMessageBox::warning(this, "Booking", "Select at least one seat.");
        return;
    }
    QJsonArray seatArr, paxArr;
    for (int i = 0; i < selectedSeats.size(); i++) {
        bool ok = false;
        QString name = QInputDialog::getText(this, "Passenger",
            QString("Name for seat %1:").arg(selectedSeats[i] + 1),
            QLineEdit::Normal, "", &ok);
        if (!ok || name.isEmpty()) return;

        QString passport = QInputDialog::getText(this, "Passenger",
            "Passport number:", QLineEdit::Normal, "", &ok);
        if (!ok) return;

        seatArr.append(QString::number(selectedSeats[i] + 1));
        QJsonObject pax;
        pax["name"] = name;
        pax["passport"] = passport;
        paxArr.append(pax);
    }
    QJsonObject reqObj;
    reqObj["username"] = loggedInUser;
    reqObj["flightNumber"] = currentFlight;
    reqObj["date"] = currentDate;
    reqObj["seats"] = seatArr;
    reqObj["passengers"] = paxArr;
    auto r = postJson(net, serverBase + "/api/book", reqObj);
    if (r.value("ok").toBool()) {
        QMessageBox::information(this, "Booking", "Booking confirmed! Boarding code(s) saved.");
        stack->setCurrentWidget(passengerPortalPage);
    } else {
        QMessageBox::warning(this, "Booking", "Failed: " + r.value("error").toString());
    }
}

void MainWindow::refreshBagList() {
    bagList->clear();
    if (simBagList) simBagList->clear();

    QJsonObject reqObj;
    QString endpoint;
    if (loggedInRole == "Passenger") {
        reqObj["username"] = loggedInUser;
        endpoint = "/api/my_bags";
    } else {
        endpoint = "/api/bag_list";
    }
    auto arr = postJsonArr(net, serverBase + endpoint, reqObj);

    for (auto v : arr) {
        auto o = v.toObject();
        QString status;
        if (o["lost"].toBool()) status = "LOST";
        else if (o["safeRecovered"].toBool()) status = "Safe";
        else status = "In transit";

        QString line = QString("%1 | %2 | Sensor %3 | %4")
            .arg(o["bagId"].toString(), o["flightNumber"].toString())
            .arg(o["currentSensor"].toInt())
            .arg(status);

        bagList->addItem(line);
        if (simBagList) simBagList->addItem(line);
    }
}

void MainWindow::onBagPush() {
    auto arr = postJsonArr(net, serverBase + "/api/flights_within_3h", {});
    if (arr.isEmpty()) {
        QMessageBox::information(this, "Sim", "No flights within 3h of sim time.");
        return;
    }
    QStringList options;
    for (auto v : arr) options.append(v.toObject()["flightNumber"].toString());
    bool ok = false;
    QString flight = QInputDialog::getItem(this, "Sim", "Select flight:", options, 0, false, &ok);
    if (!ok) return;

    QJsonObject reqObj1;
    reqObj1["flightNumber"] = flight;
    auto paxArr = postJsonArr(net, serverBase + "/api/checked_in_list", reqObj1);
    if (paxArr.isEmpty()) {
        QMessageBox::information(this, "Sim", "No checked-in passengers for this flight.");
        return;
    }
    QStringList paxOptions;
    for (auto v : paxArr) {
        auto o = v.toObject();
        paxOptions.append(QString("%1 | %2").arg(o["boardingCode"].toString(), o["username"].toString()));
    }
    QString pick = QInputDialog::getItem(this, "Sim", "Pick a passenger:", paxOptions, 0, false, &ok);
    if (!ok) return;
    int idx = paxOptions.indexOf(pick);
    auto o = paxArr[idx].toObject();

    QJsonObject reqObj2;
    reqObj2["username"] = o["username"].toString();
    reqObj2["flightNumber"] = flight;
    reqObj2["date"] = o["date"].toString();
    reqObj2["passengerName"] = o["username"].toString();
    reqObj2["boardingCode"] = o["boardingCode"].toString();
    auto r = postJson(net, serverBase + "/api/bag_push", reqObj2);
    QMessageBox::information(this, "Sim",
        r.value("ok").toBool() ? "Bag pushed on belt." : "Failed.");
    refreshBagList();
}

void MainWindow::onBagLoss() {
    refreshBagList();
    bool ok = false;
    int sensor = QInputDialog::getInt(this, "Sim",
        "Sensor number to simulate loss at (1-10):", 3, 1, 10, 1, &ok);
    if (!ok) return;
    QString bagId = QInputDialog::getText(this, "Sim",
        "Bag ID to lose (e.g. BAG0001):", QLineEdit::Normal, "", &ok);
    if (!ok || bagId.isEmpty()) return;

    QJsonObject reqObj;
    reqObj["bagId"] = bagId;
    reqObj["sensor"] = sensor;
    auto r = postJson(net, serverBase + "/api/bag_simulate_loss", reqObj);
    QMessageBox::information(this, "Sim",
        r.value("ok").toBool() ? "Loss simulated." : "Bag not found.");
    refreshBagList();
}

void MainWindow::onBagSafe() {
    auto* item = simBagList->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Belt", "Select a bag first.");
        return;
    }
    QString text = item->text();
    QString bagId = text.split(" ").first();
    QJsonObject reqObj;
    reqObj["bagId"] = bagId;
    auto r = postJson(net, serverBase + "/api/bag_mark_safe", reqObj);
    QMessageBox::information(this, "Belt",
        r.value("ok").toBool() ? "Marked safe." : "Failed.");
    refreshBagList();
}