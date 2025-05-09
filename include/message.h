#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <stdlib.h>
#include <pthread.h>
#include <sock.h>
#include <peer.h>

//sizeof(int) => 20
#define MSG_SIZE 60 // 255.255.255.255:655535 + int + type
#define msg_size_peer_list(x) (MSG_SIZE + (x * 50)) //MSG_SIZE + (x * sizeof(255.255.255.255:655535:status:int))
#define msg_size_files_list(x) (MSG_SIZE + (x * 300)) //MSG_SIZE + (x * sizeof(name:int)) (name => 255(linux) int 20 )

typedef enum _msg_type {
    UNEXPECTED_MSG_TYPE,

    HELLO,
    GET_PEERS,
    PEER_LIST,
    LS,
    LS_LIST,
    DL,
    FILEMSG, //msg_type = FILE type (stdio)😢
    BYE
} MSG_TYPE;

typedef struct _peer_list_args {
    peer sender;
    peer *peers;
    size_t peers_size;
} peer_list_args;

typedef struct _files {
    char *fname;
    size_t fsize;
} file_desc;

typedef struct _ls_files {
    file_desc file;
    peer holder;
} ls_files;

typedef struct _lslist_msg_args {
    char **list;
    size_t *list_file_len;
    size_t list_len;
} lslist_msg_args;

typedef struct _dl_msg_args {
    char *fname;
    int a; //FIXME: Fix me when p3 drop
    int b; //FIXME: Fix me when p3 drop
} dl_msg_args;

typedef struct _file_msg_args {
    char *file_name;
    char *contentb64;
    size_t file_size;
    int a; //FIXME: Fix me when p3 drop
    int b; //FIXME: Fix me when p3 drop
} file_msg_args;


void show_peers(peer *server, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size); // print the peers in list
void get_peers(peer *server, pthread_mutex_t *clock_lock, peer **peers, size_t *peers_size, char *file); // request the peers list of every known peer
void share_peers_list(peer *server, pthread_mutex_t *clock_lock, SOCKET con, peer *sender, peer *peers, size_t peers_size); // share the peers list with who requested
void get_files(peer *server, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size, char *dir_path, char **files_list, size_t *files_len); // asks for files of all peers 
void share_files_list(peer *server, pthread_mutex_t *clock_lock, SOCKET con, peer *sender, char *dir_path); //send list of files 
void send_file(peer *server, pthread_mutex_t *clock_lock, char *buf, SOCKET con, char *dir_path); //send file
char *build_message(sockaddr_in sender_ip, int clock, MSG_TYPE msg_type, void *args); // create a message with the sender ip, its clock and the message type
SOCKET send_message(const char *msg, peer *neighbour, MSG_TYPE msg_type); // send a built message to an known peer socket
MSG_TYPE read_message(const char *buf, peer *sender); // read message, mark its sender and return the message type
char *check_msg_full(const char *buf, SOCKET sock, MSG_TYPE msg_type, void *args, ssize_t *valread); // check if message received was read fully
void append_files_list(const char *buf, ls_files *list, size_t list_len, peer sender, size_t rec_files_len); //append list received to known list
char *get_file_in_msg(const char *buf, char **fname, int *a, int *b); //return the file in base64 format
void bye_peers(peer server, peer *peers, size_t peers_size); // send a bye message to every peer in list

#endif