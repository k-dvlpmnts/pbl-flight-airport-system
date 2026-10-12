// server.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <mutex>
#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace httplib;

struct User {
    std::string username;
    std::string password;
};

struct Flight {
    std::string flightNumber;
    std::string origin;
    std::string destination;
    std::string departureTime; // ISO format "YYYY-MM-DDTHH:MM:SS"
    double baseFare;
    int totalSeats;
    int seatsSoldCount;
};

static std::mutex dbMutex;

// Trim utility
static std::string trim(const std::string &s) {
    auto l = s.find_first_not_of(" \t\r\n");
    if (l == std::string::npos) return "";
    auto r = s.find_last_not_of(" \t\r\n");
    return s.substr(l, r - l + 1);
}

// Load users with BOM stripping
static std::vector<User> loadUsers(const std::string &path) {
    std::vector<User> users;
    std::ifstream ifs(path);
    if (!ifs) return users;
    std::string line;
    bool firstLine = true;
    while (std::getline(ifs, line)) {
        if (firstLine) {
            if (line.size() >= 3 && 
                static_cast<unsigned char>(line[0]) == 0xEF && 
                static_cast<unsigned char>(line[1]) == 0xBB && 
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line.erase(0, 3);
            }
            firstLine = false;
        }
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        auto pos = line.find('|');
        if (pos == std::string::npos) continue;
        users.push_back({trim(line.substr(0, pos)), trim(line.substr(pos + 1))});
    }
    return users;
}

// Load flights from flights.dat
static std::vector<Flight> loadFlights() {
    std::vector<Flight> flights;
    std::ifstream ifs("flights.dat");
    if (!ifs) {
        std::ofstream ofs("flights.dat");
        ofs << "# flightNumber|origin|destination|departureTime|baseFare|totalSeats|seatsSoldCount\n";
        ofs << "AI101|DEL|BOM|2026-11-10T08:00:00|4500.0|30|5\n";
        ofs << "AI202|BOM|BLR|2026-11-12T10:30:00|3200.0|30|2\n";
        ofs << "AI303|DEL|BLR|2026-11-15T14:00:00|7000.0|30|10\n";
        ofs.close();
        ifs.open("flights.dat");
    }
    std::string line;
    bool firstLine = true;
    while (std::getline(ifs, line)) {
        if (firstLine) {
            if (line.size() >= 3 && 
                static_cast<unsigned char>(line[0]) == 0xEF && 
                static_cast<unsigned char>(line[1]) == 0xBB && 
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line.erase(0, 3);
            }
            firstLine = false;
        }
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        std::string fNum, org, dst, depTime, fareStr, tSeatsStr, sSoldCountStr;
        if (std::getline(ss, fNum, '|') &&
            std::getline(ss, org, '|') &&
            std::getline(ss, dst, '|') &&
            std::getline(ss, depTime, '|') &&
            std::getline(ss, fareStr, '|') &&
            std::getline(ss, tSeatsStr, '|') &&
            std::getline(ss, sSoldCountStr, '|')) {
            Flight f;
            f.flightNumber = trim(fNum);
            f.origin = trim(org);
            f.destination = trim(dst);
            f.departureTime = trim(depTime);
            f.baseFare = std::stod(trim(fareStr));
            f.totalSeats = std::stoi(trim(tSeatsStr));
            f.seatsSoldCount = std::stoi(trim(sSoldCountStr));
            flights.push_back(f);
        }
    }
    return flights;
}

