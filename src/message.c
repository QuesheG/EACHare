#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <file.h>
#include <sock.h>
#include <peer.h>
#include <message.h>
#include <threading.h>
#include <base64.h>

char *message_string[] = { "HELLO", "GET_PEERS", "PEER_LIST", "LS", "LS_LIST", "DL", "FILE", "BYE"};

// print the peers in list
void show_peers(peer *server, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size) {
    printf("Lista de peers:\n");
    printf("\t[0] voltar para o menu anterior\n");

    for(int i = 0; i < peers_size; i++) {
        printf("\t[%d] %s:%u ", i + 1, inet_ntoa(peers[i].con.sin_addr), ntohs(peers[i].con.sin_port));
        if(peers[i].status == ONLINE)
            printf("%s\n", status_string[1]);
        else
            printf("%s\n", status_string[0]);
    }

    printf(">");

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
        server->p_clock++;
        pthread_mutex_unlock(clock_lock);
        printf("\t=> Atualizando relogio para %d\n", server->p_clock);
        char *msg = build_message(server->con, server->p_clock, HELLO, NULL);
        if(!msg) {
            fprintf(stderr, "Erro: Falha na construcao da mensagem!\n");
            return;
        }
        SOCKET s = send_message(msg, &(peers[input - 1]), HELLO);
        sock_close(s);
        free(msg);
    }
    else {
        fprintf(stderr, "Erro: Entrada invalida! Tente de novo.\n");
        return;
    }
}


// request the peers list of every known peer
void get_peers(peer *server, pthread_mutex_t *clock_lock, peer **peers, size_t *peers_size, char *file) {
    size_t loop_len = *peers_size;
    for(int i = 0; i < loop_len; i++) {
        pthread_mutex_lock(clock_lock);
        server->p_clock++;
        pthread_mutex_unlock(clock_lock);
        printf("\t=> Atualizando relogio para %d\n", server->p_clock);
        char *msg = build_message(server->con, server->p_clock, GET_PEERS, NULL);
        if(!msg) {
            perror("Erro: Falha na construcao da mensagem!\n");
            return;
        }
        SOCKET req = send_message(msg, &((*peers)[i]), GET_PEERS);
        if(is_invalid_sock(req)) {
            free(msg);
            continue;
        }
        char *buf = malloc(sizeof(char) * MSG_SIZE);
        ssize_t valread = recv(req, buf, MSG_SIZE - 1, 0);
        if(valread <= 0) {
            fprintf(stderr, "\nErro: Falha lendo mensagem\n");
            free(buf);
            free(msg);
            continue;
        }
        peer sender;
        MSG_TYPE msg_type = read_message(buf, &sender);

        int *rec_peers_size = malloc(sizeof(int));
        char *temp = check_msg_full(buf, req, msg_type, (void *)rec_peers_size, &valread);
        if(temp) {
            free(buf);
            buf = temp;
        }

        buf[valread] = '\0';

        printf("\n");
        printf("\tResposta recebida: \"%.*s\"\n", (int)strcspn(buf, "\n"), buf);
        append_list_peers(buf, peers, peers_size, *rec_peers_size, file);
        free(rec_peers_size);
        sock_close(req);
        free(msg);
    }
}


// share the peers list with who requested
void share_peers_list(peer *server, pthread_mutex_t *clock_lock, SOCKET soc, peer *sender, peer *peers, size_t peers_size) {
    peer_list_args args;
    args.sender = *sender;
    args.peers = peers;
    args.peers_size = peers_size;
    char *msg = build_message(server->con, server->p_clock, PEER_LIST, (void *)&args);
    pthread_mutex_lock(clock_lock);
    server->p_clock++;
    pthread_mutex_unlock(clock_lock);
    printf("\t=> Atualizando relogio para %d\n", server->p_clock);
    printf("\tEncaminhando mensagem \"%.*s\" para %s:%d\n", (int)strcspn(msg, "\n"), msg, inet_ntoa(sender->con.sin_addr), ntohs(sender->con.sin_port));
    send(soc, msg, strlen(msg) + 1, 0);
    sock_close(soc);
    free(msg);
}


