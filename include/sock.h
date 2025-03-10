#ifndef SOCK_H
#define SOCK_H
#ifdef _WIN32
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0600
    #endif
    #define WIN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define is_invalid_sock(x) (x == INVALID_SOCKET)
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>
    #define SOCKET int
    #define is_invalid_sock(x) (x < 0)
#endif

typedef struct sockaddr_in sockaddr_in;

typedef enum status {
    OFFLINE,
    ONLINE
} STATUS;

typedef struct peer {
    sockaddr_in con;
    STATUS status;
} peer;

#ifdef WIN
int init_win_sock(void);
#endif

//standard function for socket creation
void sock_close(SOCKET sock);
void create_address(sockaddr_in *address, char *ip);
//create peers
peer * create_peers(char **peers_ip, int peers_size);

void show_peers(peer *peers, int peers_size);

#endif