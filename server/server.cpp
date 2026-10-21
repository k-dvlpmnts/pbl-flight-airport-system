#ifdef _MSC_VER
  #define _CRT_SECURE_NO_WARNINGS
  #define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <direct.h>
  typedef SOCKET sock_t;
  #define CLOSESOCK closesocket
  #define MKDIR(p) _mkdir(p)
  typedef int sock_len_t;
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/time.h>
  #include <sys/stat.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <signal.h>
  typedef int sock_t;
  #define INVALID_SOCKET (-1)
  #define CLOSESOCK close
  #define MKDIR(p) mkdir(p, 0755)
  typedef socklen_t sock_len_t;
#endif

#include "data_structures.cpp"
#include "sorting_algorithms.cpp"

using namespace DataStructures;
using namespace Sorting;
using namespace std;
// ----------------------------------------------------------------------------
//  Configuration
// ----------------------------------------------------------------------------
#define BOOKING_WINDOW_DAYS   30      // bookings open one month before the flight
#define CHECKIN_CUTOFF_MIN    30     // check-in allowed strictly up to 30 min before departure
#define SIM_WINDOW_MIN        180     // simulator works on flights departing within 3 h
#define MIN_LAYOVER_MIN       60      // minimum connection time used by the route search
#define BOARDING_BEFORE_MIN   40
#define MAX_PAX_PER_BOOKING   9
#ifndef BELT_SPEED_MPS
#define BELT_SPEED_MPS        2.0     // conveyor speed (metres per second)
#endif
#ifndef BELT_TOLERANCE_SEC
#define BELT_TOLERANCE_SEC    1.0     // sensor gets this long to see a bag before "missed"
#endif
#ifndef BELT_MIN_GAP_SEC
#define BELT_MIN_GAP_SEC      3.0     // minimum spacing between two bags on the belt
#endif
#define MAX_SENSORS           8
#define BELT_REPORT_BAGS      60

#define F_FLIGHTS     "flights.dat"
#define F_AIRPORTS    "airports.dat"
#define F_PASSENGERS  "passengers_users.dat"
#define F_STAFF       "staff_users.dat"
#define F_ADMINS      "admin_users.dat"
#define F_TICKETS     "booked_tickets.dat"
#define DIR_BOOKINGS  "bookings"

//  Time helpers
static long long simOffset = 0;
static bool      simActive = false;

static long long floorDiv(long long a, long long b) {
    long long q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0))) q--;
    return q;
}
static long long floorMod(long long a, long long b) { return a - floorDiv(a, b) * b; }

static long long daysFromCivil(int y, int m, int d) {          // Howard Hinnant's algorithm
    y -= (m <= 2);
    long long era = floorDiv(y, 400);
    long long yoe = y - era * 400;
    long long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}
static void civilFromDays(long long z, int& y, int& m, int& d) {
    z += 719468;
    long long era = floorDiv(z, 146097);
    long long doe = z - era * 146097;
    long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long long yy = yoe + era * 400;
    long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    long long mp = (5 * doy + 2) / 153;
    d = (int)(doy - (153 * mp + 2) / 5 + 1);
    m = (int)(mp < 10 ? mp + 3 : mp - 9);
    y = (int)(yy + (m <= 2));
}
static int weekdayMon0(long long days) { return (int)floorMod(days + 3, 7); }   // 0 = Monday

static double realNowSec() {
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    unsigned long long t = ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    return (double)(t / 10000ULL) / 1000.0;
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (double)tv.tv_sec + tv.tv_usec / 1000000.0;
#endif
}
static long long realCivilNow() {
    time_t t = time(0);
    struct tm* lt = localtime(&t);
    return daysFromCivil(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday) * 86400LL
         + lt->tm_hour * 3600 + lt->tm_min * 60 + lt->tm_sec;
}
static long long nowCivil() { return realCivilNow() + simOffset; }

static String fmtDate(long long days) {
    int y, m, d;
    civilFromDays(days, y, m, d);
    char b[32];
    snprintf(b, sizeof(b), "%04d-%02d-%02d", y, m, d);
    return String(b);
}
static String fmtHM(int minutesOfDay) {
    char b[16];
    snprintf(b, sizeof(b), "%02d:%02d", (minutesOfDay / 60) % 24, minutesOfDay % 60);
    return String(b);
}
static String fmtTime(long long civil) {                         // HH:MM:SS
    int s = (int)floorMod(civil, 86400);
    char b[16];
    snprintf(b, sizeof(b), "%02d:%02d:%02d", s / 3600, (s / 60) % 60, s % 60);
    return String(b);
}
static String fmtDateTime(long long civil) {                     // YYYY-MM-DD HH:MM:SS
    return fmtDate(floorDiv(civil, 86400)) + " " + fmtTime(civil);
}
static String fmtDateHM(long long civil) {                       // YYYY-MM-DD HH:MM
    int s = (int)floorMod(civil, 86400);
    return fmtDate(floorDiv(civil, 86400)) + " " + fmtHM(s / 60);
}
static bool parseDate(const String& s, long long* days) {        // "YYYY-MM-DD"
    DynArray<String> p;
    String::split(s.trimmed(), '-', p);
    if (p.size() != 3) return false;
    bool a, b, c;
    long long y = p[0].toLong(&a), m = p[1].toLong(&b), d = p[2].toLong(&c);
    if (!a || !b || !c || y < 1970 || y > 2200 || m < 1 || m > 12 || d < 1 || d > 31) return false;
    long long dd = daysFromCivil((int)y, (int)m, (int)d);
    int y2, m2, d2;
    civilFromDays(dd, y2, m2, d2);
    if (y2 != y || m2 != m || d2 != d) return false;             // e.g. 2026-02-31
    *days = dd;
    return true;
}
static bool parseHM(const String& s, int* minutes) {               // "HH:MM"
    DynArray<String> p;
    String::split(s.trimmed(), ':', p);
    if (p.size() < 2) return false;
    bool a, b;
    long long h = p[0].toLong(&a), m = p[1].toLong(&b);
    if (!a || !b || h < 0 || h > 23 || m < 0 || m > 59) return false;
    *minutes = (int)(h * 60 + m);
    return true;
}
static bool parseDateTime(const String& s, long long* civil) {   // "YYYY-MM-DD HH:MM[:SS]"
    String t = s.trimmed().replaced('T', ' ');
    int sp = t.indexOf(' ');
    if (sp < 0) return false;
    long long days;
    int mins;
    if (!parseDate(t.substr(0, sp), &days)) return false;
    String rest = t.substr(sp + 1);
    if (!parseHM(rest, &mins)) return false;
    long long secs = 0;
    DynArray<String> p;
    String::split(rest, ':', p);
    if (p.size() >= 3) {
        bool ok;
        secs = p[2].toLong(&ok);
        if (!ok || secs < 0 || secs > 59) return false;
    }
    *civil = days * 86400LL + mins * 60LL + secs;
    return true;
}

// ----------------------------------------------------------------------------
//  Text file helpers (BOM / CRLF tolerant)
// ----------------------------------------------------------------------------
static bool fileExists(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}
static bool readTextFile(const char* path, String& out) {
    out.clear();
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    char chunk[4096];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0) out.append(chunk, (int)n);
    fclose(f);
    if (out.length() >= 3 && (unsigned char)out[0] == 0xEF && (unsigned char)out[1] == 0xBB
        && (unsigned char)out[2] == 0xBF)
        out = out.substr(3);                                    // strip UTF-8 BOM
    return true;
}
// reads every data line (trimmed, blank lines and '#' comments skipped)
static bool readDataLines(const char* path, DynArray<String>& lines) {
    lines.clear();
    String text;
    if (!readTextFile(path, text)) return false;
    DynArray<String> raw;
    String::split(text, '\n', raw);
    for (int i = 0; i < raw.size(); i++) {
        String l = raw[i].trimmed();
        if (l.empty() || l[0] == '#') continue;
        lines.push(l);
    }
    return true;
}
static bool writeTextFile(const char* path, const String& text) {
    FILE* f = fopen(path, "wb");
    if (!f) return false;
    fwrite(text.c_str(), 1, text.length(), f);
    fclose(f);
    return true;
}
// appends one line; repairs a missing final newline first (a classic cause of
// two records ending up glued together on one line)
static bool appendLine(const char* path, const String& line) {
    bool needNl = false;
    FILE* r = fopen(path, "rb");
    if (r) {
        if (fseek(r, -1, SEEK_END) == 0) {
            int c = fgetc(r);
            if (c != '\n' && c != EOF) needNl = true;
        }
        fclose(r);
    }
    FILE* f = fopen(path, "ab");
    if (!f) return false;
    if (needNl) fputc('\n', f);
    fwrite(line.c_str(), 1, line.length(), f);
    fputc('\n', f);
    fclose(f);
    return true;
}

//  Response builder
class Row {
public:
    String s;
    Row(const char* tag) : s(tag) {}
    Row& f(const String& v) { s += '|'; s += v; return *this; }
    Row& f(const char* v) { s += '|'; s += v; return *this; }
    Row& n(long long v) { s += '|'; s.appendInt(v); return *this; }
    Row& d(double v, int dec) { s += '|'; s.appendDouble(v, dec); return *this; }
    void emit(String& out) { out += s; out += '\n'; }
};
static void fail(String& out, const char* code, const char* message) {
    out.clear();
    Row("ERR").f(code).f(message).emit(out);
}
static String clean(const String& in, int maxLen) {              
    String r;
    for (int i = 0; i < in.length() && i < maxLen; i++) {
        char c = in[i];
        if (c == '|' || c == '\r' || c == '\n' || c == '^') c = ' ';
        r += c;
    }
    return r.trimmed();
}

