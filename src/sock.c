#include <sock.h>

#ifdef WIN
int init_win_sock(void) {
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize. %d\n", WSAGetLastError());
    }
}
#endif

void sock_close(SOCKET sock) {
    #ifdef WIN
        closesocket(sock);
        return;
    #else
        close(sock);
    #endif
}