// Helper to parse ISO time string into std::chrono::system_clock::time_point
static std::chrono::system_clock::time_point parseIsoTime(const std::string &isoStr) {
    std::tm tm = {};
    std::stringstream ss(isoStr);
    char dash1, char2, tChar, colon1, colon2;
    ss >> tm.tm_year >> dash1 >> tm.tm_mon >> char2 >> tm.tm_mday >> tChar >> tm.tm_hour >> colon1 >> tm.tm_min >> colon2 >> tm.tm_sec;
    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
    tm.tm_isdst = -1;
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

// Core Data Structures Graph & Dijkstra's Algorithm (Vector-mapped instead of hash maps)
struct Edge {
    int toIndex;
    double weight;
    std::string flightNum;
};

struct GraphResult {
    double totalCost;
    std::vector<std::string> path;
    std::vector<std::string> flightNumbers;
};

// Binary search to find station index in sorted unique stations vector
static int getStationIndex(const std::vector<std::string> &stations, const std::string &name) {
    auto it = std::lower_bound(stations.begin(), stations.end(), name);
    if (it != stations.end() && *it == name) {
        return std::distance(stations.begin(), it);
    }
    return -1;
}

static GraphResult findShortestRoute(const std::string &start, const std::string &target, const std::vector<Flight> &flights) {
    // 1. Collect all unique station names for indexing
    std::vector<std::string> stations;
    for (const auto &f : flights) {
        stations.push_back(f.origin);
        stations.push_back(f.destination);
    }
    std::sort(stations.begin(), stations.end());
    stations.erase(std::unique(stations.begin(), stations.end()), stations.end());

    int startIndex = getStationIndex(stations, start);
    int targetIndex = getStationIndex(stations, target);

    GraphResult result;
    result.totalCost = -1;
    if (startIndex == -1 || targetIndex == -1) return result;

    int n = stations.size();
    // 2. Adjacency list represented as vector of vectors using integer node indices
    std::vector<std::vector<Edge>> adj(n);
    for (const auto &f : flights) {
        int u = getStationIndex(stations, f.origin);
        int v = getStationIndex(stations, f.destination);
        if (u != -1 && v != -1) {
            adj[u].push_back({v, f.baseFare, f.flightNumber});
        }
    }

    // 3. Dijkstra's Algorithm using flat vectors instead of unordered_map
    const double INF = 1e9;
    std::vector<double> dist(n, INF);
    std::vector<int> parent(n, -1);
    std::vector<std::string> edgeFlight(n, "");

    // Min-heap priority queue: {cost, nodeIndex}
    using P = std::pair<double, int>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;

    dist[startIndex] = 0.0;
    pq.push({0.0, startIndex});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        if (d > dist[u]) continue;
        if (u == targetIndex) break;

        for (const auto &edge : adj[u]) {
            if (dist[u] + edge.weight < dist[edge.toIndex]) {
                dist[edge.toIndex] = dist[u] + edge.weight;
                parent[edge.toIndex] = u;
                edgeFlight[edge.toIndex] = edge.flightNum;
                pq.push({dist[edge.toIndex], edge.toIndex});
            }
        }
    }

    if (dist[targetIndex] >= INF - 1e5) {
        return result;
    }

    // 4. Reconstruct shortest path
    result.totalCost = dist[targetIndex];
    int curr = targetIndex;
    while (curr != startIndex) {
        result.path.push_back(stations[curr]);
        result.flightNumbers.push_back(edgeFlight[curr]);
        curr = parent[curr];
    }
    result.path.push_back(stations[startIndex]);
    std::reverse(result.path.begin(), result.path.end());
    std::reverse(result.flightNumbers.begin(), result.flightNumbers.end());
    return result;
}