// asks for files of all peers 
void get_files(peer *server, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size, char *dir_path, char **files_list, size_t *files_len) {
    //FIXME: diminuir função?
    ls_files *files;
    size_t files_list_len = 0;;
    for(int i = 0; i < peers_size; i++) {
        if(peers[i].status == OFFLINE) continue;
        pthread_mutex_lock(clock_lock);
        server->p_clock++;
        pthread_mutex_unlock(clock_lock);
        printf("\t=> Atualizando relogio para %d\n", server->p_clock);
        char *msg = build_message(server->con, server->p_clock, LS, NULL);
        if(!msg) {
            perror("Erro: Falha na construcao da mensagem!\n");
            return;
        }
        SOCKET req = send_message(msg, &(peers[i]), LS);
        if(is_invalid_sock(req)) {
            free(msg);
            continue;
        }
        char *buf = malloc(sizeof(char) * MSG_SIZE);
        ssize_t valread = recv(req, buf, MSG_SIZE - 1, 0);
        if(valread <= 0) {
            fprintf(stderr, "\nErro: Falha lendo mensagem\n");
            free(buf);
            free(msg);
            continue;
        }
        peer sender;
        MSG_TYPE msg_type = read_message(buf, &sender);

        size_t *rec_files_size = malloc(sizeof(size_t));
        char *temp = check_msg_full(buf, req, msg_type, (void *)rec_files_size, &valread);
        if(temp) {
            free(buf);
            buf = temp;
        }

        buf[valread] = '\0';

        ls_files *temp = realloc(files, sizeof(ls_files) * (files_list_len + *rec_files_size));
        if(!temp) {
            free(temp);
            continue;
        }
        files = temp;
        files_list_len += *rec_files_size;

        printf("\n");
        printf("\tResposta recebida: \"%.*s\"\n", (int)strcspn(buf, "\n"), buf);
        append_files_list(buf, files, files_list_len, sender, *rec_files_size);
        free(rec_files_size);
        sock_close(req);
        free(msg);
    }
    uint16_t max_fname_size = 10;
    uint16_t max_fsize_size = 7;
    uint16_t max_peer_size = 14;
    for(int i = 0; i < files_list_len; i++) {
        uint16_t s_f = strlen(files[i].file.fname);
        uint16_t s_s = strlen(files[i].file.fname);
        uint16_t s_p = strlen(inet_ntoa(files[i].holder.con.sin_addr)) + floor(log10((double)ntohs(files[i].holder.con.sin_port))) + 2;
        if(s_f > max_fname_size) max_fname_size = s_f;
        if(s_s > max_fname_size) max_fsize_size = s_s;
        if(s_p > max_fname_size) max_peer_size = s_p;
    }
    uint16_t count_size = floor(log10(files_list_len)) + 1;
    //print header
    printf("[%*.s] ", count_size, "");
    printf("%-*s ", max_fname_size, "Name");
    printf("|");
    printf(" %-*s ", max_fsize_size, "Tamanho");
    printf("|");
    printf(" %s\n", max_peer_size, "Peer");
    //print cancel
    printf("[%0*d] ", count_size, 0);
    printf("%-*s ", max_fname_size, "<Cancelar>");
    printf("|");
    printf(" %*.s ", max_fsize_size, "");
    printf("|");
    printf("\n");
    //print list
    for(int i = 0; i < files_list_len; i++) {
        printf("[%0*d] ", count_size, i + 1);
        printf("%-*s", max_fname_size, files[i].file.fname);
        printf("%-*d", max_fname_size, files[i].file.fsize);
        printf("%s\n", max_peer_size, files[i].holder);
    }
    printf("Digite o numero do arquivo para fazer o download:\n");
    int f_to_down = 0;
    printf(">");
    if(scanf("%d", &f_to_down) != 1) {
        while(getchar() != '\n');
        printf("Comando inesperado\n");
        f_to_down = 0;
    }
    if(!f_to_down) return;
    if(f_to_down > files_list_len) return;
    dl_msg_args *args;
    args->fname = files[f_to_down - 1].file.fname;
    args->a = 0;
    args->b = 0;
    char *msg = build_message(server->con, server->p_clock, DL, (void *)args);
    SOCKET req = send_message(msg, &(files[f_to_down].holder), DL);
    if(is_invalid_sock(req)) {
        fprintf(stderr, "\nErro: Falha com socket\n");
        free(msg);
        return;
    }
    size_t temp_size = MSG_SIZE + 26 + base64encode_len(files[f_to_down].file.fsize);
    char *buf = malloc(sizeof(char) * temp_size);
    ssize_t valread = recv(req, buf, temp_size - 1, 0);
    if(valread <= 0) {
        fprintf(stderr, "\nErro: Falha lendo mensagem\n");
        free(buf);
        free(msg);
        return;
    }
    buf[valread] = '\0';

    printf("\n");
    printf("\tResposta recebida: \"%.*s\"\n", (int)strcspn(buf, "\n"), buf);
    
    int a, b;
    char *file_b64 = get_file_in_msg(buf, &a, &b);
    char *file_decoded = malloc(sizeof(char) * files[f_to_down].file.fsize);
    base64_decode(file_decoded, file_b64);
    char *file_path = dir_file_path(dir_path, files[f_to_down].file.fname);
    FILE *new_file = fopen(file_path, "w");
    fwrite(file_decoded, files[f_to_down].file.fsize, 1, new_file);
    char **temp = realloc(files_list, sizeof(char *) * (*files_len + 1));
    if(!temp) {
        fprintf(stderr, "Erro: Falha na alocacao");
        free(file_b64);
        free(file_decoded);
        free(file_path);
        free(msg);
        fclose(new_file);
        free(buf);
        sock_close(req);
        free(temp);
        return;
    }
    files_list = temp;
    files_list[*files_len] = strdup(files[f_to_down].file.fname);
    *files_len++;
    free(file_b64);
    free(file_decoded);
    free(file_path);
    free(msg);
    fclose(new_file);
    free(buf);
    sock_close(req);
}


