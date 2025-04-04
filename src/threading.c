#include <stdlib.h>
#include <threading.h>

void mssleep(uint64_t ms) {
    #ifdef _WIN32
    Sleep(ms);
    #else
    struct timespec a = {0, msg*1000000};
    nanosleep(&a, NULL);
    #endif
}

listen_args *send_args(peer server, peer **neighbours, size_t *peers_size, int *clock, char *file, DIR *dir, SOCKET rec_sock) {
    listen_args * l_args = malloc(sizeof(listen_args));
    if(!l_args) return NULL;
    l_args->server = server;
    l_args->neighbours = neighbours;
    l_args->peers_size = peers_size;
    l_args->clock = clock;
    l_args->file = file;
    l_args->dir = dir;
    l_args->rec_sock = rec_sock;
    return l_args;
}

void get_args(listen_args *l_args, peer *server, peer ***neighbours, size_t **peers_size, int **clock, char **file, DIR **dir, SOCKET *rec_sock) {
    *server = l_args->server;
    *neighbours = l_args->neighbours;
    *peers_size = l_args->peers_size;
    *clock = l_args->clock;
    *file = l_args->file;
    *dir = l_args->dir;
    *rec_sock = l_args->rec_sock;
}