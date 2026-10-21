# Qt Widgets client for the Airline & Airport system  (Qt 5.15 or Qt 6)
QT       += core gui widgets network
TARGET    = AirlineClient
TEMPLATE  = app

greaterThan(QT_MAJOR_VERSION, 5) { CONFIG += c++17 } else { CONFIG += c++11 }
msvc { QMAKE_CXXFLAGS += /utf-8 }

# the hand written data structures / sorting algorithms live next to the server
INCLUDEPATH += . ../server

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    homepages.cpp \
    loginwindow.cpp \
    bookingpages.cpp \
    ticketpages.cpp \
    luggagepages.cpp \
    staffpages.cpp \
    simulatorpages.cpp \
    beltwidget.cpp \
    beltdata.cpp \
    flightmodel.cpp \
    seatwidget.cpp \
    serverapi.cpp \
    theme.cpp

HEADERS += \
    basepage.h \
    mainwindow.h \
    homepages.h \
    loginwindow.h \
    bookingpages.h \
    ticketpages.h \
    luggagepages.h \
    staffpages.h \
    simulatorpages.h \
    beltwidget.h \
    beltdata.h \
    flightmodel.h \
    seatwidget.h \
    serverapi.h \
    theme.h
