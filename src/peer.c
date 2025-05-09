#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sock.h>
#include <peer.h>

char *status_string[] = { "OFFLINE", "ONLINE" };
// TODO: array para enum de mensagens
// TODO: make peer func && print peer func

// compare two peers
bool is_same_peer(peer a, peer b) {
    return (strcmp(inet_ntoa(a.con.sin_addr), inet_ntoa(b.con.sin_addr)) == 0 && (a.con.sin_port == b.con.sin_port));
}

// create an IPv4 sockaddr_in
void create_address(peer *address, const char *ip, uint32_t clock) {
    char *ip_cpy = strdup(ip);
    char *tokip = strtok(ip_cpy, ":");
    char *tokport = strtok(NULL, ":");
    address->con.sin_addr.s_addr = inet_addr(tokip);
    address->con.sin_family = AF_INET;
    address->con.sin_port = htons((unsigned short)(atoi(tokport))); // make string into int, then the int into short, then the short into a network byte order short
    address->status = OFFLINE;
    address->p_clock = clock;
    free(ip_cpy);
}

// bind a server to the socket
bool create_server(SOCKET *server, sockaddr_in address, int opt) {
    *server = socket(AF_INET, SOCK_STREAM, 0);
    if(is_invalid_sock(*server)) {
        fprintf(stderr, "Erro: Falha na criacao do socket.\n");
        return false;
    }
    if(setsockopt(*server, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) != 0) {
        fprintf(stderr, "Erro: Falha ao criar servidor\n");
        show_soc_error();
        return false;
    }
    if(bind(*server, (const struct sockaddr *)&address, sizeof(address)) != 0) {
        fprintf(stderr, "Erro: Falha ao ligar servidor\n");
        show_soc_error();
        return false;
    }
    return true;
}

// create peers
peer *create_peers(const char **peers_ip, size_t peers_size) {
    peer *peers = malloc(sizeof(peer) * peers_size);
    if(!peers) {
        fprintf(stderr, "Erro: Falha na alocacao de peers!\n");
        return NULL;
    }
    for(int i = 0; i < peers_size; i++) {
        printf("Adicionando novo peer %s status %s\n", peers_ip[i], status_string[0]);
        create_address(&peers[i], peers_ip[i], 0);
    }
    return peers;
}

// append new peer
int append_peer(peer **peers, size_t *peers_size, peer new_peer, int *i, char *file) {
    peer *new_peers = realloc(*peers, (*peers_size + 1) * sizeof(peer));
    if(!new_peers) {
        fprintf(stderr, "Erro: Falha na alocacao de new_peers");
        return -1; // FIXME: tratar possivel erro
    }
    *peers = new_peers;
    (*peers)[*peers_size] = new_peer;
    (*peers_size)++;

    FILE *f = fopen(file, "a");
    (*i) = *peers_size - 1;
    fprintf(f, "%s:%d\n", inet_ntoa((*peers)[(*i)].con.sin_addr), ntohs((*peers)[(*i)].con.sin_port));
    fclose(f);
    printf("\tAdicionando novo peer %s:%d status %s\n", inet_ntoa((*peers)[(*i)].con.sin_addr), ntohs((*peers)[(*i)].con.sin_port), status_string[(*peers)[(*i)].status]);

    return 1;
}

// check if peer is in list of known peers
int peer_in_list(peer a, peer *neighbours, size_t peers_size) {
    for(int i = 0; i < peers_size; i++) {
        if(is_same_peer(a, neighbours[i]))
            return i;
    }
    return -1;
}

// append received list to known peer list
void append_list_peers(const char *buf, peer **peers, size_t *peers_size, size_t rec_peers_size, char *file) {
    char *cpy = strdup(buf);
    strtok(cpy, " "); //ip
    strtok(NULL, " ");//clock
    strtok(NULL, " ");//type
    strtok(NULL, " ");//size
    char *list = strtok(NULL, "\n");

    if(!list) {
        fprintf(stderr, "Erro: Lista de peers nao encontrada!\n");
        free(cpy);
        return;
    }

    char *p = NULL;
    peer *rec_peers_list = malloc(sizeof(peer) * rec_peers_size);

    if(!rec_peers_list) {
        fprintf(stderr, "Erro: Falha na alocacao de rec_peers_list");
        free(cpy);
        return;
    }
    size_t info_count = 0;
    for(int i = 0; i < rec_peers_size; i++, info_count++) {
        char *cpy_l = strdup(list);
        for(int j = 0; j <= info_count; j++) {
            if(j == 0) p = strtok(cpy_l, " ");
            else p = strtok(NULL, " ");
        }
        char *infon = strtok(p, ":");
        for(int j = 0; j < 4; j++) { //hardcoding infos size :D => ip1:port2:status3:num4
            if(j == 0) rec_peers_list[i].con.sin_addr.s_addr = inet_addr(infon);
            if(j == 1) rec_peers_list[i].con.sin_port = htons((unsigned short)atoi(infon));
            if(j == 2) {
                if(strcmp(infon, "ONLINE") == 0) rec_peers_list[i].status = ONLINE;
                else rec_peers_list[i].status = OFFLINE;
            }
            if(j == 3) rec_peers_list[i].p_clock = atoi(infon);
            infon = strtok(NULL, ":");
        }
        
        bool add = true;
        for(int j = 0; j < *peers_size; j++) {
            if(is_same_peer(rec_peers_list[i], (*peers)[j])) {
                add = false;
                break;
            }
        }
        if(add) {
            int j;
            rec_peers_list[i].con.sin_family = AF_INET;
            int res = append_peer(peers, peers_size, rec_peers_list[i], &j, file);
            if(res == -1) free(rec_peers_list);
        }
        free(cpy_l);
    }

    free(cpy);
    free(rec_peers_list);
}