static int strCmp(const String& a, const String& b) { return a.compare(b); }

//  Accounts 
class Account {
public:
    String username;
    String password;
    virtual ~Account() {}
    virtual const char* role() const = 0;
    virtual String toLine() const { return username + "|" + password; }
};
class PassengerAccount : public Account {
public:
    const char* role() const { return "PASSENGER"; }
};
class StaffAccount : public Account {
public:
    String staffType;                                   // CHECKIN or BELT
    const char* role() const { return "STAFF"; }
    String toLine() const { return username + "|" + password + "|" + staffType; }
};
class AdminAccount : public Account {
public:
    const char* role() const { return "ADMIN"; }
};

static int accountCmp(Account* const& a, Account* const& b) { return a->username.compare(b->username); }
static int accountKeyCmp(Account* const& a, const String& key) { return a->username.compare(key); }

enum AccountKind { KIND_PASSENGER = 0, KIND_STAFF = 1, KIND_ADMIN = 2 };

class AccountStore {
private:
    String path;
    int kind;
    DynArray<Account*> list;                            // kept sorted by username
    AccountStore(const AccountStore&);
    AccountStore& operator=(const AccountStore&);
    void freeAll() {
        for (int i = 0; i < list.size(); i++) delete list[i];
        list.clear();
    }
    Account* make() const {
        if (kind == KIND_STAFF) return new StaffAccount();
        if (kind == KIND_ADMIN) return new AdminAccount();
        return new PassengerAccount();
    }
public:
    AccountStore(const char* p, int k) : path(p), kind(k) {}
    ~AccountStore() { freeAll(); }

    void load() {
        freeAll();
        DynArray<String> lines;
        readDataLines(path.c_str(), lines);
        for (int i = 0; i < lines.size(); i++) {
            DynArray<String> f;
            String::split(lines[i], '|', f);
            if (f.size() < 2 || f[0].trimmed().empty()) continue;
            Account* a = make();
            a->username = f[0].trimmed();
            a->password = f[1].trimmed();
            if (kind == KIND_STAFF) {
                String t = (f.size() > 2) ? f[2].trimmed().upper() : String("CHECKIN");
                if (!(t == "CHECKIN" || t == "BELT")) t = "CHECKIN";
                static_cast<StaffAccount*>(a)->staffType = t;
            }
            list.push(a);
        }
        mergeSort(list, accountCmp);
    }
    int count() const { return list.size(); }
    Account* at(int i) const { return list[i]; }
    int find(const String& user) const { return binarySearch(list, user, accountKeyCmp); }   // binary search

    bool check(const String& user, const String& pass) const {
        int i = find(user);
        return i >= 0 && list[i]->password == pass;
    }
    // appends to the file (and to the sorted list)
    bool add(const String& user, const String& pass, const String& type) {
        Account* a = make();
        a->username = user;
        a->password = pass;
        if (kind == KIND_STAFF) static_cast<StaffAccount*>(a)->staffType = type;
        bool ok = appendLine(path.c_str(), a->toLine());
        delete a;
        return ok;
    }
};

static bool validUsername(const String& u) {
    if (u.length() < 2 || u.length() > 24) return false;
    for (int i = 0; i < u.length(); i++) {
        char c = u[i];
        bool good = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
                    || c == '_' || c == '.' || c == '-';
        if (!good) return false;
    }
    return true;
}

class Flight {
public:
    String number, origin, dest;
    int depMin, durMin, seats, days;                    // days = 7 bit mask, bit0 = Monday
    double fare;
    Flight() : depMin(0), durMin(0), seats(0), days(127), fare(0) {}
    bool operatesOn(long long dayNum) const { return ((days >> weekdayMon0(dayNum)) & 1) != 0; }
    long long departure(long long dayNum) const { return dayNum * 86400LL + depMin * 60LL; }
};
static int flightCmp(const Flight& a, const Flight& b) { return a.number.compare(b.number); }
static int flightKeyCmp(const Flight& a, const String& k) { return a.number.compare(k); }
static int flightByDepCmp(const Flight& a, const Flight& b) {
    if (a.depMin != b.depMin) return a.depMin < b.depMin ? -1 : 1;
    return a.number.compare(b.number);
}

static void loadFlights(DynArray<Flight>& out) {
    out.clear();
    DynArray<String> lines;
    readDataLines(F_FLIGHTS, lines);
    for (int i = 0; i < lines.size(); i++) {
        DynArray<String> f;
        String::split(lines[i], '|', f);
        if (f.size() < 7) continue;
        Flight fl;
        fl.number = f[0].trimmed();
        fl.origin = f[1].trimmed().upper();
        fl.dest = f[2].trimmed().upper();
        bool ok1 = true, ok2 = true, ok3 = true, okT = true;
        if (f[3].indexOf('T') >= 0) {                              // legacy "2026-11-10T08:00:00" format
            long long civil;
            okT = parseDateTime(f[3], &civil);
            fl.depMin = (int)(floorMod(civil, 86400) / 60);
            fl.durMin = 120;
            fl.fare = f[4].toDouble(&ok1);
            fl.seats = (int)f[5].toLong(&ok2);
            fl.days = 127;
        } else {
            okT = parseHM(f[3], &fl.depMin);
            fl.durMin = (int)f[4].toLong(&ok1);
            fl.fare = f[5].toDouble(&ok2);
            fl.seats = (int)f[6].toLong(&ok3);
            fl.days = 127;
            if (f.size() > 7 && f[7].trimmed().length() == 7) {
                fl.days = 0;
                String m = f[7].trimmed();
                for (int b = 0; b < 7; b++) if (m[b] == '1') fl.days |= (1 << b);
            }
        }
        if (!ok1 || !ok2 || !ok3 || !okT || fl.number.empty() || fl.seats <= 0 || fl.durMin <= 0) continue;
        out.push(fl);
    }
    mergeSort(out, flightCmp);
}
static int findFlight(const DynArray<Flight>& fl, const String& number) {
    return binarySearch(fl, number, flightKeyCmp);
}

class Airport {
public:
    String code, name;
};
static int airportCmp(const Airport& a, const Airport& b) { return a.code.compare(b.code); }
static int airportKeyCmp(const Airport& a, const String& k) { return a.code.compare(k); }
static void loadAirports(DynArray<Airport>& out) {
    out.clear();
    DynArray<String> lines;
    readDataLines(F_AIRPORTS, lines);
    for (int i = 0; i < lines.size(); i++) {
        DynArray<String> f;
        String::split(lines[i], '|', f);
        if (f.size() < 2) continue;
        Airport a;
        a.code = f[0].trimmed().upper();
        a.name = f[1].trimmed();
        out.push(a);
    }
    quickSort(out, airportCmp);
}
// every airport that appears in the timetable, sorted and unique
static void collectStops(const DynArray<Flight>& fl, DynArray<String>& codes) {
    DynArray<String> all;
    for (int i = 0; i < fl.size(); i++) { all.push(fl[i].origin); all.push(fl[i].dest); }
    quickSort(all, strCmp);
    codes.clear();
    for (int i = 0; i < all.size(); i++)
        if (i == 0 || all[i] != all[i - 1]) codes.push(all[i]);
}
static int stopIndex(const DynArray<String>& codes, const String& code) {
    return binarySearch(codes, code, strCmp);
}

static void parseSeat(const String& s, int& row, char& col) {
    row = 0;
    col = 0;
    int i = 0;
    while (i < s.length() && s[i] >= '0' && s[i] <= '9') { row = row * 10 + (s[i] - '0'); i++; }
    if (i < s.length()) col = s[i];
    if (col >= 'a' && col <= 'z') col -= 32;
}
static int seatLabelCmp(const String& a, const String& b) {
    int ra, rb;
    char ca, cb;
    parseSeat(a, ra, ca);
    parseSeat(b, rb, cb);
    if (ra != rb) return ra < rb ? -1 : 1;
    if (ca != cb) return ca < cb ? -1 : 1;
    return 0;
}
static bool validSeat(const String& s, int totalSeats) {
    int row;
    char col;
    parseSeat(s, row, col);
    if (row < 1 || col < 'A' || col > 'F') return false;
    if (s.length() != (row >= 10 ? 3 : 2)) return false;
    return (row - 1) * 6 + (col - 'A') < totalSeats;
}
static String normalizeSeat(const String& s) {
    int row;
    char col;
    parseSeat(s.trimmed(), row, col);
    String r;
    r.appendInt(row);
    r.appendChar(col);
    return r;
}

//  Booking files
class PaxRecord {
public:
    String pnr, seat, name, gender, username, code, bagTag, checkInTime, bagStatus, bookedAt;
    int age, bagKg;
    bool checkedIn;
    PaxRecord() : age(0), bagKg(0), checkedIn(false) {}

    String toLine() const {
        Row r(pnr.c_str());
        r.f(seat).f(name).n(age).f(gender).f(username).f(code).f(bagTag).n(bagKg)
         .n(checkedIn ? 1 : 0).f(checkInTime).f(bagStatus).f(bookedAt);
        return r.s;
    }
    static bool fromLine(const String& line, PaxRecord& p) {
        DynArray<String> f;
        String::split(line, '|', f);
        if (f.size() < 13) return false;
        p.pnr = f[0].trimmed();
        p.seat = f[1].trimmed();
        p.name = f[2];
        p.age = (int)f[3].toLong();
        p.gender = f[4];
        p.username = f[5];
        p.code = f[6];
        p.bagTag = f[7];
        p.bagKg = (int)f[8].toLong();
        p.checkedIn = f[9].trimmed() == "1";
        p.checkInTime = f[10];
        p.bagStatus = f[11].trimmed();
        p.bookedAt = f[12];
        return true;
    }
};
static int paxSeatCmp(const PaxRecord& a, const PaxRecord& b) { return seatLabelCmp(a.seat, b.seat); }
static int paxSeatKeyCmp(const PaxRecord& a, const String& seat) { return seatLabelCmp(a.seat, seat); }
static int paxTagCmp(const PaxRecord& a, const PaxRecord& b) { return a.bagTag.compare(b.bagTag); }
static int paxTagKeyCmp(const PaxRecord& a, const String& tag) { return a.bagTag.compare(tag); }

