#ifndef THREADING_H
#define THREADING_H
#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#endif

#include <stdint.h>
#include <dirent.h>
#include <pthread.h>
#include <peer.h>
#include <sock.h>

typedef struct listen_args
{
    peer server;
    peer **neighbours;
    size_t *peers_size;
    int *clock;
    char *file;
    DIR *dir;
    int rec_sock;
} listen_args;

void mssleep(uint64_t ms);
listen_args *send_args(peer server, peer **neighbours, size_t *peers_size, int *clock, char *file, DIR *dir, SOCKET rec_sock);
void get_args(listen_args *l_args, peer *server, peer ***neighbours, size_t **peers_size, int **clock, char **file, DIR **dir, SOCKET *rec_sock);

#endif