#include <sock.h>
#include <file.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef WIN
int init_win_sock(void) {
    WSADATA d;
    int iResult = WSAStartup(MAKEWORD(2, 2), &d);
    if(iResult != 0) {
        fprintf(stderr, "Failed to initialize. %d\n", WSAGetLastError());
        return 1;
    }
    return iResult;
}
#endif

// standard function for socket creation
void sock_close(SOCKET sock) {
#ifdef WIN
    closesocket(sock);
    return;
#else
    close(sock);
#endif
}
// standard function for socket error messages
void show_soc_error() {
#ifdef _WIN32
    wchar_t *s = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&s, 0, NULL);
    fprintf(stderr, "%d: %S\n", WSAGetLastError(), s);
    LocalFree(s);
#else
    fprintf(stderr, "%d: %s\n", errno, strerror(errno));
#endif
}

// compare two peers
bool is_same_peer(peer a, peer b) {
    return (strcmp(inet_ntoa(a.con.sin_addr), inet_ntoa(b.con.sin_addr)) == 0 && (a.con.sin_port == b.con.sin_port));
}

// create an IPv4 sockaddr_in
void create_address(peer *address, const char *ip) {
    char *ip_cpy = strdup(ip);
    char *tokip = strtok(ip_cpy, ":");
    char *tokport = strtok(NULL, ":");
    address->con.sin_addr.s_addr = inet_addr(tokip);
    address->con.sin_family = AF_INET;
    address->con.sin_port = htons((unsigned short)(atoi(tokport))); // make string into int, then the int into short, then the short into a network byte order short
    address->status = OFFLINE;
}

// bind a server to the socket
bool create_server(SOCKET *server, sockaddr_in address, int opt) {
    *server = socket(AF_INET, SOCK_STREAM, 0);
    if(is_invalid_sock(*server)) {
        printf("Error upon socket creation.\n");
        return false;
    }
    if(setsockopt(*server, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) != 0) {
        printf("Error creating servern\n");
        show_soc_error();
        return false;
    }
    if(bind(*server, (const struct sockaddr *)&address, sizeof(address)) != 0) {
        printf("Error binding server\n");
        show_soc_error();
        return false;
    }
    return true;
}

// create peers
peer *create_peers(const char **peers_ip, size_t peers_size) {
    peer *peers = malloc(sizeof(peer) * peers_size);
    if(!peers) {
        perror("Failed to allocate memory");
        return NULL;
    }
    for(int i = 0; i < peers_size; i++) {
        printf("Adding new peer %s status OFFLINE\n", peers_ip[i]);
        create_address(&peers[i], peers_ip[i]);
    }
    return peers;
}

// append new peer
void append_peer(peer **peers, size_t *peers_size, peer new_peer) {
    peer *new_peers = realloc(*peers, (*peers_size + 1) * sizeof(peer));
    if(!new_peers) {
        perror("Failed to allocate memory");
        return;
    }
    *peers = new_peers;
    (*peers)[*peers_size] = new_peer;
    (*peers_size)++;
}

// print the peers in list
void show_peers(peer server, int *clock, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size) {
    printf("List peers:\n");
    printf("\t[0] Go back\n");

    for(int i = 0; i < peers_size; i++) {
        printf("\t[%d] %s:%u ", i + 1, inet_ntoa(peers[i].con.sin_addr), ntohs(peers[i].con.sin_port));
        if(peers[i].status == ONLINE)
            printf("ONLINE\n");
        else
            printf("OFFLINE\n");
    }

    int input;
    if(scanf("%d", &input) != 1) {
        while(getchar() != '\n');
        printf("Invalid input! Try again.\n");
        return;
    }
    else if(input == 0)
        return;
    else if(input > 0 && input <= peers_size) {
        pthread_mutex_lock(clock_lock);
        (*clock)++;
        pthread_mutex_unlock(clock_lock);
        printf("\t=> Atualizando relogio para %d\n", *clock);
        char *msg = build_message(server.con, *clock, HELLO, NULL);
        if(!msg) {
            printf("Error: Failed to build message!\n");
            return;
        }
        send_message(msg, &(peers[input - 1]), HELLO);
        free(msg);
    }
    else {
        printf("Invalid input! Try again.\n");
        return;
    }
}

// request the peers list of every known peer
void get_peers(peer server, int *clock, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size) {
    char *msg = build_message(server.con, *clock, GET_PEERS, NULL);
    if(!msg) {
        printf("Error: Failed to build message!\n");
        return;
    }
    for(int i = 0; i < peers_size; i++) {
        pthread_mutex_lock(clock_lock);
        (*clock)++;
        pthread_mutex_unlock(clock_lock);
        printf("\t=> Atualizando relogio para %d\n", *clock);
        send_message(msg, &peers[i], GET_PEERS);
    }
}

