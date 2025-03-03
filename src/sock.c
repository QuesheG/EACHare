#include <sock.h>

#ifdef WIN
int init_win_sock(void) {
    WSADATA d;
    int iResult = WSAStartup(MAKEWORD(2, 2), &d);
    if (iResult != 0) {
        fprintf(stderr, "Failed to initialize. %d\n", WSAGetLastError());
        return 1;
    }
    return iResult;
}
#endif

//standard function for socket creation
void sock_close(SOCKET sock) {
    #ifdef WIN
        closesocket(sock);
        return;
    #else
        close(sock);
    #endif
}

//create peers
create_peers(char **peers_ip, int peers_size) {
    //ainda nem sei pra falar a real pqp
}