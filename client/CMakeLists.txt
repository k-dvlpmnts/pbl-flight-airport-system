cmake_minimum_required(VERSION 3.16)
project(AirlineClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

add_executable(AirlineClient
    main.cpp
    mainwindow.cpp mainwindow.h
    basepage.h
    homepages.cpp homepages.h
    loginwindow.cpp loginwindow.h
    bookingpages.cpp bookingpages.h
    ticketpages.cpp ticketpages.h
    luggagepages.cpp luggagepages.h
    staffpages.cpp staffpages.h
    simulatorpages.cpp simulatorpages.h
    beltwidget.cpp beltwidget.h
    beltdata.cpp beltdata.h
    flightmodel.cpp flightmodel.h
    seatwidget.cpp seatwidget.h
    serverapi.cpp serverapi.h
    theme.cpp theme.h
)

# data_structures.cpp / sorting_algorithms.cpp are shared with the server (header-style .cpp files)
target_include_directories(AirlineClient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../server)
target_link_libraries(AirlineClient PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

if(MSVC)
    target_compile_options(AirlineClient PRIVATE /utf-8)
endif()
if(WIN32)
    set_target_properties(AirlineClient PROPERTIES WIN32_EXECUTABLE ON)
endif()