int main(int argc, char** argv) {
    int port = 8080;
    if (argc > 1) port = std::stoi(argv[1]);

    Server svr;

    svr.set_pre_routing_handler([](const Request &req, Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        if (req.method == "OPTIONS") {
            res.status = 200;
            return Server::HandlerResponse::Handled;
        }
        return Server::HandlerResponse::Unhandled;
    });

    // Endpoint: Search stops
    svr.Post("/api/search_stops", [&](const Request &, Response &res) {
        std::lock_guard<std::mutex> lock(dbMutex);
        auto flights = loadFlights();
        std::vector<std::string> stopsList;
        for (const auto &f : flights) {
            stopsList.push_back(f.origin);
            stopsList.push_back(f.destination);
        }
        std::sort(stopsList.begin(), stopsList.end());
        stopsList.erase(std::unique(stopsList.begin(), stopsList.end()), stopsList.end());

        json arr = json::array();
        for (const auto &s : stopsList) arr.push_back(s);
        res.set_content(arr.dump(), "application/json");
    });

    // Endpoint: Search Shortest Route & Flights
    svr.Post("/api/search", [&](const Request &req, Response &res) {
        std::lock_guard<std::mutex> lock(dbMutex);
        try {
            auto body = json::parse(req.body);
            std::string from = body.value("from", "");
            std::string to = body.value("to", "");

            auto flights = loadFlights();
            GraphResult route = findShortestRoute(from, to, flights);

            json jArr = json::array();
            if (route.totalCost > 0) {
                for (size_t i = 0; i < route.flightNumbers.size(); ++i) {
                    std::string fNum = route.flightNumbers[i];
                    auto it = std::find_if(flights.begin(), flights.end(), [&](const Flight &f){ return f.flightNumber == fNum; });
                    if (it != flights.end()) {
                        json obj;
                        obj["flightNumber"] = it->flightNumber;
                        obj["origin"] = route.path[i];
                        obj["destination"] = route.path[i+1];
                        obj["departureTime"] = it->departureTime;
                        obj["baseFare"] = it->baseFare;
                        obj["seatsSoldCount"] = it->seatsSoldCount;
                        obj["totalSeats"] = it->totalSeats;
                        jArr.push_back(obj);
                    }
                }
            }
            res.set_content(jArr.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"invalid_request"})", "application/json");
        }
    });

    // Endpoint: Book Ticket
    svr.Post("/api/book", [&](const Request &req, Response &res) {
        std::lock_guard<std::mutex> lock(dbMutex);
        try {
            auto body = json::parse(req.body);
            std::string username = body.value("username", "");
            std::string flightNum = body.value("flightNumber", "");
            int seatCount = body.value("seatCount", 1);
            double totalFare = body.value("totalFare", 0.0);

            auto flights = loadFlights();
            auto it = std::find_if(flights.begin(), flights.end(), [&](const Flight &f){ return f.flightNumber == flightNum; });
            if (it == flights.end()) {
                res.set_content(R"({"ok":false,"error":"flight_not_found"})", "application/json");
                return;
            }

            auto now = std::chrono::system_clock::now();
            auto depTime = parseIsoTime(it->departureTime);
            auto oneMonthBefore = depTime - std::chrono::hours(24 * 30);

            if (now < oneMonthBefore) {
                res.set_content(R"({"ok":false,"error":"booking_not_open_yet"})", "application/json");
                return;
            }
            if (now > depTime) {
                res.set_content(R"({"ok":false,"error":"flight_already_departed"})", "application/json");
                return;
            }

            if (it->seatsSoldCount + seatCount > it->totalSeats) {
                res.set_content(R"({"ok":false,"error":"seats_full"})", "application/json");
                return;
            }

            it->seatsSoldCount += seatCount;
            std::ofstream ofs("flights.dat", std::ios::trunc);
            ofs << "# flightNumber|origin|destination|departureTime|baseFare|totalSeats|seatsSoldCount\n";
            for (const auto &f : flights) {
                ofs << f.flightNumber << "|" << f.origin << "|" << f.destination << "|" << f.departureTime << "|" << f.baseFare << "|" << f.totalSeats << "|" << f.seatsSoldCount << "\n";
            }
            ofs.close();

            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::ofstream ticketFile("booked_tickets.dat", std::ios::app);
            ticketFile << username << "|" << flightNum << "|" << seatCount << "|" << totalFare << "|" << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S") << "\n";
            ticketFile.close();

            res.set_content(R"({"ok":true})", "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"ok":false,"error":"bad_request"})", "application/json");
        }
    });

    // Endpoint: User Validation (Binary Search)
    svr.Post("/api/validate", [&](const Request &req, Response &res) {
        std::lock_guard<std::mutex> lock(dbMutex);
        try {
            auto body = json::parse(req.body);
            std::string role = body.value("role", "Passenger");
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");

            std::string filename = (role == "Staff") ? "staff_users.dat" : "passengers_users.dat";
            auto users = loadUsers(filename);
            std::sort(users.begin(), users.end(), [](const User& a, const User& b){ return a.username < b.username; });

            User dummy{username, ""};
            auto it = std::lower_bound(users.begin(), users.end(), dummy, [](const User& a, const User& b){ return a.username < b.username; });

            bool ok = (it != users.end() && it->username == username && it->password == password);
            json out = { {"ok", ok} };
            res.set_content(out.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"ok":false,"error":"bad_request"})", "application/json");
        }
    });

    // Endpoint: Register User
    svr.Post("/api/register", [&](const Request &req, Response &res) {
        std::lock_guard<std::mutex> lock(dbMutex);
        try {
            auto body = json::parse(req.body);
            std::string role = body.value("role", "Passenger");
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");

            if (username.empty() || password.empty()) {
                res.set_content(R"({"ok":false,"error":"empty_fields"})", "application/json");
                return;
            }

            std::string filename = (role == "Staff") ? "staff_users.dat" : "passengers_users.dat";
            auto users = loadUsers(filename);
            std::sort(users.begin(), users.end(), [](const User& a, const User& b){ return a.username < b.username; });

            User dummy{username, ""};
            auto it = std::lower_bound(users.begin(), users.end(), dummy, [](const User& a, const User& b){ return a.username < b.username; });

            if (it != users.end() && it->username == username) {
                res.set_content(R"({"ok":false,"error":"user_exists"})", "application/json");
                return;
            }

            std::ofstream ofs(filename, std::ios::app);
            if (ofs.is_open()) {
                ofs << username << "|" << password << "\n";
                ofs.close();
                res.set_content(R"({"ok":true})", "application/json");
            } else {
                res.status = 500;
                res.set_content(R"({"ok":false,"error":"file_error"})", "application/json");
            }
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"ok":false,"error":"bad_request"})", "application/json");
        }
    });

    std::cout << "Advanced Vector-Graph Server running on port " << port << " with zero unordered_maps.\n";
    svr.listen("0.0.0.0", port);
    return 0;
}