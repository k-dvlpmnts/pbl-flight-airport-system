// server.cpp — single file backend
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <mutex>
#include <iomanip>
#include <sys/stat.h>
#include <sys/types.h>
#include "httplib.h"
#include "json.hpp"
#include "data_structures.h"
#include "sorting_algorithms.h"

using json = nlohmann::json;
using namespace httplib;
using namespace DS;
using namespace std;

// ============== CONFIG ==============
static string DATA_DIR = "data/";
static string BOOK_DIR = "data/bookings/";
static const int BAG_BELT_LENGTH_M = 100;
static const double BELT_SPEED_MPS = 0.5;
static const int PHOTON_COUNT = 10;

// ============== SIM TIME ==============
static mutex g_mtx;
static time_t g_simTime = 0;
static bool g_simTimeSet = false;

static time_t nowT() {
    if (g_simTimeSet) return g_simTime;
    return time(nullptr);
}

static string fmtTime(time_t t, const char* f = "%Y-%m-%d %H:%M:%S") {
    char buf[64];
    struct tm* tm = localtime(&t);
    strftime(buf, sizeof(buf), f, tm);
    return string(buf);
}

static time_t parseIso(const string& s) {
    if (s.size() < 19) return 0;
    struct tm tm = {};
    tm.tm_year  = atoi(s.substr(0, 4).c_str()) - 1900;
    tm.tm_mon   = atoi(s.substr(5, 2).c_str()) - 1;
    tm.tm_mday  = atoi(s.substr(8, 2).c_str());
    tm.tm_hour  = atoi(s.substr(11, 2).c_str());
    tm.tm_min   = atoi(s.substr(14, 2).c_str());
    tm.tm_sec   = atoi(s.substr(17, 2).c_str());
    tm.tm_isdst = -1;
    return mktime(&tm);
}

// ============== DOMAIN MODELS ==============
struct User {
    string username, password, role;
};

struct Flight {
    string flightNumber, origin, destination;
    string departureTime;
    double baseFare;
    int totalSeats;
};

struct BookingRec {
    string username, passengerName, passport, seatNo, boardingCode;
};

struct BagRec {
    string bagId;
    string username;
    string flightNumber;
    string passengerName;
    string boardingCode;
    int currentSensor;
    time_t pushTime;
    bool lost;
    bool safeRecovered;

    bool operator==(const BagRec& o) const { return bagId == o.bagId; }
};

// ============== FILE HELPERS ==============
static string trim(const string& s) {
    size_t l = s.find_first_not_of(" \t\r\n");
    if (l == string::npos) return "";
    size_t r = s.find_last_not_of(" \t\r\n");
    return s.substr(l, r - l + 1);
}

static void ensureDir(const string& p) {
#ifdef _WIN32
    mkdir(p.c_str());
#else
    mkdir(p.c_str(), 0755);
#endif
}

static void stripBom(string& line, bool& first) {
    if (first) {
        if (line.size() >= 3 &&
            (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB &&
            (unsigned char)line[2] == 0xBF) {
            line.erase(0, 3);
        }
        first = false;
    }
}

// ============== USER LOADING / SEARCH ==============
static int loadUsers(const string& path, User* out, int maxN) {
    ifstream f(path.c_str());
    if (!f) return 0;
    string line;
    int n = 0;
    bool first = true;
    while (getline(f, line) && n < maxN) {
        stripBom(line, first);
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        string parts[3];
        int pc = 0;
        size_t start = 0;
        for (size_t i = 0; i <= line.size() && pc < 3; i++) {
            if (i == line.size() || line[i] == '|') {
                parts[pc++] = trim(line.substr(start, i - start));
                start = i + 1;
            }
        }
        out[n].username = parts[0];
        out[n].password = parts[1];
        out[n].role = (pc >= 3) ? parts[2] : "";
        n++;
    }
    return n;
}

static bool userLess(const User& a, const User& b) {
    return a.username < b.username;
}

static User* findUser(User* arr, int n, const string& uname) {
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        int m = lo + (hi - lo) / 2;
        if (arr[m].username == uname) return &arr[m];
        if (arr[m].username < uname) lo = m + 1;
        else hi = m - 1;
    }
    return nullptr;
}

