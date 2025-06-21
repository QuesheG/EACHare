#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sock.h>
#include <fcntl.h>

#ifdef WIN
int init_win_sock(void) {
    WSADATA d;
    int iResult = WSAStartup(MAKEWORD(2, 2), &d);
    if(iResult != 0) {
        fprintf(stderr, "Falha na inicializacao. %d\n", WSAGetLastError());
        return 1;
    }
    return iResult;
}
#endif

// standard function for socket creation
void sock_close(SOCKET sock) {
#ifdef WIN
    closesocket(sock);
    return;
#else
    close(sock);
#endif
}
// standard function for socket error messages
void show_soc_error() {
#ifdef _WIN32
    wchar_t *s = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&s, 0, NULL);
    fprintf(stderr, "%d: %S\n", WSAGetLastError(), s);
    LocalFree(s);
#else
    fprintf(stderr, "%d: %s\n", errno, strerror(errno));
#endif
}

bool set_sock_block(SOCKET sock, bool blocking)
{
   if (sock < 0) return false;

#ifdef _WIN32
   unsigned long mode = blocking ? 0 : 1;
   return (ioctlsocket(sock, FIONBIO, &mode) == 0);
#else
   int flags = fcntl(sock, F_GETFL, 0);
   if (flags == -1) return false;
   flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
   return (fcntl(sock, F_SETFL, flags) == 0);
#endif
}

void set_sock_timeout(SOCKET sock, uint8_t touts) {
    #ifdef WIN
    DWORD timeout = touts * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);
    #else
    struct timeval tv;
    tv.tv_sec = touts;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    #endif
}

// send full message
int send_complete(SOCKET sock, const void *buf, size_t len, int flag) {
    size_t sent = 0;
    while(sent < len) {
        ssize_t n = send(sock, (char *)buf + sent, len - sent, flag);
        if(n <= 0)
            return -1;
        sent += n;
    }
    return 0;
}