class PaxKey {
public:
    String pnr, seat;
};
static int paxPnrSeatCmp(const PaxRecord& a, const PaxRecord& b) {
    int c = a.pnr.compare(b.pnr);
    return c ? c : seatLabelCmp(a.seat, b.seat);
}
static int paxPnrSeatKeyCmp(const PaxRecord& a, const PaxKey& k) {
    int c = a.pnr.compare(k.pnr);
    return c ? c : seatLabelCmp(a.seat, k.seat);
}

static String bookingPath(const String& flight, const String& date) {
    return String(DIR_BOOKINGS) + "/" + flight + "_" + date + ".dat";
}
static const char* BOOKING_HEADER =
    "# pnr|seat|name|age|gender|username|checkInCode|bagTag|bagKg|checkedIn|checkInTime|bagStatus|bookedAt\n";

// loads (sorted by seat). returns false when no bookings exist yet for that flight/day
static bool loadBooking(const String& flight, const String& date, DynArray<PaxRecord>& out) {
    out.clear();
    DynArray<String> lines;
    if (!readDataLines(bookingPath(flight, date).c_str(), lines)) return false;
    for (int i = 0; i < lines.size(); i++) {
        PaxRecord p;
        if (PaxRecord::fromLine(lines[i], p)) out.push(p);
    }
    mergeSort(out, paxSeatCmp);
    return true;
}
// NOTE: sorts a private copy, so references the caller holds into 'arr' stay valid
static bool saveBooking(const String& flight, const String& date, const DynArray<PaxRecord>& arr) {
    DynArray<PaxRecord> sorted = arr;
    mergeSort(sorted, paxSeatCmp);
    String text;
    text += "# flight ";
    text += flight;
    text += " on ";
    text += date;
    text += "\n";
    text += BOOKING_HEADER;
    for (int i = 0; i < sorted.size(); i++) { text += sorted[i].toLine(); text += '\n'; }
    return writeTextFile(bookingPath(flight, date).c_str(), text);
}
// first booking of the day creates the file, later bookings edit (extend) the existing one
static bool appendBooking(const String& flight, const String& date, const PaxRecord& p) {
    String path = bookingPath(flight, date);
    if (!fileExists(path.c_str())) {
        String head = String("# flight ") + flight + " on " + date + "\n" + BOOKING_HEADER;
        if (!writeTextFile(path.c_str(), head)) return false;
    }
    return appendLine(path.c_str(), p.toLine());
}


class TicketRec {
public:
    String username, pnr, flight, date, bookedAt;
    int seats;
    double total;
    TicketRec() : seats(0), total(0) {}
    String toLine() const {
        Row r(username.c_str());
        r.f(pnr).f(flight).f(date).n(seats).d(total, 2).f(bookedAt);
        return r.s;
    }
};
static const char* TICKET_HEADER = "# username|pnr|flightNumber|date|seatCount|totalFare|bookingTimestamp\n";

static void loadTickets(DynArray<TicketRec>& out) {
    out.clear();
    DynArray<String> lines;
    readDataLines(F_TICKETS, lines);
    for (int i = 0; i < lines.size(); i++) {
        DynArray<String> f;
        String::split(lines[i], '|', f);
        if (f.size() < 7) continue;                       
        TicketRec t;
        t.username = f[0].trimmed();
        t.pnr = f[1].trimmed();
        t.flight = f[2].trimmed();
        t.date = f[3].trimmed();
        t.seats = (int)f[4].toLong();
        t.total = f[5].toDouble();
        t.bookedAt = f[6].trimmed();
        out.push(t);
    }
}
static int ticketUserCmp(const TicketRec& a, const TicketRec& b) {
    int c = a.username.compare(b.username);
    if (c) return c;
    c = a.date.compare(b.date);
    if (c) return c;
    c = a.bookedAt.compare(b.bookedAt);
    return c ? c : a.pnr.compare(b.pnr);
}
static int ticketUserKeyCmp(const TicketRec& t, const String& user) { return t.username.compare(user); }
static int ticketPnrCmp(const TicketRec& a, const TicketRec& b) { return a.pnr.compare(b.pnr); }
static int ticketPnrKeyCmp(const TicketRec& t, const String& pnr) { return t.pnr.compare(pnr); }

//  Random identifiers
static String randomPnr() {
    static const char* alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    String s;
    for (int i = 0; i < 6; i++) s += alphabet[rand() % 32];
    return s;
}
static String randomDigits(int n) {
    String s;
    for (int i = 0; i < n; i++) s += (char)('0' + rand() % 10);
    return s;
}
static String gateFor(const String& flight, long long dayNum) {     // stable, made up "gate"
    int h = 0;
    for (int i = 0; i < flight.length(); i++) h = h * 31 + flight[i];
    h += (int)floorMod(dayNum, 5);
    if (h < 0) h = -h;
    String g;
    g += (char)('A' + h % 4);
    g.appendInt(1 + (h / 4) % 12);
    return g;
}

// creates files that are missing so that a fresh folder still works
static void ensureDefaults() {
    MKDIR(DIR_BOOKINGS);
    if (!fileExists(F_ADMINS)) {
        writeTextFile(F_ADMINS, String("# username|password   (administrators who may create staff accounts)\nadmin|admin123\n"));
        printf("[setup] created %s with default administrator  admin / admin123\n", F_ADMINS);
    }
    if (!fileExists(F_STAFF))
        writeTextFile(F_STAFF, String("# username|password|staffType (CHECKIN or BELT)\n"));
    if (!fileExists(F_PASSENGERS))
        writeTextFile(F_PASSENGERS, String("# username|password\n"));
    if (!fileExists(F_TICKETS))
        writeTextFile(F_TICKETS, String(TICKET_HEADER));
    if (!fileExists(F_FLIGHTS)) {
        writeTextFile(F_FLIGHTS, String(
            "# flightNumber|origin|destination|departure HH:MM|duration min|baseFare|totalSeats|days Mon..Sun\n"
            "AI101|DEL|BOM|08:00|130|4500|30|1111111\n"
            "AI202|BOM|BLR|12:30|105|3200|30|1111111\n"
            "AI303|DEL|BLR|14:00|175|7800|30|1111111\n"));
        printf("[setup] created sample %s\n", F_FLIGHTS);
    }
}


//  Passenger lookup
static int locatePax(DynArray<PaxRecord>& arr, const String& pnr, const String& seat) {
    quickSort(arr, paxPnrSeatCmp);                      // binary search needs this order
    PaxKey k;
    k.pnr = pnr;
    k.seat = seat;
    return binarySearch(arr, k, paxPnrSeatKeyCmp);
}
static bool updateBagStatus(const String& flight, const String& date, const String& pnr,
                            const String& seat, const char* status) {
    DynArray<PaxRecord> arr;
    if (!loadBooking(flight, date, arr)) return false;
    int i = locatePax(arr, pnr, seat);
    if (i < 0) return false;
    arr[i].bagStatus = status;
    return saveBooking(flight, date, arr);
}

// ----------------------------------------------------------------------------
//  Luggage belt
//
//   [Start RFID] --- photon 1 --- photon 2 --- photon 3 --- photon 4 --- [End RFID]
//
//  * When a bag crosses the Start RFID it is appended to a doubly linked list
//    (one node per bag, in belt order) and the time at which every other sensor
//    must see it is calculated:   t(sensor) = t(push) + distance / belt speed.
//  * The photon sensors only know that "something passed"; the End RFID reads
//    the bag tag.  If a sensor does not fire at its timestamp the bag is
//    identified through the list: it is the node that follows the last bag the
//    sensor saw.  For the end sensor this is "the bag after the last one that
//    crossed the end sensor".
// ---------------------------------------------------------------------------
class Sensor {
public:
    String name;
    double distance;                                    // metres from the start of the belt
    Sensor(const char* n, double d) : name(n), distance(d) {}
    virtual ~Sensor() {}
    virtual const char* kind() const = 0;
    virtual bool readsTag() const = 0;
};
class RfidSensor : public Sensor {
public:
    RfidSensor(const char* n, double d) : Sensor(n, d) {}
    const char* kind() const { return "RFID"; }
    bool readsTag() const { return true; }
};
class PhotonSensor : public Sensor {
public:
    PhotonSensor(const char* n, double d) : Sensor(n, d) {}
    const char* kind() const { return "PHOTON"; }
    bool readsTag() const { return false; }
};

enum BagState { BAG_ONBELT = 0, BAG_DELIVERED = 1, BAG_LOST = 2, BAG_RECOVERED = 3 };
static const char* bagStateName(int s) {
    switch (s) {
        case BAG_ONBELT: return "ONBELT";
        case BAG_DELIVERED: return "DELIVERED";
        case BAG_LOST: return "LOST";
        default: return "RECOVERED";
    }
}

