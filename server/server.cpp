// server.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include "httplib.h"    // put httplib.h in include path or use FetchContent
#include "json.hpp"     // nlohmann/json single header

using json = nlohmann::json;
using namespace httplib;

struct User {
    std::string username;
    std::string password;
};

struct Stop {
    std::string name;
    std::string timeIso;
    double distanceKm; // distance from previous stop
};

struct Flight {
    std::string flightNumber;
    double pricePerKm;
    std::vector<Stop> stops;
};

static std::vector<User> loadUsers(const std::string &path) {
    std::vector<User> users;
    std::ifstream ifs(path);
    if (!ifs) return users;
    
    std::string line;
    bool firstLine = true;

    while (std::getline(ifs, line)) {
        // Strip out the 3-byte UTF-8 BOM if it exists on the first line
        if (firstLine) {
            firstLine = false;
            if (line.size() >= 3 && 
                (unsigned char)line[0] == 0xEF && 
                (unsigned char)line[1] == 0xBB && 
                (unsigned char)line[2] == 0xBF) {
                line = line.substr(3); // Keep everything after the 3 BOM bytes
            }
        }

        if (line.empty() || line[0] == '#') continue;
        auto pos = line.find('|');
        if (pos == std::string::npos) continue;
        users.push_back({line.substr(0, pos), line.substr(pos + 1)});
    }
    return users;
}

static std::vector<Flight> loadFlights(const std::string &path) {
    std::vector<Flight> flights;
    std::ifstream ifs(path);
    if (!ifs) return flights;
    std::string line;
    Flight cur;
    bool inFlight = false;
    while (std::getline(ifs, line)) {
        if (line.empty()) {
            if (inFlight) {
                flights.push_back(cur);
                cur = Flight{};
                inFlight = false;
            }
            continue;
        }
        if (line[0] == '#') continue;
        if (!inFlight) {
            auto pos = line.find('|');
            if (pos == std::string::npos) continue;
            cur.flightNumber = line.substr(0,pos);
            cur.pricePerKm = std::stod(line.substr(pos+1));
            cur.stops.clear();
            inFlight = true;
        } else {
            std::stringstream ss(line);
            std::string name, timeIso, distStr;
            if (!std::getline(ss, name, ',')) continue;
            if (!std::getline(ss, timeIso, ',')) continue;
            if (!std::getline(ss, distStr)) continue;
            Stop s{name, timeIso, std::stod(distStr)};
            cur.stops.push_back(s);
        }
    }
    if (inFlight) flights.push_back(cur);
    return flights;
}

static bool validateUser(const std::vector<User>& users, const std::string &username, const std::string &password) {
    for (const auto &u : users) {
        if (u.username == username && u.password == password) return true;
    }
    return false;
}

static json flightToJson(const Flight &f) {
    json j;
    j["flightNumber"] = f.flightNumber;
    j["pricePerKm"] = f.pricePerKm;
    double totalDistance = 0;
    for (auto &s : f.stops) totalDistance += s.distanceKm;
    j["totalDistanceKm"] = totalDistance;
    j["stops"] = json::array();
    for (auto &s : f.stops) {
        j["stops"].push_back({
            {"name", s.name},
            {"time", s.timeIso},
            {"distanceFromPrevKm", s.distanceKm}
        });
    }
    j["baseFare"] = totalDistance * f.pricePerKm;
    return j;
}

int main(int argc, char** argv) {
    std::string usersFile = "users.dat";
    std::string flightsFile = "flights.dat";
    int port = 8080;
    if (argc > 1) port = std::stoi(argv[1]);
    if (argc > 2) usersFile = argv[2];
    if (argc > 3) flightsFile = argv[3];

    Server svr;

    // CORS for local testing
    svr.set_pre_routing_handler(
        [](const Request &req, Response &res) -> Server::HandlerResponse {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
            if (req.method == "OPTIONS") {
                res.status = 200;
                return Server::HandlerResponse::Handled;
            }
            return Server::HandlerResponse::Unhandled;
        }
    );

    // Validate user endpoint: returns {"ok":true} or {"ok":false}
    svr.Post("/api/validate", [&](const Request &req, Response &res) {
        try {
            auto body = json::parse(req.body);
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");
            auto users = loadUsers(usersFile);
            bool ok = validateUser(users, username, password);
            json out = { {"ok", ok} };
            res.set_content(out.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"ok":false,"error":"bad_request"})", "application/json");
        }
    });

    // Search flights endpoint
    svr.Post("/api/search", [&](const Request &req, Response &res) {
        try {
            auto body = json::parse(req.body);
            std::string from = body.value("from", "");
            std::string to = body.value("to", "");
            std::string date = body.value("date", "");
            auto flights = loadFlights(flightsFile);
            json out = json::array();
            for (auto &f : flights) {
                if (!from.empty()) {
                    if (f.stops.empty() || f.stops.front().name != from) continue;
                }
                if (!to.empty()) {
                    bool found = false;
                    for (size_t i = 1; i < f.stops.size(); ++i) {
                        if (f.stops[i].name == to) { found = true; break; }
                    }
                    if (!found) continue;
                }
                if (!date.empty()) {
                    bool dateMatch = false;
                    for (auto &s : f.stops) {
                        if (s.timeIso.find(date) != std::string::npos) { dateMatch = true; break; }
                    }
                    if (!dateMatch) continue;
                }
                out.push_back(flightToJson(f));
            }
            res.set_content(out.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"bad_request"})", "application/json");
        }
    });

    std::cout << "Server listening on 0.0.0.0:" << port << " (users=" << usersFile << ", flights=" << flightsFile << ")\n";
    svr.listen("0.0.0.0", port);
    return 0;
}
