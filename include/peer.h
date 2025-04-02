#ifndef PEER_H
#define PEER_H

#include <stdbool.h>
#include <stdlib.h>
#include <sock.h>

typedef enum status {
    OFFLINE,
    ONLINE
} STATUS;

typedef struct sockaddr_in sockaddr_in;

typedef struct peer {
    sockaddr_in con;
    STATUS status;
} peer;


bool is_same_peer(peer a, peer b);                                  // compare two peers
void create_address(peer *address, const char *ip);                 // create an IPv4 sockaddr_in
bool create_server(SOCKET *server, sockaddr_in address, int opt);   // bind a server to the socket
peer *create_peers(const char **peers_ip, size_t peers_size);       // create peers
void append_peer(peer **peers, size_t *peers_size, peer new_peer, int *i, char *file);  // append new peer
int peer_in_list(peer a, peer *neighbours, size_t peers_size);      // check if peer is in list of known peers

#endif