// ============== FLIGHTS ==============
static int loadFlights(Flight* out, int maxN) {
    ifstream f((DATA_DIR + "flights.dat").c_str());
    if (!f) return 0;
    string line;
    int n = 0;
    bool first = true;
    while (getline(f, line) && n < maxN) {
        stripBom(line, first);
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        string p[6];
        int pc = 0;
        size_t s = 0;
        for (size_t i = 0; i <= line.size() && pc < 6; i++) {
            if (i == line.size() || line[i] == '|') {
                p[pc++] = trim(line.substr(s, i - s));
                s = i + 1;
            }
        }
        if (pc < 6) continue;
        out[n].flightNumber = p[0];
        out[n].origin = p[1];
        out[n].destination = p[2];
        out[n].departureTime = p[3];
        out[n].baseFare = atof(p[4].c_str());
        out[n].totalSeats = atoi(p[5].c_str());
        n++;
    }
    return n;
}

// ============== BAG BELT STATE ==============
static DoublyLinkedList<BagRec> g_belt;
static mutex g_beltMtx;
static int g_nextBagId = 1;

static void computeSensorTimes(time_t pushTime, time_t* outTimes, int& n) {
    n = PHOTON_COUNT;
    for (int i = 0; i < PHOTON_COUNT; i++) {
        double pos = (double)(i + 1) * BAG_BELT_LENGTH_M / (PHOTON_COUNT + 1);
        double dt = pos / BELT_SPEED_MPS;
        outTimes[i] = pushTime + (time_t)dt;
    }
}
// Compute how far along the belt a bag is, based on elapsed time
static int computeCurrentSensor(time_t pushTime, time_t now) {
    double elapsed = difftime(now, pushTime);
    if (elapsed < 0) return 0;
    double distance = elapsed * BELT_SPEED_MPS;
    if (distance >= BAG_BELT_LENGTH_M) return PHOTON_COUNT + 1; // off the end

    for (int i = 0; i < PHOTON_COUNT; i++) {
        double pos = (double)(i + 1) * BAG_BELT_LENGTH_M / (PHOTON_COUNT + 1);
        if (distance < pos) return i; // hasn't reached sensor (i+1) yet
    }
    return PHOTON_COUNT;
}
// ============== BOOKINGS ==============
static string bookingFilePath(const string& flightNum, const string& dateStr) {
    return BOOK_DIR + dateStr + "_" + flightNum + ".dat";
}

static string getDatePart(const string& iso) {
    if (iso.size() < 10) return "";
    return iso.substr(0, 10);
}

static int loadBookings(const string& path, BookingRec* out, int maxN) {
    ifstream f(path.c_str());
    if (!f) return 0;
    string line;
    int n = 0;
    bool first = true;
    while (getline(f, line) && n < maxN) {
        stripBom(line, first);
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        string p[5];
        int pc = 0;
        size_t s = 0;
        for (size_t i = 0; i <= line.size() && pc < 5; i++) {
            if (i == line.size() || line[i] == '|') {
                p[pc++] = trim(line.substr(s, i - s));
                s = i + 1;
            }
        }
        if (pc < 5) continue;
        out[n].username = p[0];
        out[n].passengerName = p[1];
        out[n].passport = p[2];
        out[n].seatNo = p[3];
        out[n].boardingCode = p[4];
        n++;
    }
    return n;
}

static void saveBookings(const string& path, BookingRec* arr, int n) {
    ofstream f(path.c_str(), ios::trunc);
    f << "# username|passengerName|passport|seatNo|boardingCode\n";
    for (int i = 0; i < n; i++) {
        f << arr[i].username << "|"
          << arr[i].passengerName << "|"
          << arr[i].passport << "|"
          << arr[i].seatNo << "|"
          << arr[i].boardingCode << "\n";
    }
}

static int countSoldSeats(const string& flightNum, const string& dateStr) {
    BookingRec tmp[500];
    return loadBookings(bookingFilePath(flightNum, dateStr), tmp, 500);
}

// ============== CHECKED-IN PASSENGERS ==============
struct CheckedIn {
    string flightNum, dateStr, username, boardingCode, seatNo;
    time_t checkTime;
};

