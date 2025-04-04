#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sock.h>

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