//send list of files
void share_files_list(peer *server, pthread_mutex_t *clock_lock, SOCKET con, char *dir_path) {
    //TODO: FINISH ME PLEASE
}


//send file
void send_file(peer *server, pthread_mutex_t *clock_lock, SOCKET con, char *dir_path, dl_msg_args *arg) {
    //TODO: FINISH ME PLEASE
}


// create a message with the sender ip, its clock and the message type
char *build_message(sockaddr_in sender_ip, int clock, MSG_TYPE msg_type, void *args) {
    char *ip = inet_ntoa(sender_ip.sin_addr);
    u_long port = ntohl(sender_ip.sin_port);
    char *msg = malloc(sizeof(char) * MSG_SIZE);

    if(!msg) {
        fprintf(stderr, "Erro: Falha na alocacao de memoria\n");
        return NULL;
    }
    if(msg_type < 0 || msg_type > BYE) {
        free(msg);
        msg = NULL;
        return msg;
    }
    switch(msg_type) {
        /*
        MESSAGE FORMATING: <sender_ip>:<sender_port> <sender_clock> <MSG_TYPE> [<ARGS...>]\n <- important newline
        */
    case PEER_LIST:
        ;
        peer_list_args *list_args = (peer_list_args *)args;
        peer sender = list_args->sender;
        size_t peers_size = list_args->peers_size;
        peer *peers = list_args->peers;
        char *temp = realloc(msg, sizeof(char *) * msg_size_peer_list(peers_size));
        if(!temp) {
            fprintf(stderr, "Erro: Falha na alocacao da mensagem!\n");
            return NULL;
        }
        msg = temp;
        sprintf(msg, "%s:%d %d PEER_LIST %d", ip, port, clock, (int)peers_size - 1);
        for(int i = 0; i < peers_size; i++) {
            if(is_same_peer(sender, peers[i]))
                continue;
            sprintf(msg, "%s %s:%d:%s:%d", msg, inet_ntoa(peers[i].con.sin_addr), ntohs(peers[i].con.sin_port), status_string[peers[i].status], peers[i].p_clock);
        }
        sprintf(msg, "%s\n", msg);
        break;
    case LS_LIST:
        lslist_msg_args *llargs = (lslist_msg_args *)args;
        sprintf(msg, "%s:%d %d LS_LIST %d", ip, port, clock, (int)llargs->list_len);
        for(int i = 0; i < llargs->list_len; i++) {
            sprintf(msg, "%s %s:%d", msg, llargs->list[i], llargs->list_file_len[i]);
        }
        sprintf(msg, "%s\n", msg);
        break;
    case DL:
        dl_msg_args *dargs = (dl_msg_args *)args;
        sprintf(msg, "%s:%d %d DL %s %d %d\n", ip, port, clock, dargs->fname, dargs->a, dargs->b);
        break;
    case FILEMSG:
        file_msg_args *fargs = (file_msg_args *)args;
        sprintf(msg, "%s:%d %d FILE %s %d %d %s\n", ip, port, clock, fargs->file_name, fargs->a, fargs->b, fargs->contentb64);
        break;
    default:
        sprintf(msg, "%s:%d %d %s\n", ip, port, clock, message_string[msg_type]);
        break;
    }
    return msg;
}