static int loadCheckedIn(CheckedIn* out, int maxN) {
    ifstream f((DATA_DIR + "checked_in.dat").c_str());
    if (!f) return 0;
    string line;
    int n = 0;
    bool first = true;
    while (getline(f, line) && n < maxN) {
        stripBom(line, first);
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        string p[6];
        int pc = 0;
        size_t s = 0;
        for (size_t i = 0; i <= line.size() && pc < 6; i++) {
            if (i == line.size() || line[i] == '|') {
                p[pc++] = trim(line.substr(s, i - s));
                s = i + 1;
            }
        }
        if (pc < 6) continue;
        out[n].flightNum    = p[0];
        out[n].dateStr      = p[1];
        out[n].username     = p[2];
        out[n].boardingCode = p[3];
        out[n].seatNo       = p[4];
        out[n].checkTime    = (time_t)atoll(p[5].c_str());
        n++;
    }
    return n;
}

static void appendCheckedIn(const CheckedIn& c) {
    ofstream f((DATA_DIR + "checked_in.dat").c_str(), ios::app);
    f << c.flightNum << "|" << c.dateStr << "|" << c.username << "|"
      << c.boardingCode << "|" << c.seatNo << "|" << (long long)c.checkTime << "\n";
}

// ============== BAGS PERSISTENCE ==============
static void saveBags() {
    ofstream f((DATA_DIR + "bags.dat").c_str(), ios::trunc);
    f << "# bagId|username|flightNumber|passengerName|boardingCode|sensor|pushTime|lost|safe\n";
    DNode<BagRec>* n = g_belt.getHead();
    while (n) {
        const BagRec& b = n->data;
        f << b.bagId << "|" << b.username << "|" << b.flightNumber << "|"
          << b.passengerName << "|" << b.boardingCode << "|"
          << b.currentSensor << "|" << (long long)b.pushTime << "|"
          << (b.lost ? 1 : 0) << "|" << (b.safeRecovered ? 1 : 0) << "\n";
        n = n->next;
    }
}

// ============== UTIL ==============
static string makeBoardingCode(const string& flightNum, const string& seat) {
    static int counter = 0;
    counter++;
    char buf[64];
    snprintf(buf, sizeof(buf), "%s-%s-%05d",
             flightNum.c_str(), seat.c_str(),
             counter + (int)(nowT() % 100000));
    return string(buf);
}

// ============== GRAPH / DIJKSTRA ==============
struct GraphResult {
    double cost;
    string path[10];
    string flights[10];
    int len;
};

static int stationIdx(string* stops, int n, const string& s) {
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        int m = lo + (hi - lo) / 2;
        if (stops[m] == s) return m;
        if (stops[m] < s) lo = m + 1;
        else hi = m - 1;
    }
    return -1;
}

static GraphResult shortestRoute(const string& start, const string& target,
                                 Flight* flights, int fn) {
    GraphResult r;
    r.cost = -1;
    r.len = 0;
    if (fn == 0) return r;

    string stops[500];
    int sn = 0;
    for (int i = 0; i < fn; i++) {
        stops[sn++] = flights[i].origin;
        stops[sn++] = flights[i].destination;
    }
    for (int i = 0; i < sn - 1; i++)
        for (int j = 0; j < sn - i - 1; j++)
            if (stops[j] > stops[j + 1]) {
                string t = stops[j]; stops[j] = stops[j + 1]; stops[j + 1] = t;
            }
    int un = 0;
    for (int i = 0; i < sn; i++)
        if (i == 0 || stops[i] != stops[i - 1]) stops[un++] = stops[i];
    sn = un;

    int sIdx = stationIdx(stops, sn, start);
    int tIdx = stationIdx(stops, sn, target);
    if (sIdx == -1 || tIdx == -1) return r;

    Graph g(sn);
    for (int i = 0; i < fn; i++) {
        int u = stationIdx(stops, sn, flights[i].origin);
        int v = stationIdx(stops, sn, flights[i].destination);
        if (u != -1 && v != -1)
            g.addEdge(u, v, flights[i].baseFare, flights[i].flightNumber.c_str());
    }

    const double INF = 1e18;
    double* dist = new double[sn];
    int* parent = new int[sn];
    string* edgeFlt = new string[sn];
    for (int i = 0; i < sn; i++) {
        dist[i] = INF;
        parent[i] = -1;
        edgeFlt[i] = "";
    }

    PriorityQueue<int> pq(sn * 10);
    dist[sIdx] = 0;
    pq.push(sIdx, 0);

    while (!pq.isEmpty()) {
        int u;
        pq.pop(u);
        GraphEdge* e = g.adj[u];
        while (e) {
            if (dist[u] + e->weight < dist[e->to]) {
                dist[e->to] = dist[u] + e->weight;
                parent[e->to] = u;
                edgeFlt[e->to] = string(e->flightNum);
                pq.push(e->to, (int)dist[e->to]);
            }
            e = e->next;
        }
    }

    if (dist[tIdx] >= INF - 1) {
        delete[] dist; delete[] parent; delete[] edgeFlt;
        return r;
    }

    r.cost = dist[tIdx];
    string revPath[10];
    string revFlights[10];
    int rn = 0;
    int cur = tIdx;
    while (cur != sIdx && rn < 10) {
        revPath[rn] = stops[cur];
        revFlights[rn] = edgeFlt[cur];
        rn++;
        cur = parent[cur];
    }
    revPath[rn] = stops[sIdx];

    for (int i = 0; i <= rn; i++) r.path[i] = revPath[rn - i];
    for (int i = 0; i < rn; i++) r.flights[i] = revFlights[rn - 1 - i];
    r.len = rn + 1;

    delete[] dist; delete[] parent; delete[] edgeFlt;
    return r;
}

