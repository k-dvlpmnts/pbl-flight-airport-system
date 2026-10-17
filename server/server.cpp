#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
int main(int argc, char** argv) {
    int port = 8080;
    if (argc > 1) port = atoi(argv[1]);
    if (port <= 0 || port > 65535) port = 8080;

    srand((unsigned)(time(0) ^ (unsigned)(realNowSec() * 1000.0)));
    ensureDefaults();
    belt = new Belt();

    sock_t ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls == INVALID_SOCKET) { printf("cannot create socket\n"); return 1; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);
    if (bind(ls, (struct sockaddr*)&addr, sizeof(addr)) != 0) { printf("cannot bind port %d (is the server already running?)\n", port); return 1; }
    if (listen(ls, 32) != 0) { printf("listen failed\n"); return 1; }

    printf("=== Airline & Airport server running on port %d ===\n", port);
    printAddresses(port);
    printf("   system clock: %s\n", fmtDateTime(nowCivil()).c_str());
    printf("   (press Ctrl+C to stop)\n\n");

    for (;;) {
        fd_set rf;
        FD_ZERO(&rf);
        FD_SET(ls, &rf);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000;                              
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