class BagInfo {
public:
    String tag, flight, date, pnr, seat, name, username;
    String lostTime, deliveredTime, recoveredInfo;
    int weightKg;
    int state;
    int passed;                                         // highest sensor index that saw the bag
    int lostAt;                                         // sensor that failed to see it (-1 none)
    int dropAt;                                         // SIMULATION ONLY: sensor that will miss it
    double pushReal;                                    // real clock at the start RFID
    long long pushCivil;                                // system (simulated) clock at the start RFID
    BagInfo() : weightKg(0), state(BAG_ONBELT), passed(0), lostAt(-1), dropAt(-1), pushReal(0), pushCivil(0) {}
};

class Belt {
private:
    DynArray<Sensor*> sensors;
    DoublyLinkedList<BagInfo> line;
    DNode<BagInfo>* sensorLast[MAX_SENSORS];            // last bag each sensor has dealt with
    double lastPushReal;
    Belt(const Belt&);
    Belt& operator=(const Belt&);

    // bag the DLL says sensor j should be seeing now
    DNode<BagInfo>* expectedBagAt(int j) {
        DNode<BagInfo>* n = sensorLast[j] ? sensorLast[j]->next : line.head();
        while (n && n->data.lostAt != -1 && n->data.lostAt <= j) n = n->next;   // skip bags that fell earlier
        return n;
    }
public:
    Belt() : lastPushReal(-1e12) {
        sensors.push(new RfidSensor("Start RFID", 0));
        sensors.push(new PhotonSensor("Photon 1", 5));
        sensors.push(new PhotonSensor("Photon 2", 10));
        sensors.push(new PhotonSensor("Photon 3", 15));
        sensors.push(new PhotonSensor("Photon 4", 20));
        sensors.push(new RfidSensor("End RFID", 25));
        for (int i = 0; i < MAX_SENSORS; i++) sensorLast[i] = 0;
    }
    ~Belt() { for (int i = 0; i < sensors.size(); i++) delete sensors[i]; }

    int sensorCount() const { return sensors.size(); }
    const Sensor* sensor(int i) const { return sensors[i]; }
    double length() const { return sensors[sensors.size() - 1]->distance; }
    double offsetOf(int j) const { return sensors[j]->distance / BELT_SPEED_MPS; }   // seconds after the push
    DoublyLinkedList<BagInfo>& bags() { return line; }

    DNode<BagInfo>* findBag(const String& flight, const String& date, const String& pnr, const String& seat) {
        for (DNode<BagInfo>* n = line.tail(); n; n = n->prev)
            if (n->data.pnr == pnr && n->data.seat == seat && n->data.flight == flight && n->data.date == date)
                return n;
        return 0;
    }
    DNode<BagInfo>* findTag(const String& tag) {
        for (DNode<BagInfo>* n = line.tail(); n; n = n->prev)
            if (n->data.tag == tag) return n;
        return 0;
    }

    // bag crosses the start RFID.  false => start of the belt still occupied
    bool pushBag(const BagInfo& proto, double now, double* waitSec) {
        if (now - lastPushReal < BELT_MIN_GAP_SEC) {
            *waitSec = BELT_MIN_GAP_SEC - (now - lastPushReal);
            return false;
        }
        BagInfo b = proto;
        b.state = BAG_ONBELT;
        b.passed = 0;
        b.lostAt = -1;
        b.pushReal = now;
        b.pushCivil = nowCivil();
        sensorLast[0] = line.pushBack(b);
        lastPushReal = now;
        return true;
    }

    // run the timetable: every sensor either sees its bag on time or misses it
    void tick(double now) {
        int last = sensors.size() - 1;
        for (DNode<BagInfo>* n = line.head(); n; n = n->next) {
            BagInfo& b = n->data;
            if (b.state != BAG_ONBELT) continue;
            for (int j = b.passed + 1; j <= last; j++) {
                double due = b.pushReal + offsetOf(j);
                if (b.dropAt == j) {                                   // this sensor will NOT fire
                    if (now < due + BELT_TOLERANCE_SEC) break;         // still within tolerance
                    DNode<BagInfo>* victim = expectedBagAt(j);         // identify through the list
                    if (!victim) victim = n;
                    BagInfo& v = victim->data;
                    v.state = BAG_LOST;
                    v.lostAt = j;
                    v.lostTime = fmtDateTime(v.pushCivil + (long long)offsetOf(j));
                    sensorLast[j] = victim;
                    updateBagStatus(v.flight, v.date, v.pnr, v.seat, "LOST");
                    printf("[belt] !! sensor %d (%s) missed bag %s (%s) - loss reported\n",
                           j, sensors[j]->name.c_str(), v.tag.c_str(), v.name.c_str());
                    break;
                }
                if (now < due) break;                                  // not yet time
                b.passed = j;                                          // sensor fired on time
                sensorLast[j] = n;
                if (j == last) {
                    b.state = BAG_DELIVERED;
                    b.deliveredTime = fmtDateTime(b.pushCivil + (long long)offsetOf(j));
                    updateBagStatus(b.flight, b.date, b.pnr, b.seat, "DELIVERED");
                }
            }
        }
    }

    // metres from the start; -1 = not on the belt any more (fell off, not yet noticed)
    double positionOf(const BagInfo& b, double now) const {
        double L = length();
        if (b.state == BAG_DELIVERED) return L;
        if (b.state == BAG_LOST || b.state == BAG_RECOVERED) {
            int j = b.lostAt;
            return (sensors[j - 1]->distance + sensors[j]->distance) / 2.0;
        }
        double pos = (now - b.pushReal) * BELT_SPEED_MPS;
        if (pos < 0) pos = 0;
        if (pos > L) pos = L;
        if (b.dropAt >= 1) {
            double fall = (sensors[b.dropAt - 1]->distance + sensors[b.dropAt]->distance) / 2.0;
            if (pos >= fall) return -1;
        }
        return pos;
    }
};

static Belt* belt = 0;                                  

//  Route search : Dijkstra over the airport graph using the real timetable
class ScheduleModel : public EdgeModel {
public:
    const DynArray<Flight>* flights;
    long long startDay;                                  // the date chosen on the calendar
    long long now;                                       // system clock
    ScheduleModel(const DynArray<Flight>* f, long long day, long long n) : flights(f), startDay(day), now(n) {}

    // departure (absolute civil seconds) of flight fi when we stand at its origin at time tFrom; -1 = none
    long long departureFor(int fi, long long tFrom, bool isStart) const {
        const Flight& f = (*flights)[fi];
        if (isStart) {
            if (!f.operatesOn(startDay)) return -1;      // first leg must leave on the chosen date
            long long dep = f.departure(startDay);
            return dep >= now ? dep : -1;                // cannot board a plane that has left
        }
        long long earliest = tFrom + MIN_LAYOVER_MIN * 60LL;
        long long d0 = floorDiv(earliest, 86400);
        for (int k = 0; k < 3; k++) {                    // today, tomorrow, day after
            long long day = d0 + k;
            if (!f.operatesOn(day)) continue;
            long long dep = f.departure(day);
            if (dep >= earliest) return dep;
        }
        return -1;
    }
    bool traverse(int edgeId, int, long long timeAtFrom, bool isStart, long long* arrivalAtTo) {
        long long dep = departureFor(edgeId, timeAtFrom, isStart);
        if (dep < 0) return false;
        *arrivalAtTo = dep + (*flights)[edgeId].durMin * 60LL;
        return true;
    }
};

// can the flight on this day be booked right now?
static bool bookingOpen(const Flight& f, long long dayNum, long long now, String& code, String& reason) {
    if (!f.operatesOn(dayNum)) { code = "not_operating"; reason = "Flight does not operate on this date"; return false; }
    long long dep = f.departure(dayNum);
    if (now >= dep) { code = "departed"; reason = "Flight has already departed"; return false; }
    long long opens = dep - (long long)BOOKING_WINDOW_DAYS * 86400LL;
    if (now < opens) {
        code = "not_open";
        reason = String("Booking opens on ") + fmtDateHM(opens) + " (one month before the flight)";
        return false;
    }
    code = "";
    reason = "";
    return true;
}
static int soldSeats(const String& flight, const String& date) {
    DynArray<PaxRecord> arr;
    loadBooking(flight, date, arr);
    return arr.size();
}

static void emitLeg(String& out, const char* kind, int index, const Flight& f, long long dep, long long arr, long long now) {
    long long day = floorDiv(dep, 86400);
    String date = fmtDate(day);
    String code, reason;
    bool ok = bookingOpen(f, day, now, code, reason);
    int avail = f.seats - soldSeats(f.number, date);
    Row("LEG").f(kind).n(index).f(f.number).f(f.origin).f(f.dest).f(date)
        .f(fmtHM((int)(floorMod(dep, 86400) / 60))).f(fmtDateHM(arr)).n(f.durMin).d(f.fare, 2)
        .n(f.seats).n(avail).n(ok ? 1 : 0).f(reason).emit(out);
}

//  Command handlers
static bool needArgs(DynArray<String>& a, int n, String& out) {
    if (a.size() < n) { fail(out, "bad_request", "Missing request fields"); return false; }
    return true;
}
static bool staffIsType(const String& user, const char* type) {
    AccountStore st(F_STAFF, KIND_STAFF);
    st.load();
    int i = st.find(user);
    return i >= 0 && static_cast<StaffAccount*>(st.at(i))->staffType == type;
}
static bool adminOk(const String& user, const String& pass) {
    AccountStore st(F_ADMINS, KIND_ADMIN);
    st.load();
    return st.check(user, pass);
}

static void cmdTime(DynArray<String>&, String& out) {
    long long t = nowCivil();
    Row("OK").n(t).f(fmtDateTime(t)).n(simActive ? 1 : 0).emit(out);
}

