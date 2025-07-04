#ifndef _SOCK_H
#define _SOCK_H
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
#define INVALID_SOCKET -1
#endif

#include <stdint.h>
#include <stdbool.h>

#ifdef WIN
int init_win_sock(void);
#endif

void sock_close(SOCKET sock); // standard function for socket creation
void show_soc_error(); // standard function for socket error messages
bool set_sock_block(SOCKET sock, bool blocking);
void set_sock_timeout(SOCKET sock, uint8_t touts);
int send_complete(SOCKET sock, const void *buf, size_t len, int flag);

#endif
