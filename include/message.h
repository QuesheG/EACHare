#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdlib.h>
#include <pthread.h>
#include <sock.h>
#include <peer.h>

#define MSG_SIZE 60 // 255.255.255.255:655535 + int + type
#define alt_msg_size(x) (MSG_SIZE + (x * 40)) //MSG_SIZE + (x * sizeof(255.255.255.255:655535:status:int))

typedef enum msg_type {
    HELLO,
    GET_PEERS,
    PEER_LIST,
    BYE,

    UNEXPECTED_MSG_TYPE
} MSG_TYPE;

typedef struct peer_list_args {
    peer sender;
    peer *peers;
    size_t peers_size;
} peer_list_args;


void show_peers(peer server, int *clock, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size);  // print the peers in list
void get_peers(peer server, int *clock, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size);   // request the peers list of every known peer
void share_peers_list(peer server, int *clock, pthread_mutex_t *clock_lock, peer *sender, peer *peers, size_t peers_size);// share the peers list with who requested
char *build_message(sockaddr_in sender_ip, int clock, MSG_TYPE msg_type, void *args);                   // create a message with the sender ip, its clock and the message type
void send_message(const char *msg, peer *neighbour, MSG_TYPE msg_type);                                 // send a built message to an known peer socket
MSG_TYPE read_message(const char *buf, int *clock, peer *sender);                                       // read message, mark its sender and return the message type
char *check_msg_full(const char *buf, SOCKET sock, int *rec_peers_size, int *valread);                  // check if message received was read fully
void append_list_peers(const char *buf, peer **peers, size_t *peers_size, size_t rec_peers_size);       // append received list to known peer list
void bye_peers(peer server, int *clock, peer *peers, size_t peers_size);                                // send a bye message to every peer in list

#endif