static void cmdLogin(DynArray<String>& a, String& out) {
    if (!needArgs(a, 4, out)) return;
    String role = a[1].trimmed().upper(), user = a[2].trimmed(), pass = a[3];
    const char* file = F_PASSENGERS;
    int kind = KIND_PASSENGER;
    if (role == "STAFF") { file = F_STAFF; kind = KIND_STAFF; }
    else if (role == "ADMIN") { file = F_ADMINS; kind = KIND_ADMIN; }
    else if (!(role == "PASSENGER")) { fail(out, "bad_request", "Unknown role"); return; }
    AccountStore st(file, kind);
    st.load();
    if (!st.check(user, pass)) { fail(out, "bad_credentials", "Invalid username or password."); return; }
    Row r("OK");
    r.f(role);
    if (kind == KIND_STAFF) r.f(static_cast<StaffAccount*>(st.at(st.find(user)))->staffType);
    r.emit(out);
}

static void cmdRegister(DynArray<String>& a, String& out) {
    if (!needArgs(a, 3, out)) return;
    String user = a[1].trimmed(), pass = a[2];
    if (user.empty() || pass.empty()) { fail(out, "empty_fields", "Username and password are required."); return; }
    if (!validUsername(user)) { fail(out, "bad_username", "Username must be 2-24 letters, digits, '_', '.' or '-'."); return; }
    if (pass.length() < 3 || pass.length() > 40) { fail(out, "bad_password", "Password must be 3-40 characters."); return; }
    AccountStore st(F_PASSENGERS, KIND_PASSENGER);
    st.load();
    if (st.find(user) >= 0) { fail(out, "user_exists", "That username is already taken."); return; }
    if (!st.add(user, pass, "")) { fail(out, "file_error", "Could not write the passenger file."); return; }
    Row("OK").emit(out);
}

static void cmdAdminListStaff(DynArray<String>& a, String& out) {
    if (!needArgs(a, 3, out)) return;
    if (!adminOk(a[1].trimmed(), a[2])) { fail(out, "bad_credentials", "Administrator login failed."); return; }
    AccountStore st(F_STAFF, KIND_STAFF);
    st.load();
    Row("OK").n(st.count()).emit(out);
    for (int i = 0; i < st.count(); i++)
        Row("S").f(st.at(i)->username).f(static_cast<StaffAccount*>(st.at(i))->staffType).emit(out);
}

static void cmdAdminAddStaff(DynArray<String>& a, String& out) {
    if (!needArgs(a, 6, out)) return;
    if (!adminOk(a[1].trimmed(), a[2])) { fail(out, "bad_credentials", "Administrator login failed."); return; }
    String user = a[3].trimmed(), pass = a[4], type = a[5].trimmed().upper();
    if (!(type == "CHECKIN" || type == "BELT")) { fail(out, "bad_type", "Staff type must be CHECKIN or BELT."); return; }
    if (!validUsername(user)) { fail(out, "bad_username", "Username must be 2-24 letters, digits, '_', '.' or '-'."); return; }
    if (pass.length() < 3 || pass.length() > 40) { fail(out, "bad_password", "Password must be 3-40 characters."); return; }
    AccountStore st(F_STAFF, KIND_STAFF);
    st.load();
    if (st.find(user) >= 0) { fail(out, "user_exists", "A staff member with that username already exists."); return; }
    if (!st.add(user, pass, type)) { fail(out, "file_error", "Could not write the staff file."); return; }
    printf("[admin] %s created %s account '%s'\n", a[1].c_str(), type.c_str(), user.c_str());
    Row("OK").emit(out);
}

static void cmdStops(DynArray<String>&, String& out) {
    DynArray<Flight> fl;
    loadFlights(fl);
    DynArray<String> codes;
    collectStops(fl, codes);
    DynArray<Airport> ap;
    loadAirports(ap);
    Row("OK").n(codes.size()).emit(out);
    for (int i = 0; i < codes.size(); i++) {
        int k = binarySearch(ap, codes[i], airportKeyCmp);
        Row("A").f(codes[i]).f(k >= 0 ? ap[k].name : codes[i]).emit(out);
    }
}

static void cmdSearch(DynArray<String>& a, String& out) {
    if (!needArgs(a, 4, out)) return;
    String from = a[1].trimmed().upper(), to = a[2].trimmed().upper();
    long long day;
    if (!parseDate(a[3], &day)) { fail(out, "bad_date", "Invalid date."); return; }
    DynArray<Flight> fl;
    loadFlights(fl);
    DynArray<String> stops;
    collectStops(fl, stops);
    int si = stopIndex(stops, from), ti = stopIndex(stops, to);
    if (si < 0 || ti < 0) { fail(out, "unknown_airport", "Unknown airport."); return; }
    if (si == ti) { fail(out, "same_airport", "Origin and destination must be different."); return; }

    long long now = nowCivil();
    int n = stops.size();
    Graph g(n);
    for (int i = 0; i < fl.size(); i++) g.addEdge(stopIndex(stops, fl[i].origin), stopIndex(stops, fl[i].dest), i);

    ScheduleModel model(&fl, day, now);
    int* prevNode = new int[n];
    int* prevEdge = new int[n];
    long long* arrival = new long long[n];
    bool found = g.earliestArrival(si, ti, day * 86400LL, &model, prevNode, prevEdge, arrival);

    Row("OK").emit(out);
    DynArray<int> usedFlights;
    if (found) {
        Stack<int> path;                                  // edges come out destination -> source
        for (int cur = ti; cur != si; cur = prevNode[cur]) path.push(prevEdge[cur]);
        DynArray<int> legFlight;
        DynArray<long long> legDep, legArr;
        long long t = day * 86400LL;
        bool first = true;
        double totalFare = 0;
        String route = from;
        while (!path.empty()) {
            int fi = path.pop();
            long long dep = model.departureFor(fi, t, first);
            long long arr = dep + fl[fi].durMin * 60LL;
            legFlight.push(fi); legDep.push(dep); legArr.push(arr);
            totalFare += fl[fi].fare;
            route += ">";
            route += fl[fi].dest;
            t = arr;
            first = false;
        }
        long long totalMin = (legArr[legArr.size() - 1] - legDep[0]) / 60;
        Row("ROUTE").f(route).n(totalMin).d(totalFare, 2).n(legFlight.size()).emit(out);
        for (int i = 0; i < legFlight.size(); i++) {
            emitLeg(out, "ROUTE", i + 1, fl[legFlight[i]], legDep[i], legArr[i], now);
            usedFlights.push(legFlight[i]);
        }
    } else {
        Row("NOROUTE").emit(out);
    }
    // other direct flights between the two airports on that day
    DynArray<Flight> alts;
    for (int i = 0; i < fl.size(); i++) {
        if (fl[i].origin != from || fl[i].dest != to || !fl[i].operatesOn(day)) continue;
        bool used = false;
        for (int k = 0; k < usedFlights.size(); k++) if (usedFlights[k] == i) used = true;
        if (!used) alts.push(fl[i]);
    }
    insertionSort(alts, flightByDepCmp);
    for (int i = 0; i < alts.size(); i++) {
        long long dep = alts[i].departure(day);
        emitLeg(out, "ALT", i + 1, alts[i], dep, dep + alts[i].durMin * 60LL, now);
    }
    delete[] prevNode;
    delete[] prevEdge;
    delete[] arrival;
}

static void cmdSeats(DynArray<String>& a, String& out) {
    if (!needArgs(a, 3, out)) return;
    String number = a[1].trimmed();
    long long day;
    if (!parseDate(a[2], &day)) { fail(out, "bad_date", "Invalid date."); return; }
    DynArray<Flight> fl;
    loadFlights(fl);
    int fi = findFlight(fl, number);
    if (fi < 0) { fail(out, "flight_not_found", "Unknown flight."); return; }
    const Flight& f = fl[fi];
    String code, reason;
    bool ok = bookingOpen(f, day, nowCivil(), code, reason);
    Row("OK").f(f.number).f(f.origin).f(f.dest).f(fmtDate(day)).f(fmtHM(f.depMin)).d(f.fare, 2)
        .n(f.seats).n(ok ? 1 : 0).f(reason).emit(out);
    DynArray<PaxRecord> sold;
    loadBooking(f.number, fmtDate(day), sold);
    Row r("SOLD");
    for (int i = 0; i < sold.size(); i++) r.f(sold[i].seat);
    r.emit(out);
}