// share the peers list with who requested
void share_peers_list(peer server, int clock, SOCKET soc_sender, peer *sender, peer *peers, size_t peers_size) {
    peer_list_args args;
    args.sender = *sender;
    args.peers = peers;
    args.peers_size = peers_size;
    char *msg = build_message(server.con, clock, PEER_LIST, (void *)&args);
    size_t len = strlen(msg);
    printf("Encaminhando mensagem \"%.*s\" para %s:%d\n", (int)strcspn(msg, "\n"), msg, inet_ntoa(sender->con.sin_addr), ntohs(sender->con.sin_port));
    if(send(soc_sender, msg, len + 1, 0) == len + 1) // reaproveita soc para responder sem ter q abrir um novo
        ;
    else printf("Error answering peer\n");
}

// create a message with the sender ip, its clock and the message type
char *build_message(sockaddr_in sender_ip, int clock, MSG_TYPE msg_type, void *args) {
    char *ip = inet_ntoa(sender_ip.sin_addr);
    char *msg = malloc(sizeof(char) * MSG_SIZE);

    if(!msg) {
        perror("Failed to allocate memory");
        return NULL;
    }

    switch(msg_type) {
        /*
        MESSAGE FORMATING: <sender_ip>:<sender_port> <sender_clock> <MSG_TYPE> [<ARGS...>]\n <- important newline
        */
    case HELLO:
        sprintf(msg, "%s:%d %d HELLO\n", ip, (int)ntohs(sender_ip.sin_port), clock);
        break;
    case GET_PEERS:
        sprintf(msg, "%s:%d %d GET_PEERS\n", ip, (int)ntohs(sender_ip.sin_port), clock);
        break;
    case PEER_LIST:
        ;
        peer_list_args *list_args = (peer_list_args *)args;
        peer sender = list_args->sender;
        size_t peers_size = list_args->peers_size;
        peer *peers = list_args->peers;
        char *temp = realloc(msg, sizeof(char *) * alt_msg_size(peers_size));
        if(!temp) {
            printf("Failed to build message\n");
            return NULL;
        }
        msg = temp;
        int argumento_sem_nome_ainda = 0;
        sprintf(msg, "%s:%d %d PEER_LIST %d", ip, (int)ntohs(sender_ip.sin_port), clock, peers_size - 1); // TODO: analyse why do it return wrong with sender_ip
        for(int i = 0; i < peers_size; i++) {
            if(is_same_peer(sender, peers[i]))
                continue;
            sprintf(msg, "%s ", msg);
            if(peers[i].status == ONLINE) sprintf(msg, "%s%s:%d:ONLINE:%d", msg, inet_ntoa(peers[i].con.sin_addr), ntohs(peers[i].con.sin_port), argumento_sem_nome_ainda);
            else sprintf(msg, "%s%s:%d:OFFLINE:%d", msg, inet_ntoa(peers[i].con.sin_addr), ntohs(peers[i].con.sin_port), argumento_sem_nome_ainda);
        }
        sprintf(msg, "%s\n", msg);
        break;
    case BYE:
        sprintf(msg, "%s:%d %d BYE\n", ip, (int)ntohs(sender_ip.sin_port), clock);
        break;
    default:
        free(msg);
        return NULL;
    }
    return msg;
}

// send a built message to an known peer socket
void send_message(const char *msg, peer *neighbour, MSG_TYPE msg_type) {
    if(!msg) {
        printf("Failed to pass message\n");
        return;
    }
    SOCKET server_soc = socket(AF_INET, SOCK_STREAM, 0);
    if(is_invalid_sock(server_soc)) {
        printf("Error creating socket");
        return;
    }
    if(connect(server_soc, (const struct sockaddr *)&(neighbour->con), sizeof(neighbour->con)) != 0) {
        printf("Error connecting to peer\n");
        show_soc_error();
        return;
    }
    size_t len = strlen(msg);
    printf("Encaminhando mensagem \"%.*s\" para %s:%d\n", (int)strcspn(msg, "\n"), msg, inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port));
    if(send(server_soc, msg, len + 1, 0) == len + 1) {
        switch(msg_type) {
        case HELLO:
            if(neighbour->status == OFFLINE) {
                neighbour->status = ONLINE;
                printf("Atualizando peer %s:%d status ONLINE\n", inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port));
            }
            break;
        case GET_PEERS:
            if(neighbour->status == OFFLINE) {
                neighbour->status = ONLINE;
                printf("Atualizando peer %s:%d status ONLINE\n", inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port));
            }
            break;
        default:
            break;
        }
    }
    else {
        switch(msg_type) {
        case HELLO:
            if(neighbour->status == ONLINE) {
                neighbour->status = OFFLINE;
                printf("Atualizando peer %s:%d status OFFLINE\n", inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port));
            }
            break;
        case GET_PEERS:
            if(neighbour->status == ONLINE) {
                neighbour->status = OFFLINE;
                printf("Atualizando peer %s:%d status OFFLINE\n", inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port));
            }
            break;
        default:
            break;
        }
        show_soc_error();
    }
    sock_close(server_soc);
}

