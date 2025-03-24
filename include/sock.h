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

#include <stdbool.h>
#include <pthread.h>

#define MSG_SIZE 50

#define xstr(s) str(s)
#define str(s) #s

typedef enum status
{
    OFFLINE,
    ONLINE
} STATUS;

typedef enum msg_type
{
    HELLO,
    BYE,

    UNEXPECTED_MSG_TYPE
} MSG_TYPE;

typedef struct sockaddr_in sockaddr_in;

typedef struct peer
{
    sockaddr_in con;
    STATUS status;
} peer;

#ifdef WIN
int init_win_sock(void);
#endif

void sock_close(SOCKET sock); // standard function for socket creation
void show_soc_error();        // standard function for socket error messages

void create_address(peer *address, char *ip);                                         // create an IPv4 sockaddr_in
bool create_server(SOCKET *server, sockaddr_in address, int opt);                     // bind a server to the socket
peer *create_peers(char **peers_ip, size_t peers_size);                               // create peers
void append_peer(peer **peers, size_t *peers_size, peer new_peer);                    // append new peer
void show_peers(peer server, int *clock, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size);             // print the peers in list
char *build_message(sockaddr_in sender_ip, int clock, MSG_TYPE msg_type, void *args); // create a message with the sender ip, its clock and the message type
void send_message(char *msg, peer *neighbour, MSG_TYPE msg_type);                     // send a built message to an ONLINE known peer socket
// TODO: add args when needed
MSG_TYPE read_message(peer receiver, char *buf, int *clock, peer *sender); // read message, mark its sender and return the message type
int peer_in_list(peer a, peer *neighbours, size_t peers_size);             // check if peer is in list of known peers
void bye_peers(peer server, int *clock, peer *peers, size_t peers_size);   // send a bye message to every peer in list
#endif