// send a built message to an known peer socket
SOCKET send_message(const char *msg, peer *neighbour, MSG_TYPE msg_type) {
    if(!msg) {
        fprintf(stderr, "\nErro: Mensagem de envio vazia!\n");
        return INVALID_SOCKET;
    }
    SOCKET server_soc = socket(AF_INET, SOCK_STREAM, 0);
    if(is_invalid_sock(server_soc)) {
        fprintf(stderr, "\nErro: Falha na criacao do socket");
        show_soc_error();
        return INVALID_SOCKET;
    }
    if(connect(server_soc, (const struct sockaddr *)&(neighbour->con), sizeof(neighbour->con)) != 0) {
        fprintf(stderr, "\nErro: Falha ao conectar ao peer\n");
        show_soc_error();
        sock_close(server_soc);
        return INVALID_SOCKET;
    }
    size_t len = strlen(msg);
    printf("\tEncaminhando mensagem \"%.*s\" para %s:%d\n", (int)strcspn(msg, "\n"), msg, inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port));
    if(send(server_soc, msg, len + 1, 0) == len + 1) {
        if(neighbour->status == OFFLINE) {
            neighbour->status = ONLINE;
            printf("\tAtualizando peer %s:%d status %s\n", inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port), status_string[1]);
        }
    }
    else {
        if(neighbour->status == ONLINE) {
            neighbour->status = OFFLINE;
            printf("\tAtualizando peer %s:%d status %s\n", inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port), status_string[0]);
        }
        show_soc_error();
    }
    return server_soc;
}


// read message, mark its sender and return the message type
MSG_TYPE read_message(const char *buf, peer *sender) {
    char *buf_cpy = malloc(sizeof(char) * MSG_SIZE);
    strdup(buf_cpy, buf, MSG_SIZE);
    char *tok_ip = strtok(buf_cpy, " ");
    int aclock = atoi(strtok(NULL, " "));
    char *tok_msg = strtok(NULL, " ");

    if(tok_msg) {
        tok_msg[strcspn(tok_msg, "\n")] = '\0';
    }
    create_address(sender, tok_ip, aclock);
    if(strcmp(tok_msg, "HELLO") == 0) {
        free(buf_cpy);
        return HELLO;
    }
    if(strcmp(tok_msg, "GET_PEERS") == 0) {
        free(buf_cpy);
        return GET_PEERS;
    }
    if(strcmp(tok_msg, "PEER_LIST") == 0) {
        free(buf_cpy);
        return PEER_LIST;
    }
    if(strcmp(tok_msg, "LS") == 0) {
        free(buf_cpy);
        return LS;
    }
    if(strcmp(tok_msg, "LS_LIST") == 0) {
        free(buf_cpy);
        return LS_LIST;
    }
    if(strcmp(tok_msg, "DL") == 0) {
        free(buf_cpy);
        return DL;
    }
    if(strcmp(tok_msg, "FILE") == 0) {
        free(buf_cpy);
        return FILEMSG;
    }
    if(strcmp(tok_msg, "BYE") == 0) {
        free(buf_cpy);
        return BYE;
    }
    return UNEXPECTED_MSG_TYPE;
}