// read message, mark its sender and return the message type
MSG_TYPE read_message(peer receiver, const char *buf, int *clock, peer *sender) {
    char *buf_cpy = strdup(buf);
    char *tok_ip = strtok(buf_cpy, " ");
    int aclock = atoi(strtok(NULL, " "));
    char *tok_msg = strtok(NULL, " ");
    aclock = aclock; // FIXME: melhor jeito de tirar mensagem de variável inútil

    if(tok_msg) {
        tok_msg[strcspn(tok_msg, "\n")] = '\0';
    }

    create_address(sender, tok_ip);
    if(strcmp(tok_msg, "HELLO") == 0)
        return HELLO;
    if(strcmp(tok_msg, "GET_PEERS") == 0)
        return GET_PEERS;
    if(strcmp(tok_msg, "PEER_LIST") == 0)
        return PEER_LIST;
    if(strcmp(tok_msg, "BYE") == 0)
        return BYE;
    return UNEXPECTED_MSG_TYPE;
}

// check if peer is in list of known peers
int peer_in_list(peer a, peer *neighbours, size_t peers_size) {
    for(int i = 0; i < peers_size; i++) {
        if(is_same_peer(a, neighbours[i]))
            return i;
    }
    return -1;
}

// check if message received was read fully
char *check_msg_full(const char *buf, SOCKET sock, int *rec_peers_size, int *valread) {
    bool is_full_msg = false;
    if(*valread == 50) {
        for(int i = 0; i < *valread; i++) {
            if(buf[i] == '\n') {
                is_full_msg = true;
                break;
            }
        }
        if(!is_full_msg) {
            int spaces = 0;
            char *duplicate;
            for(int i = 0; i < *valread; i++) {
                if(buf[i] == ' ') spaces++;
                if(spaces == 3) {
                    duplicate = strdup(&buf[i]);
                }
                if(spaces == 4) {
                    duplicate[i] = '\0';
                    *rec_peers_size = atoi(duplicate);
                    int new_size = alt_msg_size(*rec_peers_size);
                    char *aux_buf;
                    aux_buf = malloc(sizeof(char) * new_size);
                    if(!aux_buf) return NULL;
                    sprintf(aux_buf, "%s", buf);
                    *valread += recv(sock, &aux_buf[*valread], new_size, 0);
                    return aux_buf;
                }
            }
        }
    }
    return NULL;
}

// append received list to known peer list
void append_list_peers(const char *buf, peer **peers, size_t *peers_size, size_t rec_peers_size) {
    char *cpy = strdup(buf);
    strtok(cpy, " "); //ip
    strtok(NULL, " ");//clock
    strtok(NULL, " ");//type
    strtok(NULL, " ");//size
    char *list = strtok(NULL, " \n");
    cpy = strdup(list);
    char *p = strtok(cpy, " ");
    int times = 0;
    peer *rec_peers_list = malloc(sizeof(peer) * rec_peers_size);
    for(int i = 0; i < rec_peers_size; i++) {
        char *infon = strtok(p, ":");
        for(int j = 0; j < 4; j++) { //hardcoding infos size :D => ip1:port2:status3:num4
            if(j == 0) rec_peers_list[i].con.sin_addr.s_addr = inet_addr(infon);
            if(j == 1) rec_peers_list[i].con.sin_port = htons((unsigned short)atoi(infon));
            if(j == 2) {
                if(strcmp(infon, "ONLINE") == 0) rec_peers_list[i].status = ONLINE;
                else rec_peers_list[i].status = OFFLINE;
            }
            if(j == 3); // FIXME: numero aleatorio jogado fora por enquanto;
            infon = strtok(NULL, ":");
        }
        times += 1;
        cpy = strdup(list);
        for(int j = 0; j <= times; j++) {
            if(j == 0) p = strtok(cpy, " ");
            else p = strtok(NULL, " ");
        }
    }
    //TODO: FIXME: APPEND UNKNOWN REC_PEERS_LIST TO PEERS;
    for(int i = 0; i < rec_peers_size; i++) {
        bool add = true;
        for(int j = 0; j < *peers_size; j++) {
            if(is_same_peer(rec_peers_list[i], (*peers)[j])) {
                add = false;
                break;
            }
        }
        if(add) {
            peer *temp = realloc(*peers, sizeof(peer) * (*peers_size + 1));
            if(!temp) free(rec_peers_list);
            *peers = temp;
            (*peers)[*peers_size] = rec_peers_list[i];
            *peers_size += 1;
        }
    }
}

// send a bye message to every peer in list
void bye_peers(peer server, int *clock, peer *peers, size_t peers_size) {
    for(int i = 0; i < peers_size; i++) {
        if(peers[i].status == ONLINE) {
            char *msg = build_message(server.con, *clock, BYE, NULL);
            send_message(msg, &peers[i], BYE);
        }
    }
}
