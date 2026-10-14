cmake_minimum_required(VERSION 3.16)
project(flight_server VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(flight_server server.cpp)

target_include_directories(flight_server PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/include
)

if (WIN32)
    target_link_libraries(flight_server PRIVATE ws2_32 iphlpapi)
endif()

set(THIRD_PARTY_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/include")
if (NOT EXISTS "${THIRD_PARTY_INCLUDE}")
    message(FATAL_ERROR "third_party include folder not found: ${THIRD_PARTY_INCLUDE}")
endif()
