#ifndef _PEER_H
#define _PEER_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sock.h>
#include <list.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))


typedef enum status {
    OFFLINE,
    ONLINE
} STATUS;

extern char *status_string[];

typedef struct sockaddr_in sockaddr_in;

typedef struct peer {
    sockaddr_in con;
    STATUS status;
    uint64_t p_clock;
} peer;


bool is_same_peer(peer a, peer b); // compare two peers
void create_address(peer *address, const char *ip, uint64_t clock); // create an IPv4 sockaddr_in
bool create_server(SOCKET *server, sockaddr_in address, int opt); // bind a server to the socket
void create_peers(ArrayList *peers, ArrayList *peers_txt); // create peers
int append_peer(ArrayList *peers, peer new_peer, int *i/*, char *file*/); // append new peer
int peer_in_list(peer a, peer *neighbours, size_t peers_size); // check if peer is in list of known peers
void append_list_peers(const char *buf, ArrayList *peers, size_t rec_peers_size/*, char *file*/); // append received list to known peer list
void update_clock(peer *a, pthread_mutex_t *clock_lock, uint64_t n_clock); // update clock 

#endif