// ============== MAIN ==============
int main(int argc, char** argv) {
    ensureDir(DATA_DIR);
    ensureDir(BOOK_DIR);

    int port = 8080;
    if (argc > 1) port = atoi(argv[1]);

    Server svr;

    svr.set_pre_routing_handler([](const Request& req, Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        if (req.method == "OPTIONS") {
            res.status = 200;
            return Server::HandlerResponse::Handled;
        }
        return Server::HandlerResponse::Unhandled;
    });

    // ---- /api/validate ----
    svr.Post("/api/validate", [](const Request& req, Response& res) {
        lock_guard<mutex> lk(g_mtx);
        json out;
        out["ok"] = false;
        try {
            auto body = json::parse(req.body);
            string role = body.value("role", "Passenger");
            string u = trim(body.value("username", ""));
            string p = body.value("password", "");

            string file;
            if (role == "Admin")       file = DATA_DIR + "admins.dat";
            else if (role == "Staff")  file = DATA_DIR + "staff_users.dat";
            else                        file = DATA_DIR + "passengers_users.dat";

            User arr[500];
            int n = loadUsers(file, arr, 500);
            Sort::insertionSort<User>(arr, n, userLess);
            User* found = findUser(arr, n, u);

            cout << "[validate] role=" << role
                 << " user='" << u << "'"
                 << " found=" << (found ? "yes" : "no");
            if (found) cout << " storedPass='" << found->password << "'";
            cout << endl;

            out["ok"] = (found && found->password == p);
            if (found) out["role"] = found->role;
        } catch (...) {}
        res.set_content(out.dump(), "application/json");
    });

    // ---- /api/register ----
    svr.Post("/api/register", [](const Request& req, Response& res) {
        lock_guard<mutex> lk(g_mtx);
        json out;
        out["ok"] = false;
        try {
            auto body = json::parse(req.body);
            string u = trim(body.value("username", ""));
            string p = body.value("password", "");
            if (u.empty() || p.empty()) {
                out["error"] = "empty";
                res.set_content(out.dump(), "application/json");
                return;
            }
            User arr[500];
            int n = loadUsers(DATA_DIR + "passengers_users.dat", arr, 500);
            Sort::insertionSort<User>(arr, n, userLess);
            if (findUser(arr, n, u)) {
                out["error"] = "user_exists";
                res.set_content(out.dump(), "application/json");
                return;
            }
            ofstream f((DATA_DIR + "passengers_users.dat").c_str(), ios::app);
            f << u << "|" << p << "\n";
            out["ok"] = true;
        } catch (...) {}
        res.set_content(out.dump(), "application/json");
    });

    // ---- /api/admin/create_staff ----
    svr.Post("/api/admin/create_staff", [](const Request& req, Response& res) {
        lock_guard<mutex> lk(g_mtx);
        json out;
        out["ok"] = false;
        try {
            auto body = json::parse(req.body);
            string admin = trim(body.value("admin", ""));
            string apwd  = body.value("adminPassword", "");
            string u     = trim(body.value("username", ""));
            string p     = body.value("password", "");
            string srole = body.value("staffRole", "checkin");

            User ad[50];
            int an = loadUsers(DATA_DIR + "admins.dat", ad, 50);
            Sort::insertionSort<User>(ad, an, userLess);
            User* af = findUser(ad, an, admin);
            if (!af || af->password != apwd) {
                out["error"] = "not_admin";
                res.set_content(out.dump(), "application/json");
                return;
            }

            User arr[500];
            int n = loadUsers(DATA_DIR + "staff_users.dat", arr, 500);
            Sort::insertionSort<User>(arr, n, userLess);
            if (findUser(arr, n, u)) {
                out["error"] = "user_exists";
                res.set_content(out.dump(), "application/json");
                return;
            }

            ofstream f((DATA_DIR + "staff_users.dat").c_str(), ios::app);
            f << u << "|" << p << "|" << srole << "\n";
            out["ok"] = true;
        } catch (...) {}
        res.set_content(out.dump(), "application/json");
    });

    // ---- /api/search_stops ----
    svr.Post("/api/search_stops", [](const Request&, Response& res) {
        lock_guard<mutex> lk(g_mtx);
        Flight fl[500];
        int fn = loadFlights(fl, 500);

        string stops[1000];
        int sn = 0;
        for (int i = 0; i < fn; i++) {
            stops[sn++] = fl[i].origin;
            stops[sn++] = fl[i].destination;
        }
        Sort::bubbleSort<string>(stops, sn,
            [](const string& a, const string& b) { return a < b; });

        int un = 0;
        for (int i = 0; i < sn; i++)
            if (i == 0 || stops[i] != stops[i - 1]) stops[un++] = stops[i];

        json arr = json::array();
        for (int i = 0; i < un; i++) arr.push_back(stops[i]);
        res.set_content(arr.dump(), "application/json");
    });

    // ---- /api/search ----
    svr.Post("/api/search", [](const Request& req, Response& res) {
        lock_guard<mutex> lk(g_mtx);
        json jArr = json::array();
        try {
            auto body = json::parse(req.body);
            string from = body.value("from", "");
            string to   = body.value("to", "");

            Flight fl[500];
            int fn = loadFlights(fl, 500);

            GraphResult r = shortestRoute(from, to, fl, fn);

            if (r.cost > 0) {
                for (int i = 0; i < r.len - 1; i++) {
                    for (int k = 0; k < fn; k++) {
                        if (fl[k].flightNumber == r.flights[i]) {
                            string dateStr = getDatePart(fl[k].departureTime);
                            int sold = countSoldSeats(fl[k].flightNumber, dateStr);
                            json o;
                            o["flightNumber"]  = fl[k].flightNumber;
                            o["origin"]        = r.path[i];
                            o["destination"]   = r.path[i + 1];
                            o["departureTime"] = fl[k].departureTime;
                            o["baseFare"]      = fl[k].baseFare;
                            o["totalSeats"]    = fl[k].totalSeats;
                            o["seatsSoldCount"] = sold;
                            jArr.push_back(o);
                            break;
                        }
                    }
                }
            }
        } catch (...) {}
        res.set_content(jArr.dump(), "application/json");
    });

    // ---- /api/book ----  (FIXED: dateStr derived from flight departure)
    svr.Post("/api/book", [](const Request& req, Response& res) {
        lock_guard<mutex> lk(g_mtx);
        json out;
        out["ok"] = false;
        try {
            auto body = json::parse(req.body);
            string username  = trim(body.value("username", ""));
            string flightNum = body.value("flightNumber", "");
            auto seats = body["seats"];
            auto pax   = body["passengers"];

            Flight fl[500];
            int fn = loadFlights(fl, 500);
            Flight* tf = nullptr;
            for (int i = 0; i < fn; i++) {
                if (fl[i].flightNumber == flightNum) { tf = &fl[i]; break; }
            }
            if (!tf) {
                out["error"] = "flight_not_found";
                res.set_content(out.dump(), "application/json");
                return;
            }

            // Derive date from the flight itself — NOT from the client
            string dateStr = getDatePart(tf->departureTime);

            time_t now = nowT();
            time_t dep = parseIso(tf->departureTime);
            time_t oneMonthBefore = dep - 30 * 24 * 3600;

            if (now < oneMonthBefore) {
                cout << "[book] rejected: booking_not_open_yet "
                     << "(now=" << fmtTime(now) << ", opens=" << fmtTime(oneMonthBefore) << ")" << endl;
                out["error"] = "booking_not_open_yet";
                res.set_content(out.dump(), "application/json");
                return;
            }
            if (now > dep) {
                out["error"] = "flight_departed";
                res.set_content(out.dump(), "application/json");
                return;
            }

            string path = bookingFilePath(flightNum, dateStr);
            BookingRec existing[500];
            int ex = loadBookings(path, existing, 500);

            int add = (int)seats.size();
            if (ex + add > tf->totalSeats) {
                out["error"] = "seats_full";
                res.set_content(out.dump(), "application/json");
                return;
            }

            for (int i = 0; i < add; i++) {
                string s = seats[i].get<string>();
                for (int j = 0; j < ex; j++) {
                    if (existing[j].seatNo == s) {
                        out["error"] = "seat_taken";
                        res.set_content(out.dump(), "application/json");
                        return;
                    }
                }
            }

            for (int i = 0; i < add; i++) {
                BookingRec b;
                b.username      = username;
                b.passengerName = pax[i].value("name", "");
                b.passport      = pax[i].value("passport", "");
                b.seatNo        = seats[i].get<string>();
                b.boardingCode  = makeBoardingCode(flightNum, b.seatNo);
                existing[ex++] = b;
            }
            saveBookings(path, existing, ex);
            cout << "[book] saved " << ex << " bookings to " << path << endl;
            out["ok"] = true;
            out["message"] = "Booked";
        } catch (...) {
            out["error"] = "bad_request";
        }
        res.set_content(out.dump(), "application/json");
    });

    // ---- /api/my_boarding_pass ----
    svr.Post("/api/my_boarding_pass", [](const Request& req, Response& res) {
        lock_guard<mutex> lk(g_mtx);
        json arr = json::array();
        try {
            auto body = json::parse(req.body);
            string username = trim(body.value("username", ""));
            Flight fl[500];
            int fn = loadFlights(fl, 500);
            for (int i = 0; i < fn; i++) {
                string dateStr = getDatePart(fl[i].departureTime);
                BookingRec br[500];
                int bn = loadBookings(bookingFilePath(fl[i].flightNumber, dateStr), br, 500);
                for (int j = 0; j < bn; j++) {
                    if (br[j].username == username) {
                        json o;
                        o["flightNumber"]  = fl[i].flightNumber;
                        o["origin"]        = fl[i].origin;
                        o["destination"]   = fl[i].destination;
                        o["departureTime"] = fl[i].departureTime;
                        o["seatNo"]        = br[j].seatNo;
                        o["passengerName"] = br[j].passengerName;
                        o["boardingCode"]  = br[j].boardingCode;
                        arr.push_back(o);
                    }
                }
            }
        } catch (...) {}
        res.set_content(arr.dump(), "application/json");
    });

    // ---- /api/checkin ----  (FIXED: dateStr derived from flight)
    svr.Post("/api/checkin", [](const Request& req, Response& res) {
    lock_guard<mutex> lk(g_mtx);
    json out;
    out["ok"] = false;
    try {
        auto body = json::parse(req.body);
        string boardingCode = trim(body.value("boardingCode", ""));
        string flightNum    = trim(body.value("flightNumber", ""));

        // If the client didn't send the flight number, extract it from the
        // boarding code. Format: FLIGHT-SEAT-UNIQUE  (e.g., AI101-1-15813)
        if (flightNum.empty()) {
            size_t dash = boardingCode.find('-');
            if (dash != string::npos) {
                flightNum = boardingCode.substr(0, dash);
            }
        }
        if (flightNum.empty() || boardingCode.empty()) {
            out["error"] = "invalid_boarding_code";
            res.set_content(out.dump(), "application/json");
            return;
        }

        Flight fl[500];
        int fn = loadFlights(fl, 500);
        Flight* tf = nullptr;
        for (int i = 0; i < fn; i++) {
            if (fl[i].flightNumber == flightNum) { tf = &fl[i]; break; }
        }
        if (!tf) {
            out["error"] = "flight_not_found";
            res.set_content(out.dump(), "application/json");
            return;
        }

        string dateStr = getDatePart(tf->departureTime);

        time_t now = nowT();
        time_t dep = parseIso(tf->departureTime);
        if (now > dep) {
            out["error"] = "flight_departed";
            res.set_content(out.dump(), "application/json");
            return;
        }
        if (dep - now > 3 * 3600) {
            out["error"] = "too_early";
            res.set_content(out.dump(), "application/json");
            return;
        }

        BookingRec br[500];
        int bn = loadBookings(bookingFilePath(flightNum, dateStr), br, 500);
        BookingRec* found = nullptr;
        for (int i = 0; i < bn; i++) {
            if (br[i].boardingCode == boardingCode) { found = &br[i]; break; }
        }
        if (!found) {
            out["error"] = "not_found";
            res.set_content(out.dump(), "application/json");
            return;
        }

        CheckedIn c;
        c.flightNum    = flightNum;
        c.dateStr      = dateStr;
        c.username     = found->username;
        c.boardingCode = boardingCode;
        c.seatNo       = found->seatNo;
        c.checkTime    = now;
        appendCheckedIn(c);

        out["ok"] = true;
        out["passengerName"] = found->passengerName;
    } catch (...) {}
    res.set_content(out.dump(), "application/json");
});
    // ---- /api/bag_push ----
    svr.Post("/api/bag_push", [](const Request& req, Response& res) {
        json out;
        out["ok"] = false;
        try {
            auto body = json::parse(req.body);
            string username    = body.value("username", "");
            string flightNum   = body.value("flightNumber", "");
            string dateStr     = body.value("date", "");
            string paxName     = body.value("passengerName", "");
            string bcode       = body.value("boardingCode", "");

            lock_guard<mutex> lk(g_beltMtx);
            BagRec b;
            char idbuf[32];
            snprintf(idbuf, sizeof(idbuf), "BAG%04d", g_nextBagId++);
            b.bagId         = idbuf;
            b.username      = username;
            b.flightNumber  = flightNum;
            b.passengerName = paxName;
            b.boardingCode  = bcode;
            b.currentSensor = 0;
            b.pushTime      = nowT();
            b.lost          = false;
            b.safeRecovered = false;
            g_belt.push_back(b);
            saveBags();

            out["ok"] = true;
            out["bagId"] = b.bagId;

            time_t times[PHOTON_COUNT];
            int tn;
            computeSensorTimes(b.pushTime, times, tn);
            json jt = json::array();
            for (int i = 0; i < tn; i++) jt.push_back((long long)times[i]);
            out["sensorHitTimes"] = jt;
        } catch (...) {}
        res.set_content(out.dump(), "application/json");
    });

    // ---- /api/bag_simulate_loss ----
    svr.Post("/api/bag_simulate_loss", [](const Request& req, Response& res) {
        json out;
        out["ok"] = false;
        try {
            auto body = json::parse(req.body);
            string bagId = body.value("bagId", "");
            int sensor   = body.value("sensor", 0);

            lock_guard<mutex> lk(g_beltMtx);
            DNode<BagRec>* n = g_belt.getHead();
            while (n) {
                if (n->data.bagId == bagId) {
                    n->data.lost = true;
                    n->data.currentSensor = sensor;
                    saveBags();
                    out["ok"] = true;
                    out["bagId"] = bagId;
                    out["sensor"] = sensor;
                    break;
                }
                n = n->next;
            }
        } catch (...) {}
        res.set_content(out.dump(), "application/json");
    });

    // ---- /api/bag_mark_safe ----
    svr.Post("/api/bag_mark_safe", [](const Request& req, Response& res) {
        json out;
        out["ok"] = false;
        try {
            auto body = json::parse(req.body);
            string bagId = body.value("bagId", "");

            lock_guard<mutex> lk(g_beltMtx);
            DNode<BagRec>* n = g_belt.getHead();
            while (n) {
                if (n->data.bagId == bagId) {
                    n->data.safeRecovered = true;
                    n->data.lost = false;
                    saveBags();
                    out["ok"] = true;
                    break;
                }
                n = n->next;
            }
        } catch (...) {}
        res.set_content(out.dump(), "application/json");
    });

    // ---- /api/bag_list ----
    svr.Post("/api/bag_list", [](const Request&, Response& res) {
        lock_guard<mutex> lk(g_beltMtx);
        json arr = json::array();
        DNode<BagRec>* n = g_belt.getHead();
        while (n) {
            const BagRec& b = n->data;
            json o;
            o["bagId"]         = b.bagId;
            o["username"]      = b.username;
            o["flightNumber"]  = b.flightNumber;
            o["passengerName"] = b.passengerName;
            o["boardingCode"]  = b.boardingCode;
            o["currentSensor"] = b.lost ? b.currentSensor : computeCurrentSensor(b.pushTime, nowT());
            o["pushTime"]      = (long long)b.pushTime;
            o["lost"]          = b.lost;
            o["safeRecovered"] = b.safeRecovered;
            arr.push_back(o);
            n = n->next;
        }
        res.set_content(arr.dump(), "application/json");
    });

    // ---- /api/my_bags ----
    svr.Post("/api/my_bags", [](const Request& req, Response& res) {
        lock_guard<mutex> lk(g_beltMtx);
        json arr = json::array();
        try {
            auto body = json::parse(req.body);
            string username = body.value("username", "");
            DNode<BagRec>* n = g_belt.getHead();
            while (n) {
                if (n->data.username == username) {
                    const BagRec& b = n->data;
                    json o;
                    o["bagId"]         = b.bagId;
                    o["flightNumber"]  = b.flightNumber;
                    o["currentSensor"] = b.lost ? b.currentSensor : computeCurrentSensor(b.pushTime, nowT());
                    o["pushTime"]      = (long long)b.pushTime;
                    o["lost"]          = b.lost;
                    o["safeRecovered"] = b.safeRecovered;
                    arr.push_back(o);
                }
                n = n->next;
            }
        } catch (...) {}
        res.set_content(arr.dump(), "application/json");
    });

    // ---- /api/set_time ----
    svr.Post("/api/set_time", [](const Request& req, Response& res) {
        lock_guard<mutex> lk(g_mtx);
        json out;
        out["ok"] = false;
        try {
            auto body = json::parse(req.body);
            string iso = body.value("iso", "");
            if (!iso.empty()) {
                g_simTime = parseIso(iso);
                g_simTimeSet = true;
                out["ok"] = true;
            }
            out["simTime"] = (long long)nowT();
        } catch (...) {}
        res.set_content(out.dump(), "application/json");
    });

    // ---- /api/checked_in_list ----
    svr.Post("/api/checked_in_list", [](const Request& req, Response& res) {
        lock_guard<mutex> lk(g_mtx);
        json arr = json::array();
        try {
            auto body = json::parse(req.body);
            string flightNum = body.value("flightNumber", "");
            CheckedIn ci[1000];
            int n = loadCheckedIn(ci, 1000);
            for (int i = 0; i < n; i++) {
                if (!flightNum.empty() && ci[i].flightNum != flightNum) continue;
                json o;
                o["flightNumber"]  = ci[i].flightNum;
                o["date"]          = ci[i].dateStr;
                o["username"]      = ci[i].username;
                o["boardingCode"]  = ci[i].boardingCode;
                o["seatNo"]        = ci[i].seatNo;
                o["checkTime"]     = (long long)ci[i].checkTime;
                arr.push_back(o);
            }
        } catch (...) {}
        res.set_content(arr.dump(), "application/json");
    });

    // ---- /api/flights_within_3h ----
    svr.Post("/api/flights_within_3h", [](const Request&, Response& res) {
        lock_guard<mutex> lk(g_mtx);
        json arr = json::array();
        Flight fl[500];
        int fn = loadFlights(fl, 500);
        time_t now = nowT();
        for (int i = 0; i < fn; i++) {
            time_t dep = parseIso(fl[i].departureTime);
            if (dep > now && dep - now <= 3 * 3600) {
                json o;
                o["flightNumber"]  = fl[i].flightNumber;
                o["origin"]        = fl[i].origin;
                o["destination"]   = fl[i].destination;
                o["departureTime"] = fl[i].departureTime;
                arr.push_back(o);
            }
        }
        res.set_content(arr.dump(), "application/json");
    });

    cout << "Server listening on 0.0.0.0:" << port << endl;
    svr.listen("0.0.0.0", port);
    return 0;
}