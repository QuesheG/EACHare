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

#define MSG_SIZE 60 // 255.255.255.255:655535 + int + type
#define alt_msg_size(x) (MSG_SIZE + (x * 40)) //MSG_SIZE + (x * sizeof(255.255.255.255:655535:status:int))

typedef enum status {
    OFFLINE,
    ONLINE
} STATUS;

typedef enum msg_type {
    HELLO,
    GET_PEERS,
    PEER_LIST,
    BYE,

    UNEXPECTED_MSG_TYPE
} MSG_TYPE;

typedef struct sockaddr_in sockaddr_in;

typedef struct peer {
    sockaddr_in con;
    STATUS status;
} peer;

typedef struct peer_list_args {
    peer sender;
    peer *peers;
    size_t peers_size;
} peer_list_args;

#ifdef WIN
int init_win_sock(void);
#endif

void sock_close(SOCKET sock); // standard function for socket creation
void show_soc_error();        // standard function for socket error messages

void create_address(peer *address, const char *ip);                                                             // create an IPv4 sockaddr_in
bool create_server(SOCKET *server, sockaddr_in address, int opt);                                               // bind a server to the socket
peer *create_peers(char **peers_ip, size_t peers_size);                                                         // create peers
void append_peer(peer **peers, size_t *peers_size, peer new_peer);                                              // append new peer
void show_peers(peer server, int *clock, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size);          // print the peers in list
void get_peers(peer server, int *clock, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size);           // request the peers list of every known peer
void share_peers_list(peer server, int clock, SOCKET soc_sender, peer *sender, peer *peers, size_t peers_size); // share the peers list with who requested
char *build_message(sockaddr_in sender_ip, int clock, MSG_TYPE msg_type, void *args);                           // create a message with the sender ip, its clock and the message type
void send_message(char *msg, peer *neighbour, MSG_TYPE msg_type);                                               // send a built message to an known peer socket
MSG_TYPE read_message(peer receiver, const char *buf, int *clock, peer *sender);                                // read message, mark its sender and return the message type
int peer_in_list(peer a, peer *neighbours, size_t peers_size);                                                  // check if peer is in list of known peers
char *check_msg_full(const char *buf, SOCKET sock, int *rec_peers_size, int *valread);                          // check if message received was read fully
void append_list_peers(char *buf, peer **peers, size_t *peers_size, peer *rec_peers, size_t rec_peers_size);    // append received list to known peer list
void bye_peers(peer server, int *clock, peer *peers, size_t peers_size);                                        // send a bye message to every peer in list
#endif