static void cmdBook(DynArray<String>& a, String& out) {
    if (!needArgs(a, 5, out)) return;
    String user = a[1].trimmed(), number = a[2].trimmed();
    long long day;
    if (!parseDate(a[3], &day)) { fail(out, "bad_date", "Invalid date."); return; }
    bool okc;
    long long count = a[4].toLong(&okc);
    if (!okc || count < 1 || count > MAX_PAX_PER_BOOKING) { fail(out, "bad_request", "Between 1 and 9 seats can be booked at once."); return; }
    if (a.size() != 5 + 5 * (int)count) { fail(out, "bad_request", "Passenger details are incomplete."); return; }

    AccountStore ps(F_PASSENGERS, KIND_PASSENGER);
    ps.load();
    if (ps.find(user) < 0) { fail(out, "unknown_user", "Please log in again."); return; }

    DynArray<Flight> fl;
    loadFlights(fl);
    int fi = findFlight(fl, number);
    if (fi < 0) { fail(out, "flight_not_found", "Unknown flight."); return; }
    const Flight& f = fl[fi];
    long long now = nowCivil();
    String code, reason;
    if (!bookingOpen(f, day, now, code, reason)) { fail(out, code.c_str(), reason.c_str()); return; }

    String date = fmtDate(day);
    DynArray<PaxRecord> existing;
    loadBooking(number, date, existing);                  // sorted by seat -> binary search below

    DynArray<PaxRecord> fresh;
    for (int k = 0; k < (int)count; k++) {
        int b = 5 + 5 * k;
        PaxRecord p;
        p.seat = normalizeSeat(a[b]);
        p.name = clean(a[b + 1], 40);
        p.gender = clean(a[b + 3], 12);
        bool okAge, okBag;
        long long age = a[b + 2].toLong(&okAge);
        long long bag = a[b + 4].toLong(&okBag);
        if (!validSeat(p.seat, f.seats)) { fail(out, "bad_seat", "Invalid seat selected."); return; }
        if (binarySearch(existing, p.seat, paxSeatKeyCmp) >= 0) {
            String m = String("Seat ") + p.seat + " has just been booked by someone else. Please choose another seat.";
            fail(out, "seat_taken", m.c_str());
            return;
        }
        for (int q = 0; q < fresh.size(); q++)
            if (fresh[q].seat == p.seat) { fail(out, "bad_seat", "The same seat was selected twice."); return; }
        if (p.name.empty()) { fail(out, "bad_name", "Every passenger needs a name."); return; }
        if (!okAge || age < 0 || age > 120) { fail(out, "bad_age", "Passenger age must be between 0 and 120."); return; }
        if (!okBag || bag < 0 || bag > 40) { fail(out, "bad_bag", "Checked bag weight must be between 0 and 40 kg."); return; }
        if (p.gender.empty()) p.gender = "Other";
        p.age = (int)age;
        p.bagKg = (int)bag;
        fresh.push(p);
    }

    // unique booking reference (binary search over the sorted ticket index)
    DynArray<TicketRec> tickets;
    loadTickets(tickets);
    quickSort(tickets, ticketPnrCmp);
    String pnr = randomPnr();
    while (binarySearch(tickets, pnr, ticketPnrKeyCmp) >= 0) pnr = randomPnr();

    String bookedAt = fmtDateTime(now);
    DynArray<PaxRecord> known = existing;                 // used to keep bag tags unique
    for (int k = 0; k < fresh.size(); k++) {
        PaxRecord& p = fresh[k];
        p.pnr = pnr;
        p.username = user;
        p.code = randomDigits(6);
        p.checkedIn = false;
        p.checkInTime = "-";
        p.bookedAt = bookedAt;
        if (p.bagKg > 0) {
            quickSort(known, paxTagCmp);
            String tag;
            do { tag = String("BT") + randomDigits(8); } while (binarySearch(known, tag, paxTagKeyCmp) >= 0);
            p.bagTag = tag;
            p.bagStatus = "NONE";
        } else {
            p.bagTag = "-";
            p.bagStatus = "NOBAG";
        }
        known.push(p);
    }
    for (int k = 0; k < fresh.size(); k++)
        if (!appendBooking(number, date, fresh[k])) { fail(out, "file_error", "Could not write the booking file."); return; }

    TicketRec t;
    t.username = user; t.pnr = pnr; t.flight = number; t.date = date;
    t.seats = (int)count; t.total = f.fare * count; t.bookedAt = bookedAt;
    appendLine(F_TICKETS, t.toLine());

    Row("OK").f(pnr).d(t.total, 2).emit(out);
    for (int k = 0; k < fresh.size(); k++)
        Row("PAX").f(fresh[k].seat).f(fresh[k].name).f(fresh[k].code).f(fresh[k].bagTag).n(fresh[k].bagKg).emit(out);
    printf("[booking] %s booked %d seat(s) on %s %s  PNR %s\n", user.c_str(), (int)count, number.c_str(), date.c_str(), pnr.c_str());
}

// all boarding passes of one passenger account
static void cmdMyPasses(DynArray<String>& a, String& out) {
    if (!needArgs(a, 2, out)) return;
    String user = a[1].trimmed();
    DynArray<TicketRec> tickets;
    loadTickets(tickets);
    heapSort(tickets, ticketUserCmp);
    DynArray<Flight> fl;
    loadFlights(fl);
    Row("OK").emit(out);
    int i = lowerBound(tickets, user, ticketUserKeyCmp);       // binary search for the first ticket of the user
    int end = upperBound(tickets, user, ticketUserKeyCmp);
    for (; i < end; i++) {
        const TicketRec& t = tickets[i];
        int fi = findFlight(fl, t.flight);
        if (fi < 0) continue;
        const Flight& f = fl[fi];
        long long day = 0;
        parseDate(t.date, &day);
        DynArray<PaxRecord> pax;
        loadBooking(t.flight, t.date, pax);
        for (int k = 0; k < pax.size(); k++) {
            const PaxRecord& p = pax[k];
            if (p.pnr != t.pnr) continue;
            Row("BP").f(p.pnr).f(p.seat).f(p.name).n(p.age).f(p.gender).f(f.number).f(f.origin).f(f.dest)
                .f(t.date).f(fmtHM(f.depMin)).f(fmtHM(((f.depMin - BOARDING_BEFORE_MIN) % 1440 + 1440) % 1440))
                .f(gateFor(f.number, day)).f(p.code).f(p.bagTag).n(p.bagKg).n(p.checkedIn ? 1 : 0)
                .f(p.bagStatus).f(p.bookedAt).emit(out);
        }
    }
}

// ----------------------------------------------------------------------------
//  Check-in staff
// ----------------------------------------------------------------------------
static void cmdCheckInFlights(DynArray<String>& a, String& out) {
    if (!needArgs(a, 2, out)) return;
    long long day;
    if (!parseDate(a[1], &day)) { fail(out, "bad_date", "Invalid date."); return; }
    DynArray<Flight> fl, ops;
    loadFlights(fl);
    for (int i = 0; i < fl.size(); i++) if (fl[i].operatesOn(day)) ops.push(fl[i]);
    insertionSort(ops, flightByDepCmp);
    long long now = nowCivil();
    Row("OK").n(ops.size()).emit(out);
    for (int i = 0; i < ops.size(); i++) {
        const Flight& f = ops[i];
        String date = fmtDate(day);
        DynArray<PaxRecord> pax;
        loadBooking(f.number, date, pax);
        int checked = 0;
        for (int k = 0; k < pax.size(); k++) if (pax[k].checkedIn) checked++;
        long long closes = f.departure(day) - CHECKIN_CUTOFF_MIN * 60LL;
        Row("F").f(f.number).f(f.origin).f(f.dest).f(fmtHM(f.depMin)).n(pax.size()).n(checked)
            .n(now <= closes ? 1 : 0).f(fmtDateHM(closes)).emit(out);
    }
}

static void cmdCheckInList(DynArray<String>& a, String& out) {
    if (!needArgs(a, 3, out)) return;
    String number = a[1].trimmed();
    long long day;
    if (!parseDate(a[2], &day)) { fail(out, "bad_date", "Invalid date."); return; }
    DynArray<Flight> fl;
    loadFlights(fl);
    int fi = findFlight(fl, number);
    if (fi < 0) { fail(out, "flight_not_found", "Unknown flight."); return; }
    const Flight& f = fl[fi];
    long long closes = f.departure(day) - CHECKIN_CUTOFF_MIN * 60LL;
    Row("OK").n(nowCivil() <= closes ? 1 : 0).f(fmtDateHM(closes)).f(fmtHM(f.depMin)).f(f.origin).f(f.dest).emit(out);
    DynArray<PaxRecord> pax;
    loadBooking(number, fmtDate(day), pax);
    for (int i = 0; i < pax.size(); i++)
        Row("P").f(pax[i].pnr).f(pax[i].seat).f(pax[i].name).n(pax[i].age).f(pax[i].gender)
            .n(pax[i].checkedIn ? 1 : 0).f(pax[i].checkInTime).n(pax[i].bagKg).emit(out);   // the check-in code is NOT sent
}

static void cmdCheckIn(DynArray<String>& a, String& out) {
    if (!needArgs(a, 7, out)) return;
    String staff = a[1].trimmed(), number = a[2].trimmed(), pnr = a[4].trimmed().upper();
    String seat = normalizeSeat(a[5]), code = a[6].trimmed();
    long long day;
    if (!staffIsType(staff, "CHECKIN")) { fail(out, "not_allowed", "Only check-in staff can check passengers in."); return; }
    if (!parseDate(a[3], &day)) { fail(out, "bad_date", "Invalid date."); return; }
    DynArray<Flight> fl;
    loadFlights(fl);
    int fi = findFlight(fl, number);
    if (fi < 0) { fail(out, "flight_not_found", "Unknown flight."); return; }
    String date = fmtDate(day);
    DynArray<PaxRecord> pax;
    if (!loadBooking(number, date, pax)) { fail(out, "not_found", "No bookings exist for this flight and date."); return; }
    int idx = locatePax(pax, pnr, seat);                  // binary search on (PNR, seat)
    if (idx < 0) { fail(out, "not_found", "Passenger not found for that PNR and seat."); return; }
    if (pax[idx].checkedIn) { fail(out, "already_checked_in", "This passenger is already checked in."); return; }
    long long now = nowCivil();
    long long closes = fl[fi].departure(day) - CHECKIN_CUTOFF_MIN * 60LL;
    if (now > closes) {
        String m = String("Check-in is allowed only up to 3 hours before departure (closed at ") + fmtDateHM(closes) + ").";
        fail(out, "checkin_closed", m.c_str());
        return;
    }
    if (pax[idx].code != code) { fail(out, "bad_code", "The check-in code does not match the boarding pass."); return; }
    pax[idx].checkedIn = true;
    pax[idx].checkInTime = fmtDateTime(now);
    saveBooking(number, date, pax);
    printf("[check-in] %s checked in %s (%s) seat %s on %s %s\n", staff.c_str(), pax[idx].name.c_str(), pnr.c_str(),
           seat.c_str(), number.c_str(), date.c_str());
    Row("OK").f(pax[idx].name).f(seat).emit(out);
}

