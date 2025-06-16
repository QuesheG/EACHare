#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <stdlib.h>
#include <pthread.h>
#include <sock.h>
#include <peer.h>
#include <list.h>

// sizeof(int) => 20
#define MSG_SIZE 60                                   // 255.255.255.255:655535 + int + type
#define msg_size_peer_list(x) (MSG_SIZE + (x * 50))   // MSG_SIZE + (x * sizeof(255.255.255.255:655535:status:int))
#define msg_size_files_list(x) (MSG_SIZE + (x * 300)) // MSG_SIZE + (x * sizeof(name:int)) (name => 255(linux) int 20 )

typedef enum _msg_type
{
    UNEXPECTED_MSG_TYPE,

    HELLO,
    GET_PEERS,
    PEER_LIST,
    LS,
    LS_LIST,
    DL,
    FILEMSG, // msg_type = FILE type (stdio)
    BYE
} MSG_TYPE;

typedef struct _peer_list_args
{
    peer sender;
    peer *peers;
    size_t peers_size;
} peer_list_args;

typedef struct _files
{
    char *fname;
    size_t fsize;
} file_desc;

typedef struct _ls_files
{
    file_desc file;
    int peers_size;
    ArrayList *holders;
} ls_files;

typedef struct _lslist_msg_args
{
    char **list;
    size_t *list_file_len;
    size_t list_len;
} lslist_msg_args;

typedef struct _dl_msg_args
{
    char *fname;
    int chunk_size;
    int offset;
} dl_msg_args;

typedef struct _file_msg_args
{
    char *file_name;
    char *contentb64;
    size_t file_size;
    int chunk_size;
    int offset;
} file_msg_args;

typedef struct _downloader_workers
{
    peer *server;
    pthread_mutex_t *lock;
    uint8_t thread_l_size;
    uint8_t thread_id;
    ls_files file;
    const char *dir_path;
    int chunk_size;
    ArrayList *statistics;
} work_download;

typedef struct _stat
{
    int chunk_used;
    int n_peers;
    uint64_t fsize;
    int n;
    ArrayList *times;
    double avrg;
    double std_dev;
} stat_block;

void show_peers(peer *server, pthread_mutex_t *clock_lock, ArrayList *peers); // print the peers in list
void get_peers(peer *server, pthread_mutex_t *clock_lock, ArrayList *peers /*, char *file*/); // request the peers list of every known peer
void share_peers_list(peer *server, pthread_mutex_t *clock_lock, SOCKET con, peer sender, ArrayList *peers); // share the peers list with who requested
void get_files(peer *server, pthread_mutex_t *clock_lock, ArrayList *peers, const char *dir_path, ArrayList *files_list, int size_chunk, ArrayList *statistics); // asks for files of all peers
void share_files_list(peer *server, pthread_mutex_t *clock_lock, SOCKET con, peer sender, char *dir_path); // send list of files
int send_complete(SOCKET sock, const void *buf, size_t len, int flag); // send full message
void send_file(peer *server, pthread_mutex_t *clock_lock, char *buf, SOCKET con, peer sender, char *dir_path); // send file
char *build_message(sockaddr_in sender_ip, int clock, MSG_TYPE msg_type, void *args); // create a message with the sender ip, its clock and the message type
SOCKET send_message(const char *msg, peer *neighbour, MSG_TYPE msg_type); // send a built message to an known peer socket
MSG_TYPE read_message(const char *buf, peer *sender); // read message, mark its sender and return the message type
char *check_msg_full(const char *buf, SOCKET sock, MSG_TYPE msg_type, void *args, ssize_t *valread); // check if message received was read fully
ArrayList *receive_files(peer *server, pthread_mutex_t *clock_lock, ArrayList *peers); // receive messages for files
void print_files_received(ArrayList *files); // print files received
void download_file(peer *server, pthread_mutex_t *clock_lock, ls_files chosen_file, const char *dir_path, size_t size_chunk, ArrayList *statistics); // download chosen file
void append_files_list(const char *buf, ArrayList *list, peer sender, size_t rec_files_len); // append list received to known list
char *get_file_in_msg(char *buf, int *clock, char **fname, int *chunk_size, int *offset); // return the file in base64 format
void print_statistics(ArrayList *statistics); // print statistics gathered
void change_chunk_size(int *chunk_size); // change the local chunk_size
void bye_peers(peer *server, pthread_mutex_t *clock_lock, ArrayList *peers); // send a bye message to every peer in list

#endif
