#ifndef _THREADING_H
#define _THREADING_H
#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#endif

#include <stdint.h>
#include <dirent.h>
#include <pthread.h>
#include <list.h>
#include <peer.h>
#include <sock.h>

typedef struct listen_args
{
    peer *server;
    ArrayList *neighbours;
    // char *file;
    int rec_sock;
    char *dir_path;
} listen_args;

void mssleep(uint64_t ms);
listen_args *send_args(peer *server, ArrayList *neighbours, /*char *file, */SOCKET rec_sock, char *dir_path);
void get_args(listen_args *l_args, peer **server, ArrayList **neighbours, /*char **file,*/ SOCKET *rec_sock, char **dir_path);

#endif