// ----------------------------------------------------------------------------
//  Simulator
// ----------------------------------------------------------------------------
static void cmdSimSetTime(DynArray<String>& a, String& out) {
    if (!needArgs(a, 2, out)) return;
    long long target;
    if (!parseDateTime(a[1], &target)) { fail(out, "bad_time", "Use the format YYYY-MM-DD HH:MM:SS."); return; }
    simOffset = target - realCivilNow();
    simActive = true;
    printf("[simulator] system clock set to %s\n", fmtDateTime(target).c_str());
    Row("OK").f(fmtDateTime(nowCivil())).emit(out);
}
static void cmdSimReset(DynArray<String>&, String& out) {
    simOffset = 0;
    simActive = false;
    printf("[simulator] system clock back to real time\n");
    Row("OK").f(fmtDateTime(nowCivil())).emit(out);
}

static bool inSimWindow(const Flight& f, long long day, long long now) {
    long long dep = f.departure(day);
    return dep >= now && dep <= now + SIM_WINDOW_MIN * 60LL;
}

static void cmdSimFlights(DynArray<String>&, String& out) {
    DynArray<Flight> fl, hits;
    DynArray<long long> hitDays;
    loadFlights(fl);
    long long now = nowCivil();
    long long today = floorDiv(now, 86400);
    for (int d = 0; d < 2; d++)
        for (int i = 0; i < fl.size(); i++)
            if (fl[i].operatesOn(today + d) && inSimWindow(fl[i], today + d, now)) { hits.push(fl[i]); hitDays.push(today + d); }
    // order by absolute departure time (small list -> insertion sort on an index array)
    DynArray<int> order;
    for (int i = 0; i < hits.size(); i++) order.push(i);
    for (int i = 1; i < order.size(); i++) {
        int key = order[i], j = i - 1;
        while (j >= 0 && hits[order[j]].departure(hitDays[order[j]]) > hits[key].departure(hitDays[key])) { order[j + 1] = order[j]; j--; }
        order[j + 1] = key;
    }
    Row("OK").n(order.size()).emit(out);
    for (int o = 0; o < order.size(); o++) {
        const Flight& f = hits[order[o]];
        String date = fmtDate(hitDays[order[o]]);
        DynArray<PaxRecord> pax;
        loadBooking(f.number, date, pax);
        int checked = 0, pending = 0, onBelt = 0;
        for (int k = 0; k < pax.size(); k++) {
            if (pax[k].checkedIn) checked++;
            if (pax[k].checkedIn && pax[k].bagStatus == "NONE") pending++;
            if (pax[k].bagStatus == "ONBELT") onBelt++;
        }
        Row("F").f(f.number).f(f.origin).f(f.dest).f(date).f(fmtHM(f.depMin)).n(pax.size()).n(checked).n(pending).n(onBelt).emit(out);
    }
}

static void cmdSimCheckedIn(DynArray<String>& a, String& out) {
    if (!needArgs(a, 3, out)) return;
    long long day;
    if (!parseDate(a[2], &day)) { fail(out, "bad_date", "Invalid date."); return; }
    DynArray<PaxRecord> pax;
    loadBooking(a[1].trimmed(), fmtDate(day), pax);
    Row("OK").emit(out);
    for (int i = 0; i < pax.size(); i++) {
        if (!pax[i].checkedIn) continue;                  // only checked-in passengers are offered
        Row("P").f(pax[i].pnr).f(pax[i].seat).f(pax[i].name).n(pax[i].age).f(pax[i].gender)
            .f(pax[i].bagTag).n(pax[i].bagKg).f(pax[i].bagStatus).f(pax[i].checkInTime).emit(out);
    }
}

static void cmdSimPush(DynArray<String>& a, String& out) {
    if (!needArgs(a, 5, out)) return;
    String number = a[1].trimmed();
    long long day;
    if (!parseDate(a[2], &day)) { fail(out, "bad_date", "Invalid date."); return; }
    String date = fmtDate(day), pnr = a[3].trimmed().upper(), seat = normalizeSeat(a[4]);
    DynArray<Flight> fl;
    loadFlights(fl);
    int fi = findFlight(fl, number);
    if (fi < 0) { fail(out, "flight_not_found", "Unknown flight."); return; }
    long long now = nowCivil();
    if (!fl[fi].operatesOn(day) || !inSimWindow(fl[fi], day, now)) {
        fail(out, "outside_window", "The flight is not departing within 3 hours of the system time."); return;
    }
    DynArray<PaxRecord> pax;
    if (!loadBooking(number, date, pax)) { fail(out, "not_found", "No bookings for this flight."); return; }
    int idx = locatePax(pax, pnr, seat);
    if (idx < 0) { fail(out, "not_found", "Passenger not found."); return; }
    PaxRecord& p = pax[idx];
    if (!p.checkedIn) { fail(out, "not_checked_in", "This passenger has not checked in yet."); return; }
    if (p.bagKg <= 0) { fail(out, "no_bag", "This passenger has no checked bag."); return; }
    if (p.bagStatus != "NONE") { fail(out, "already_pushed", "This bag has already been put on the belt."); return; }

    belt->tick(realNowSec());
    BagInfo b;
    b.tag = p.bagTag; b.flight = number; b.date = date; b.pnr = p.pnr; b.seat = p.seat;
    b.name = p.name; b.username = p.username; b.weightKg = p.bagKg;
    double wait = 0;
    if (!belt->pushBag(b, realNowSec(), &wait)) {
        String m = String("The start of the belt is occupied. Try again in ") + String::fromInt((long long)wait + 1) + " s.";
        fail(out, "belt_busy", m.c_str());
        return;
    }
    p.bagStatus = "ONBELT";
    saveBooking(number, date, pax);
    DNode<BagInfo>* node = belt->findTag(p.bagTag);
    if (!node) { fail(out, "internal", "Bag could not be registered on the belt."); return; }
    printf("[belt] bag %s (%s, %s %s seat %s) crossed the start RFID\n", p.bagTag.c_str(), p.name.c_str(),
           number.c_str(), date.c_str(), seat.c_str());
    Row("OK").f(p.bagTag).emit(out);
    for (int j = 0; j < belt->sensorCount(); j++)
        Row("ET").n(j).f(belt->sensor(j)->name).f(belt->sensor(j)->kind())
            .f(fmtTime(node->data.pushCivil + (long long)belt->offsetOf(j))).d(belt->offsetOf(j), 1).emit(out);
}

static void cmdSimOnBelt(DynArray<String>& a, String& out) {
    if (!needArgs(a, 3, out)) return;
    String number = a[1].trimmed();
    long long day;
    if (!parseDate(a[2], &day)) { fail(out, "bad_date", "Invalid date."); return; }
    String date = fmtDate(day);
    belt->tick(realNowSec());
    Row("OK").emit(out);
    for (DNode<BagInfo>* n = belt->bags().head(); n; n = n->next) {
        const BagInfo& b = n->data;
        if (b.flight != number || b.date != date) continue;
        Row("B").f(b.pnr).f(b.seat).f(b.name).f(b.tag).n(b.weightKg).f(bagStateName(b.state)).n(b.passed)
            .n(b.dropAt).n(b.lostAt).emit(out);
    }
}

static void cmdSimLoss(DynArray<String>& a, String& out) {
    if (!needArgs(a, 6, out)) return;
    String number = a[1].trimmed();
    long long day;
    if (!parseDate(a[2], &day)) { fail(out, "bad_date", "Invalid date."); return; }
    String date = fmtDate(day), pnr = a[3].trimmed().upper(), seat = normalizeSeat(a[4]);
    bool ok;
    long long sensor = a[5].toLong(&ok);
    belt->tick(realNowSec());
    DNode<BagInfo>* n = belt->findBag(number, date, pnr, seat);
    if (!n || n->data.state != BAG_ONBELT) { fail(out, "not_on_belt", "This bag is not travelling on the belt."); return; }
    if (!ok || sensor < 1 || sensor >= belt->sensorCount()) { fail(out, "bad_sensor", "Choose a sensor after the start RFID."); return; }
    if ((int)sensor <= n->data.passed) { fail(out, "sensor_passed", "The bag has already passed that sensor."); return; }
    n->data.dropAt = (int)sensor;
    printf("[simulator] bag %s will go missing at sensor %d (%s)\n", n->data.tag.c_str(), (int)sensor,
           belt->sensor((int)sensor)->name.c_str());
    Row("OK").f(n->data.name).f(belt->sensor((int)sensor)->name)
        .f(fmtTime(n->data.pushCivil + (long long)belt->offsetOf((int)sensor))).emit(out);
}

