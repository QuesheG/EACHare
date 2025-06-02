#include <stdlib.h>
#include <threading.h>

void mssleep(uint64_t ms) {
    #ifdef _WIN32
    Sleep(ms);
    #else
    struct timespec a = {0, ms*1000000};
    nanosleep(&a, NULL);
    #endif
}

listen_args *send_args(peer *server, peer **neighbours, size_t *peers_size, char *file, SOCKET rec_sock, char *dir_path) {
    listen_args * l_args = malloc(sizeof(listen_args));
    if(!l_args) return NULL;
    l_args->server = server;
    l_args->neighbours = neighbours;
    l_args->peers_size = peers_size;
    l_args->file = file;
    l_args->rec_sock = rec_sock;
    l_args->dir_path = dir_path;
    return l_args;
}

void get_args(listen_args *l_args, peer **server, peer ***neighbours, size_t **peers_size, char **file, SOCKET *rec_sock, char **dir_path) {
    *server = l_args->server;
    *neighbours = l_args->neighbours;
    *peers_size = l_args->peers_size;
    *file = l_args->file;
    *rec_sock = l_args->rec_sock;
    *dir_path = l_args->dir_path;
}