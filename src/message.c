#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <file.h>
#include <sock.h>
#include <message.h>
#include <peer.h>
#include <threading.h>

// print the peers in list
void show_peers(peer server, int *clock, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size) {
    printf("Lista de peers:\n");
    printf("\t[0] voltar para o menu anterior\n");

    for(int i = 0; i < peers_size; i++) {
        printf("\t[%d] %s:%u ", i + 1, inet_ntoa(peers[i].con.sin_addr), ntohs(peers[i].con.sin_port));
        if(peers[i].status == ONLINE)
            printf("%s\n", status_string[1]);
        else
            printf("%s\n", status_string[0]);
    }

    int input;
    if(scanf("%d", &input) != 1) {
        while(getchar() != '\n');
        fprintf(stderr, "Erro: Entrada invalida! Tente de novo.\n");
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
            fprintf(stderr, "Erro: Falha na construcao da mensagem!\n");
            return;
        }
        send_message(msg, &(peers[input - 1]), HELLO);
        free(msg);
    }
    else {
        fprintf(stderr, "Erro: Entrada invalida! Tente de novo.\n");
        return;
    }
}

// request the peers list of every known peer
void get_peers(peer server, int *clock, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size) {
    char *msg = build_message(server.con, *clock, GET_PEERS, NULL);
    if(!msg) {
        perror("Erro: Falha na construcao da mensagem!\n");
        return;
    }
    for(int i = 0; i < peers_size; i++) {
        pthread_mutex_lock(clock_lock);
        (*clock)++;
        pthread_mutex_unlock(clock_lock);
        printf("\t=> Atualizando relogio para %d\n", *clock);
        send_message(msg, &peers[i], GET_PEERS);
        mssleep(500);
    }
}

// share the peers list with who requested
void share_peers_list(peer server, int *clock, pthread_mutex_t *clock_lock, peer *sender, peer *peers, size_t peers_size) {
    peer_list_args args;
    args.sender = *sender;
    args.peers = peers;
    args.peers_size = peers_size;
    char *msg = build_message(server.con, *clock, PEER_LIST, (void *)&args);
    pthread_mutex_lock(clock_lock);
    (*clock)++;
    pthread_mutex_unlock(clock_lock);
    printf("\t=> Atualizando relogio para %d\n", *clock);
    send_message(msg, sender, PEER_LIST);
}

// create a message with the sender ip, its clock and the message type
char *build_message(sockaddr_in sender_ip, int clock, MSG_TYPE msg_type, void *args) {
    char *ip = inet_ntoa(sender_ip.sin_addr);
    char *msg = malloc(sizeof(char) * MSG_SIZE);

    if(!msg) {
        fprintf(stderr, "Erro: Falha na alocacao de memoria\n");
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
            fprintf(stderr, "Erro: Falha na alocacao da mensagem!\n");
            return NULL;
        }
        msg = temp;
        int argumento_sem_nome_ainda = 0;
        sprintf(msg, "%s:%d %d PEER_LIST %ld", ip, (int)ntohs(sender_ip.sin_port), clock, peers_size - 1);
        for(int i = 0; i < peers_size; i++) {
            if(is_same_peer(sender, peers[i]))
                continue;
            sprintf(msg, "%s ", msg);
            if(peers[i].status == ONLINE) sprintf(msg, "%s%s:%d:%s:%d", msg, inet_ntoa(peers[i].con.sin_addr), ntohs(peers[i].con.sin_port), status_string[1], argumento_sem_nome_ainda);
            else sprintf(msg, "%s%s:%d:%s:%d", msg, inet_ntoa(peers[i].con.sin_addr), ntohs(peers[i].con.sin_port), status_string[0], argumento_sem_nome_ainda);
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
        fprintf(stderr, "\nErro: Mensagem de envio vazia!\n");
        return;
    }
    SOCKET server_soc = socket(AF_INET, SOCK_STREAM, 0);
    if(is_invalid_sock(server_soc)) {
        fprintf(stderr, "\nErro: Falha na criacao do socket");
        show_soc_error();
        return;
    }
    if(connect(server_soc, (const struct sockaddr *)&(neighbour->con), sizeof(neighbour->con)) != 0) {
        fprintf(stderr, "\nErro: Falha ao conectar ao peer\n");
        show_soc_error();
        return;
    }
    size_t len = strlen(msg);
    printf("\tEncaminhando mensagem \"%.*s\" para %s:%d\n", (int)strcspn(msg, "\n"), msg, inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port));
    if(send(server_soc, msg, len + 1, 0) == len + 1) {
        switch(msg_type) {
        case HELLO:
            if(neighbour->status == OFFLINE) {
                neighbour->status = ONLINE;
                printf("\tAtualizando peer %s:%d status %s\n", inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port), status_string[1]);
            }
            break;
        case GET_PEERS:
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
                printf("\tAtualizando peer %s:%d status %s\n", inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port), status_string[0]);
            }
            break;
        case GET_PEERS:
            if(neighbour->status == ONLINE) {
                neighbour->status = OFFLINE;
                printf("\tAtualizando peer %s:%d status %s\n", inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port), status_string[0]);
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
MSG_TYPE read_message(const char *buf, int *clock, peer *sender) {
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

// check if message received was read fully
char *check_msg_full(const char *buf, SOCKET sock, int *rec_peers_size, ssize_t *valread) {
    bool is_full_msg = false;
    if(*valread == MSG_SIZE - 1) {
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
                if(buf[i] == ' ') {
                    spaces++;
                    if(spaces == 3) {
                        duplicate = strdup(&buf[i + 1]);
                    }
                    if(spaces == 4 && duplicate) {
                        duplicate[strcspn(duplicate, " ")] = '\0';
                        *rec_peers_size = atoi(duplicate);
                        free(duplicate);
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
    }
    return NULL;
}

// append received list to known peer list
void append_list_peers(const char *buf, peer **peers, size_t *peers_size, size_t rec_peers_size, char *file) {
    char *cpy = strdup(buf);
    strtok(cpy, " "); //ip
    strtok(NULL, " ");//clock
    strtok(NULL, " ");//type
    strtok(NULL, " ");//size
    char *list = strtok(NULL, " \n");

    if(!list) {
        fprintf(stderr, "Erro: Lista de peers nao encontrada!\n");
        free(cpy);
        return;
    }

    char *p = NULL;
    peer *rec_peers_list = malloc(sizeof(peer) * rec_peers_size);

    if(!rec_peers_list) {
        fprintf(stderr, "Erro: Falha na alocacao de memoria");
        free(cpy);
        free(p);
        return;
    }

    for(int i = 0; i < rec_peers_size; i++) {
        int space_index = strcspn(list, " ");
        p = strdup(list);
        char *peer_entry = strtok(p, " ");
        if(!peer_entry) {
            break;
        }
        list = strdup(&list[space_index + 1]);

        char *infon = strtok(peer_entry, ":");
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

        free(peer_entry);
    }

    free(cpy);
    free(list);
    if(p) free(p);
    free(rec_peers_list);
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