// ----------------------------------------------------------------------------
//  Belt view (staff sees every bag, a passenger only his own)
// ----------------------------------------------------------------------------
static void cmdBelt(DynArray<String>& a, String& out) {
    if (!needArgs(a, 3, out)) return;
    bool staffView = (a[1].trimmed().upper() == "STAFF");
    String user = a[2].trimmed();
    double now = realNowSec();
    belt->tick(now);
    Row("OK").f(fmtDateTime(nowCivil())).d(BELT_SPEED_MPS, 2).d(belt->length(), 1).emit(out);
    for (int j = 0; j < belt->sensorCount(); j++)
        Row("SENSOR").n(j).f(belt->sensor(j)->name).f(belt->sensor(j)->kind()).d(belt->sensor(j)->distance, 1).emit(out);

    int skip = belt->bags().size() - BELT_REPORT_BAGS;
    int idx = 0;
    for (DNode<BagInfo>* n = belt->bags().head(); n; n = n->next, idx++) {
        if (idx < skip) continue;
        const BagInfo& b = n->data;
        if (!staffView && b.username != user) continue;
        Row("BAG").f(b.tag).f(b.flight).f(b.date).f(b.pnr).f(b.seat).f(b.name).n(b.weightKg).f(bagStateName(b.state))
            .n(b.passed).n(b.lostAt).d(belt->positionOf(b, now), 2).n(b.pushCivil).f(b.lostTime.empty() ? "-" : b.lostTime.c_str())
            .f(b.deliveredTime.empty() ? "-" : b.deliveredTime.c_str())
            .f(b.recoveredInfo.empty() ? "-" : b.recoveredInfo.c_str()).emit(out);
    }
    if (!staffView) {                                    // bags that are booked but not on the belt yet
        DynArray<TicketRec> tickets;
        loadTickets(tickets);
        heapSort(tickets, ticketUserCmp);
        DynArray<Flight> fl;
        loadFlights(fl);
        long long cnow = nowCivil();
        int i = lowerBound(tickets, user, ticketUserKeyCmp), end = upperBound(tickets, user, ticketUserKeyCmp);
        for (; i < end; i++) {
            int fi = findFlight(fl, tickets[i].flight);
            long long day;
            if (fi < 0 || !parseDate(tickets[i].date, &day) || fl[fi].departure(day) < cnow) continue;
            DynArray<PaxRecord> pax;
            loadBooking(tickets[i].flight, tickets[i].date, pax);
            for (int k = 0; k < pax.size(); k++)
                if (pax[k].pnr == tickets[i].pnr && pax[k].bagStatus == "NONE")
                    Row("PENDING").f(pax[k].bagTag).f(pax[k].name).f(tickets[i].flight).f(tickets[i].date)
                        .f(fmtHM(fl[fi].depMin)).n(pax[k].checkedIn ? 1 : 0).n(pax[k].bagKg).emit(out);
        }
    }
}

static void cmdBeltSafe(DynArray<String>& a, String& out) {
    if (!needArgs(a, 3, out)) return;
    String staff = a[1].trimmed(), tag = a[2].trimmed();
    if (!staffIsType(staff, "BELT")) { fail(out, "not_allowed", "Only belt staff can update bag status."); return; }
    belt->tick(realNowSec());
    DNode<BagInfo>* n = belt->findTag(tag);
    if (!n) { fail(out, "not_found", "Unknown bag."); return; }
    if (n->data.state != BAG_LOST) { fail(out, "not_lost", "This bag is not reported lost."); return; }
    n->data.state = BAG_RECOVERED;
    n->data.recoveredInfo = staff + " at " + fmtDateTime(nowCivil());
    updateBagStatus(n->data.flight, n->data.date, n->data.pnr, n->data.seat, "RECOVERED");
    printf("[belt] bag %s recovered by %s\n", tag.c_str(), staff.c_str());
    Row("OK").emit(out);
}

// ----------------------------------------------------------------------------
//  Dispatcher
// ----------------------------------------------------------------------------
static bool isPollingCommand(const String& c) {
    return c == "TIME" || c == "BELT" || c == "SEATS" || c == "CI_LIST" || c == "CI_FLIGHTS" || c == "PING"
        || c == "SIM_FLIGHTS" || c == "SIM_CHECKEDIN" || c == "SIM_ONBELT" || c == "STOPS" || c == "MYPASSES";
}

static void handleRequest(const String& line, String& out) {
    DynArray<String> a;
    String::split(line.trimmed(), '|', a);
    String cmd = a[0].trimmed().upper();
    if (!isPollingCommand(cmd)) printf("[request] %s\n", cmd.c_str());       // never log arguments (passwords)
    out.clear();
    if      (cmd == "PING")            Row("OK").f("pong").emit(out);
    else if (cmd == "TIME")            cmdTime(a, out);
    else if (cmd == "LOGIN")           cmdLogin(a, out);
    else if (cmd == "REGISTER")        cmdRegister(a, out);
    else if (cmd == "ADMIN_LISTSTAFF") cmdAdminListStaff(a, out);
    else if (cmd == "ADMIN_ADDSTAFF")  cmdAdminAddStaff(a, out);
    else if (cmd == "STOPS")           cmdStops(a, out);
    else if (cmd == "SEARCH")          cmdSearch(a, out);
    else if (cmd == "SEATS")           cmdSeats(a, out);
    else if (cmd == "BOOK")            cmdBook(a, out);
    else if (cmd == "MYPASSES")        cmdMyPasses(a, out);
    else if (cmd == "CI_FLIGHTS")      cmdCheckInFlights(a, out);
    else if (cmd == "CI_LIST")         cmdCheckInList(a, out);
    else if (cmd == "CI_DO")           cmdCheckIn(a, out);
    else if (cmd == "SIM_SETTIME")     cmdSimSetTime(a, out);
    else if (cmd == "SIM_RESET")       cmdSimReset(a, out);
    else if (cmd == "SIM_FLIGHTS")     cmdSimFlights(a, out);
    else if (cmd == "SIM_CHECKEDIN")   cmdSimCheckedIn(a, out);
    else if (cmd == "SIM_PUSH")        cmdSimPush(a, out);
    else if (cmd == "SIM_ONBELT")      cmdSimOnBelt(a, out);
    else if (cmd == "SIM_LOSS")        cmdSimLoss(a, out);
    else if (cmd == "BELT")            cmdBelt(a, out);
    else if (cmd == "BELT_SAFE")       cmdBeltSafe(a, out);
    else fail(out, "unknown_command", "Unknown command.");
}

// ============================================================================
//  Networking (plain TCP, one request per connection)
// ============================================================================
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

static bool recvLine(sock_t c, String& line) {
    line.clear();
    double deadline = realNowSec() + 3.0;
    char buf[2048];
    while (line.length() < 65536) {
        double left = deadline - realNowSec();
        if (left <= 0) return false;
        fd_set rf;
        FD_ZERO(&rf);
        FD_SET(c, &rf);
        struct timeval tv;
        tv.tv_sec = (long)left;
        tv.tv_usec = (long)((left - (long)left) * 1000000.0);
        int r = select((int)c + 1, &rf, 0, 0, &tv);
        if (r <= 0) return false;
        int n = (int)recv(c, buf, sizeof(buf), 0);
        if (n <= 0) return line.length() > 0;
        for (int i = 0; i < n; i++) {
            if (buf[i] == '\n') return true;
            line.appendChar(buf[i]);
        }
    }
    return false;
}

static void sendAll(sock_t c, const String& s) {
    int sent = 0;
    while (sent < s.length()) {
        int n = (int)send(c, s.c_str() + sent, s.length() - sent, MSG_NOSIGNAL);
        if (n <= 0) break;
        sent += n;
    }
}

static void printAddresses(int port) {
    char host[256];
    if (gethostname(host, sizeof(host)) != 0) return;
    struct hostent* he = gethostbyname(host);
    if (!he) return;
    for (char** p = he->h_addr_list; *p; ++p) {
        struct in_addr addr;
        memcpy(&addr, *p, sizeof(addr));
        printf("   clients on other laptops connect to  %s:%d\n", inet_ntoa(addr), port);
    }
}

int main(int argc, char** argv) {
    int port = 8080;
    if (argc > 1) port = atoi(argv[1]);
    if (port <= 0 || port > 65535) port = 8080;

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { printf("WSAStartup failed\n"); return 1; }
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    setvbuf(stdout, 0, _IONBF, 0);                        // log lines appear immediately, even when redirected to a file
    srand((unsigned)(time(0) ^ (unsigned)(realNowSec() * 1000.0)));
    ensureDefaults();
    belt = new Belt();

    sock_t ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls == INVALID_SOCKET) {
        cout << "cannot create socket" << endl;
        return 1;
    }
#ifndef _WIN32
    int yes = 1;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
#endif
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);
    if (bind(ls, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        cout << "Cannot bind port " << port << " is the server already running?" << endl;
        return 1;
    }
    if (listen(ls, 32) != 0) {
        cout << "Listen failed" << endl;
        return 1;
    }

    cout << "=== Airline & Airport server running on port " << port << " ===" << endl;
    printAddresses(port);
    printf( "System clock: %s\n", fmtDateTime(nowCivil()).c_str());
    cout << "Press Ctrl+C to stop\n\n";

    for (;;) {
        fd_set rf;
        FD_ZERO(&rf);
        FD_SET(ls, &rf);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000;                              // wake up 5x per second to run the belt timetable
        int r = select((int)ls + 1, &rf, 0, 0, &tv);
        belt->tick(realNowSec());
        if (r <= 0 || !FD_ISSET(ls, &rf)) continue;
        
        struct sockaddr_in peer;
        sock_len_t plen = sizeof(peer);
        sock_t c = accept(ls, (struct sockaddr*)&peer, &plen);
        if (c == INVALID_SOCKET) continue;
        
        String request, response;
        if (recvLine(c, request)) {
            handleRequest(request, response);
            sendAll(c, response);
        }
        shutdown(c, 1);
        CLOSESOCK(c);
    }
    return 0;
}