// check if message received was read fully
char *check_msg_full(const char *buf, SOCKET sock, MSG_TYPE msg_type, void *args, ssize_t *valread) { //TODO: Ajeitar funcao para aceitar tipos de mensagens novos 
    bool is_full_msg = false;
    if(*valread == MSG_SIZE - 1) {
        for(int i = 0; i < *valread; i++) {
            if(buf[i] == '\n') {
                is_full_msg = true;
                break;
            }
        }
        if(!is_full_msg) {
            switch(msg_type) {
            case DL:
                break;
            case FILEMSG:
                break;
            default:
                ;
                int spaces = 0;
                char *duplicate = malloc(sizeof(char) * 12);
                int *rec_size = (int *)args;
                for(int i = 0; i < *valread; i++) {
                    if(buf[i] == ' ') {
                        spaces++;
                        if(spaces == 3) {
                            strncpy(duplicate, &buf[i + 1], 11);
                        }
                        if(spaces == 4 && duplicate) {
                            duplicate[strcspn(duplicate, " ")] = '\0';
                            *rec_size = atoi(duplicate);
                            free(duplicate);
                            int new_size = msg_size_peer_list(*rec_size);
                            char *aux_buf;
                            aux_buf = malloc(sizeof(char) * new_size);
                            if(!aux_buf) return NULL;
                            sprintf(aux_buf, "%s", buf);
                            *valread += recv(sock, &aux_buf[*valread], new_size, 0);
                            return aux_buf;
                        }
                    }
                }
                break;
            }
        }
    }
    return NULL;
}


//append list received to known list
void append_file_list(const char *buf, ls_files *list, size_t list_len, peer sender, size_t rec_files_len) {
    char *cpy = strdup(buf);
    strtok(cpy, " "); //ip
    strtok(NULL, " ");//clock
    strtok(NULL, " ");//type
    strtok(NULL, " ");//size
    char *list = strtok(NULL, "\n");

    if(!list) {
        fprintf(stderr, "Erro: Lista de arquivos nao encontrada!\n");
        free(cpy);
        return;
    }

    char *p = NULL;

    size_t info_count = 0;
    for(int i = 0; i < rec_files_len; i++, info_count++) {
        //TODO:
        //msg = <name>:<size>
        char cpy_l = strdup(list);
        for(int j = 0; j <= info_count; j++) {
            if(j == 0) p = strtok(cpy_l, " ");
            else p = strtok(NULL, " ");
        }
        char *infoname = strtok(p, ":");
        char *infosize = strtok(NULL, "\0");
        list[list_len - rec_files_len + info_count].file.fname = strdup(infoname);
        list[list_len - rec_files_len + info_count].file.fsize = atoi(infosize);
        list[list_len - rec_files_len + info_count].holder = sender;

        free(cpy_l);
    }

    free(cpy);
}


//return the file in base64 format
char *get_file_in_msg(const char *buf,int *a, int *b) {
    char *cpy = strdup(buf);
    strtok(cpy, " "); //ip
    strtok(NULL, " "); //clock
    strtok(NULL, " "); //type
    char *infos = strtok(NULL, "\n");
    *a = atoi(strtok(NULL, " "));
    *b = atoi(strtok(NULL, " "));
    return strtok(NULL, " ");
}


// send a bye message to every peer in list
void bye_peers(peer server, peer *peers, size_t peers_size) {
    char *msg = build_message(server.con, server.p_clock, BYE, NULL);
    for(int i = 0; i < peers_size; i++) {
        if(peers[i].status == ONLINE) {
            SOCKET s = send_message(msg, &peers[i], BYE);
            sock_close(s);
            free(msg);
        }
    }
}