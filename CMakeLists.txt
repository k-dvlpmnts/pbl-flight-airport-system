cmake_minimum_required(VERSION 3.16)
project(FlightAirportSystem LANGUAGES CXX)

# --- Qt setup ---
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- GUI executable ---
add_executable(flight_airport_system
    main.cpp
    mainwindow.cpp
    mainwindow.h
    flightmodel.cpp
    flightmodel.h
    seatwidget.cpp
    seatwidget.h
    loginwindow.cpp
    loginwindow.h
)

target_link_libraries(flight_airport_system PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

# --- Compiler warnings (optional) ---
if (MSVC)
    target_compile_options(flight_airport_system PRIVATE /W4 /permissive-)
else()
    target_compile_options(flight_airport_system PRIVATE -Wall -Wextra -pedantic)
endif()

# --- Output message ---
message(STATUS "Configured FlightAirportSystem with Qt${QT_VERSION_MAJOR}")
