// server.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace httplib;

struct User {
    std::string username;
    std::string password;
};

// Load users from the specified .dat file
// Helper function to trim whitespace and carriage returns (\r)
static std::string trim(const std::string &s) {
    auto l = s.find_first_not_of(" \t\r\n");
    if (l == std::string::npos) return "";
    auto r = s.find_last_not_of(" \t\r\n");
    return s.substr(l, r - l + 1);
}

// Load users from the specified .dat file with BOM and newline handling
static std::vector<User> loadUsers(const std::string &path) {
    std::vector<User> users;
    std::ifstream ifs(path);
    if (!ifs) return users;

    std::string line;
    bool firstLine = true;

    while (std::getline(ifs, line)) {
        // 1. Handle UTF-8 BOM on the very first line if present (EF BB BF)
        if (firstLine) {
            if (line.size() >= 3 && 
                static_cast<unsigned char>(line[0]) == 0xEF && 
                static_cast<unsigned char>(line[1]) == 0xBB && 
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line.erase(0, 3);
            }
            firstLine = false;
        }

        // 2. Trim whitespace and carriage returns
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        // 3. Parse username and password safely
        auto pos = line.find('|');
        if (pos == std::string::npos) continue;

        std::string username = trim(line.substr(0, pos));
        std::string password = trim(line.substr(pos + 1));

        users.push_back({username, password});
    }
    return users;
}

// Binary Search implementation to find if a user exists and matches the password
static bool binarySearchValidate(std::vector<User>& users, const std::string &username, const std::string &password) {
    // Binary search requires the vector to be sorted by username
    std::sort(users.begin(), users.end(), [](const User& a, const User& b) {
        return a.username < b.username;
    });

    User dummy{username, ""};
    auto it = std::lower_bound(users.begin(), users.end(), dummy, [](const User& a, const User& b) {
        return a.username < b.username;
    });

    // Check if found and password matches
    if (it != users.end() && it->username == username) {
        std::cout << it->username << username << password << it->password << std::endl;
        return (it->password == password);
    }
    return false;
}

// Binary Search just to check if username exists (used during registration)
static bool binarySearchExists(std::vector<User>& users, const std::string &username) {
    std::sort(users.begin(), users.end(), [](const User& a, const User& b) {
        return a.username < b.username;
    });

    User dummy{username, ""};
    auto it = std::lower_bound(users.begin(), users.end(), dummy, [](const User& a, const User& b) {
        return a.username < b.username;
    });

    return (it != users.end() && it->username == username);
}

int main(int argc, char** argv) {
    int port = 8080;
    if (argc > 1) port = std::stoi(argv[1]);

    Server svr;

    // CORS handler for local connectivity
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

    // Endpoint: Validate User Login
    svr.Post("/api/validate", [&](const Request &req, Response &res) {
        try {
            auto body = json::parse(req.body);
            std::string role = body.value("role", "Passenger");
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");
            std::cout << username << " " << password << std::endl;
            std::string filename = (role == "Staff") ? "staff_users.dat" : "passengers_users.dat";
            auto users = loadUsers(filename);

            bool ok = binarySearchValidate(users, username, password);
            json out = { {"ok", ok} };
            res.set_content(out.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"ok":false,"error":"bad_request"})", "application/json");
        }
    });

    // Endpoint: Register New User
    svr.Post("/api/register", [&](const Request &req, Response &res) {
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

            // Check if user already exists using binary search
            if (binarySearchExists(users, username)) {
                res.set_content(R"({"ok":false,"error":"user_exists"})", "application/json");
                return;
            }

            // Append new user to the respective .dat file
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

    std::cout << "Server listening on port " << port << " using separate passenger/staff .dat files with Binary Search.\n";
    svr.listen("0.0.0.0", port);
    return 0;
}