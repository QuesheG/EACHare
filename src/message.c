#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <inttypes.h>
#include <file.h>
#include <sock.h>
#include <peer.h>
#include <message.h>
#include <threading.h>
#include <base64.h>

char *message_string[] = { "UNEXPECTED_MSG_TYPE", "HELLO", "GET_PEERS", "PEER_LIST", "LS", "LS_LIST", "DL", "FILE", "BYE" };

// print the peers in list
void show_peers(peer *server, pthread_mutex_t *clock_lock, ArrayList *peers) {
    printf("Peer list:\n");
    printf("\t[0] back\n");

    for(int i = 0; i < peers->count; i++) {
        printf("\t[%d] %s:%u ", i + 1, inet_ntoa(((peer *)peers->elements)[i].con.sin_addr), ntohs(((peer *)peers->elements)[i].con.sin_port));
        if(((peer *)peers->elements)[i].status == ONLINE)
            printf("%s\n", status_string[1]);
        else
            printf("%s\n", status_string[0]);
    }

    printf(">");

    int input;
    if(scanf("%d", &input) != 1) {
        while(getchar() != '\n')
            ;
        fprintf(stderr, "Error: Invalid input! Try again.\n");
        return;
    }
    else if(input == 0)
        return;
    else if(input > 0 && input <= peers->count) {
        update_clock(server, clock_lock, 0);
        char *msg = build_message(server->con, server->p_clock, HELLO, NULL);
        if(!msg) {
            fprintf(stderr, "Error: Failed building message\n");
            return;
        }
        peer *chosen = &(((peer*)peers->elements)[input-1]);
        SOCKET s = send_message(msg, chosen);
        if(!is_invalid_sock(s)) sock_close(s);
        free(msg);
    }
    else {
        fprintf(stderr, "Error: Unexpected input! Try again.\n");
        return;
    }
}

// request the peers list of every known peer
void get_peers(peer *server, pthread_mutex_t *clock_lock, ArrayList *peers /*, char *file*/) {
    size_t loop_len = peers->count;
    for(int i = 0; i < loop_len; i++) {
        update_clock(server, clock_lock, 0);
        char *msg = build_message(server->con, server->p_clock, GET_PEERS, NULL);
        if(!msg) {
            perror("Error: Failed building message\n");
            return;
        }
        SOCKET req = send_message(msg, &(((peer *)peers->elements)[i]));
        if(is_invalid_sock(req)) {
            free(msg);
            continue;
        }
        char *buf = calloc(MSG_SIZE, sizeof(char));
        ssize_t valread = recv(req, buf, MSG_SIZE - 1, 0);
        if(valread <= 0) {
            fprintf(stderr, "\nError: Failed reading message\n");
            free(buf);
            free(msg);
            continue;
        }
        peer sender;
        MSG_TYPE msg_type = read_message(buf, &sender);
        int rec_peers_size = 0;
        char *temp = check_msg_full(buf, req, msg_type, (void *)&rec_peers_size, &valread);
        if(temp) {
            free(buf);
            buf = temp;
        }

        buf[valread] = '\0';

        printf("\n");
        printf("\tAnswer received: \"%.*s\"\n", (int)strcspn(buf, "\n"), buf);
        update_clock(server, clock_lock, sender.p_clock);
        append_list_peers(buf, peers, rec_peers_size /*, file*/);
        sock_close(req);
        free(buf);
        free(msg);
    }
}

// share the peers list with who requested
void share_peers_list(peer *server, pthread_mutex_t *clock_lock, SOCKET soc, peer sender, ArrayList *peers) {
    peer_list_args args;
    args.sender = sender;
    args.peers = (peer *)peers->elements;
    args.peers_size = peers->count;
    update_clock(server, clock_lock, 0);
    char *msg = build_message(server->con, server->p_clock, PEER_LIST, (void *)&args);
    if(!msg) return;
    printf("\tSending message \"%.*s\" to %s:%d\n", (int)strcspn(msg, "\n"), msg, inet_ntoa(sender.con.sin_addr), ntohs(sender.con.sin_port));
    send_complete(soc, msg, strlen(msg) + 1, 0);
    sock_close(soc);
    free(msg);
}

bool file_in_list(char **files, size_t len, char *nf) {
    for(int i = 0; i < len; i++) {
        if(strcmp(files[i], nf) == 0) return true;
    }
    return false;
}

// asks for files of all peers
void get_files(peer *server, pthread_mutex_t *clock_lock, ArrayList *peers, const char *dir_path, ArrayList *files_list, int size_chunk, ArrayList *statistics) {
    ArrayList *files = receive_files(server, clock_lock, peers);
    if(!files || files->count == 0) {
        printf("No files found\n");
        return;
    }
    print_files_received(files);
    printf("Input file number to download it:\n");
    int f_to_down = 0;
    printf(">");
    if(scanf("%d", &f_to_down) != 1) {
        while(getchar() != '\n')
            ;
        printf("Unexpected command\n");
        f_to_down = 0;
    }
    if(!f_to_down)
        return;
    if(f_to_down > files->count)
        return;
    f_to_down--;

    ls_files chosen_file = ((ls_files *)files->elements)[f_to_down];
    char *file_path = dir_file_path(dir_path, chosen_file.file.fname);
    FILE *new_file = fopen(file_path, "wb");
    if(!new_file) {
        perror("Error: Failed opening file");
        free(file_path);
        free(files);
        return;
    }
    make_file_size(new_file, chosen_file.file.fsize);

    fclose(new_file);

    download_file(server, clock_lock, chosen_file, dir_path, size_chunk, statistics);
    char *n_entry = strdup(chosen_file.file.fname);
    if(!n_entry) {
        fprintf(stderr, "Error: Failed appending file to list\n");
        for(int i = 0; i < files->count; i++)
            free(((ls_files *)files->elements)[i].file.fname);
        free_list(files);
        free(file_path);
        return;
    }
    if(!file_in_list((char **)files_list->elements, files_list->count, n_entry))
        append_element(files_list, (void *)&n_entry);
    else free(n_entry);
    for(int i = 0; i < files->count; i++) {
        free(((ls_files *)files->elements)[i].file.fname);
        free_list(((ls_files *)files->elements)[i].holders);
    }
    free_list(files);
    free(file_path);
}

// send list of files
void share_files_list(peer *server, pthread_mutex_t *clock_lock, SOCKET con, peer sender, char *dir_path) {
    size_t files_len = 0;
    char **files = get_dir_files(dir_path, &files_len);
    if(!files) {
        lslist_msg_args *llargs = malloc(sizeof(lslist_msg_args));
        llargs->list_len = 0;
        char *msg = build_message(server->con, server->p_clock, LS_LIST, (void *)llargs);
        if(!msg) {
        }
        send_complete(con, msg, strlen(msg) + 1, 0);
        sock_close(con);
        printf("\tSending message \"%.*s\" to %s:%d\n", (int)strcspn(msg, "\n"), msg, inet_ntoa(sender.con.sin_addr), ntohs(sender.con.sin_port));
        free(llargs);
        free(msg);
        return;
    }
    lslist_msg_args *llargs = malloc(sizeof(lslist_msg_args));
    if(!llargs) {
        fprintf(stderr, "Error: Failed allocating args\n");
        return;
    }
    llargs->list_file_len = malloc(sizeof(size_t) * files_len);
    if(!llargs->list_file_len) {
        fprintf(stderr, "Error: Failed allocating args\n");
        return;
    }
    llargs->list = files;
    for(size_t i = 0; i < files_len; i++) {
        char *file = dir_file_path(dir_path, files[i]);
        llargs->list_file_len[i] = fsize(file);
        free(file);
    }
    llargs->list_len = files_len;
    char *msg = build_message(server->con, server->p_clock, LS_LIST, (void *)llargs);
    if(msg)
        if(send_complete(con, msg, strlen(msg) + 1, 0) == 0)
            printf("\tSending message \"%.*s\" to %s:%d\n", (int)strcspn(msg, "\n"), msg, inet_ntoa(sender.con.sin_addr), ntohs(sender.con.sin_port));
    sock_close(con);
    for(int i = 0; i < files_len; i++) {
        free(files[i]);
    }
    free(files);
    free(llargs->list_file_len);
    free(llargs);
    free(msg);
}

// send file
void send_file(peer *server, pthread_mutex_t *clock_lock, char *buf, SOCKET con, peer sender, char *dir_path) {
    if(!dir_path) {
        fprintf(stderr, "Error: No directory found\n");
        sock_close(con);
        free(buf);
        return;
    }
    file_msg_args *fargs = malloc(sizeof(file_msg_args));
    if(!fargs) {
        perror("Error: Failed allocating args\n");
        sock_close(con);
        free(buf);
        return;
    }
    int clock = 0;
    char *a = get_file_in_msg(buf, &clock, &(fargs->file_name), &(fargs->chunk_size), &(fargs->offset));
    if(a)
        free(a);
    if(!fargs->file_name) {
        fprintf(stderr, "Error: Failed finding requested file\n");
        free(fargs);
        sock_close(con);
        return;
    }
    char *file_path = dir_file_path(dir_path, fargs->file_name);
    if(!file_path) {
        fprintf(stderr, "Error: Failed creating file path\n");
        sock_close(con);
        free(fargs);
        return;
    }
    FILE *file = fopen(file_path, "rb");
    free(file_path);
    if(!file) {
        fprintf(stderr, "Error: Failed opening file\n");
        sock_close(con);
        free(fargs);
        return;
    }
    if(fseek(file, fargs->chunk_size * fargs->offset, SEEK_SET) < 0) {
        perror("Error: Failed seeking file");
        fclose(file);
        sock_close(con);
        free(fargs->file_name);
        free(fargs);
        return;
    }
    char *file_cont = calloc(fargs->chunk_size, sizeof(char));
    if(!file_cont) {
        fprintf(stderr, "Error: Failed allocating file content\n");
        fclose(file);
        sock_close(con);
        free(fargs);
        return;
    }
    size_t bytes_read = fread(file_cont, 1, fargs->chunk_size, file);
    fclose(file);
    size_t bytes_encode = base64encode_len(bytes_read);
    char *encoded = malloc(sizeof(char) * bytes_encode);
    if(!encoded) {
        fprintf(stderr, "Error: Failed allocating encrypted file\n");
        sock_close(con);
        free(fargs->file_name);
        free(fargs);
        free(file_cont);
        return;
    }
    if(base64_encode(encoded, file_cont, bytes_read) != bytes_encode) {
        fprintf(stderr, "Error: Failed encoding message\n");
        sock_close(con);
        free(encoded);
        free(fargs->file_name);
        free(fargs);
        free(file_cont);
        return;
    }
    fargs->contentb64 = encoded;
    char *msg = build_message(server->con, server->p_clock, FILEMSG, (void *)fargs);
    if(msg) {
        printf("\tSending message \"%.*s\" to %s:%d\n", (int)MIN(strcspn(msg, "\n"), MSG_SIZE), msg, inet_ntoa(sender.con.sin_addr), ntohs(sender.con.sin_port));
        send_complete(con, msg, strlen(msg) + 1, 0);
        free(msg);
    }
    else fprintf(stderr, "Error: Failed building message\n");
    sock_close(con);
    free(encoded);
    free(fargs->file_name);
    free(fargs);
    free(file_cont);
}

// create a message with the sender ip, its clock and the message type
char *build_message(sockaddr_in sender_ip, int clock, MSG_TYPE msg_type, void *args) {
    char *ip = inet_ntoa(sender_ip.sin_addr);
    u_short port = ntohs(sender_ip.sin_port);
    char *msg = malloc(sizeof(char) * MSG_SIZE);
    if(!msg) {
        fprintf(stderr, "Error: Failed allocating message\n");
        return NULL;
    }
    if(msg_type < 0 || msg_type > BYE) {
        free(msg);
        msg = NULL;
        return msg;
    }
    char *temp;
    switch(msg_type) {
        /*
        MESSAGE FORMATING: <sender_ip>:<sender_port> <sender_clock> <MSG_TYPE> [<ARGS...>]\n <- important newline
        */
    case PEER_LIST:;
        peer_list_args *list_args = (peer_list_args *)args;
        peer sender = list_args->sender;
        size_t peers_size = list_args->peers_size;
        peer *peers = list_args->peers;
        temp = realloc(msg, sizeof(char) * msg_size_peer_list(peers_size));
        if(!temp) {
            fprintf(stderr, "Error: Failed allocating message\n");
            return NULL;
        }
        msg = temp;
        sprintf(msg, "%s:%u %d PEER_LIST %d", ip, port, clock, (int)peers_size - 1);
        for(int i = 0; i < peers_size; i++) {
            if(is_same_peer(sender, peers[i]))
                continue;
            sprintf(msg, "%s %s:%d:%s:%" PRIu64, msg, inet_ntoa(peers[i].con.sin_addr), ntohs(peers[i].con.sin_port), status_string[peers[i].status], peers[i].p_clock);
        }
        sprintf(msg, "%s\n", msg);
        break;
    case LS_LIST:;
        lslist_msg_args *llargs = (lslist_msg_args *)args;
        if(llargs->list_len == 0) {
            sprintf(msg, "%s:%u %d LS_LIST 0\n", ip, port, clock);
            return msg;
        }
        temp = realloc(msg, sizeof(char) * msg_size_files_list(llargs->list_len));
        if(!temp) {
            fprintf(stderr, "Error: Failed allocating message!\n");
            return NULL;
        }
        msg = temp;
        sprintf(msg, "%s:%u %d LS_LIST %d", ip, port, clock, (int)llargs->list_len);
        for(int i = 0; i < llargs->list_len; i++) {
            sprintf(msg, "%s %s:%d", msg, llargs->list[i], (int)llargs->list_file_len[i]);
        }
        sprintf(msg, "%s\n", msg);
        break;
    case DL:;
        dl_msg_args *dargs = (dl_msg_args *)args;
        temp = realloc(msg, sizeof(char) * msg_size_files_list(1));
        if(!temp) {
            fprintf(stderr, "Error: Failed allocating message!\n");
            return NULL;
        }
        msg = temp;
        sprintf(msg, "%s:%u %d DL %s %d %d\n", ip, port, clock, dargs->fname, dargs->chunk_size, dargs->offset);
        break;
    case FILEMSG:;
        file_msg_args *fargs = (file_msg_args *)args;
        temp = realloc(msg, sizeof(char) * (msg_size_files_list(1) + base64encode_len(fargs->chunk_size)));
        if(!temp) {
            fprintf(stderr, "Error: Failed allocating message!\n");
            return NULL;
        }
        msg = temp;
        sprintf(msg, "%s:%u %d FILE %s %d %d %s\n", ip, port, clock, fargs->file_name, fargs->chunk_size, fargs->offset, fargs->contentb64);
        break;
    default:
        sprintf(msg, "%s:%u %d %s\n", ip, port, clock, message_string[msg_type]);
        break;
    }
    return msg;
}

// send a built message to an known peer socket
SOCKET send_message(const char *msg, peer *neighbour) {
    if(!msg) {
        fprintf(stderr, "\nError: Empty message\n");
        return INVALID_SOCKET;
    }
    if(!neighbour) {
        fprintf(stderr, "\nError: No peer found\n");
        return INVALID_SOCKET;
    }
    SOCKET server_soc = socket(AF_INET, SOCK_STREAM, 0);
    if(is_invalid_sock(server_soc)) {
        fprintf(stderr, "\nError: Failed creating socket");
        show_soc_error();
        return INVALID_SOCKET;
    }
    #ifdef WIN
    bool yes = true;
    if(setsockopt(server_soc, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes))!=0) {
        fprintf(stderr, "\nError: Failed defining socket option\n");
        show_soc_error();
        sock_close(server_soc);
    }
    yes = !yes;
    if(setsockopt(server_soc, SOL_SOCKET, SO_DONTLINGER, (char*)&yes, sizeof(yes))!=0) {
        fprintf(stderr, "\nError: Failed defining socket option\n");
        show_soc_error();
        sock_close(server_soc);
    }
    #endif
    if(connect(server_soc, (const struct sockaddr *)&(neighbour->con), sizeof(neighbour->con)) != 0) {
        fprintf(stderr, "\nError: Failed connecting to peer\n");
        show_soc_error();
        sock_close(server_soc);
        if(neighbour->status == ONLINE) {
            neighbour->status = OFFLINE;
            printf("\tUpdating peer %s:%d status %s\n", inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port), status_string[0]);
        }
        return INVALID_SOCKET;
    }
    size_t len = strlen(msg);
    printf("\tSending message \"%.*s\" to %s:%d\n", (int)strcspn(msg, "\n"), msg, inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port));
    if(send_complete(server_soc, msg, len + 1, 0) == 0) {
        if(neighbour->status == OFFLINE) {
            neighbour->status = ONLINE;
            printf("\tUpdate peer %s:%d status %s\n", inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port), status_string[1]);
        }
    }
    else {
        if(neighbour->status == ONLINE) {
            neighbour->status = OFFLINE;
            printf("\tUpdate peer %s:%d status %s\n", inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port), status_string[0]);
        }
        show_soc_error();
    }
    return server_soc;
}

// read message, mark its sender and return the message type
MSG_TYPE read_message(const char *buf, peer *sender) {
    char *buf_cpy = calloc(MSG_SIZE, sizeof(char));
    if(!buf_cpy)
        return UNEXPECTED_MSG_TYPE;
    strncpy(buf_cpy, buf, MSG_SIZE);
    char *svptr = buf_cpy;
    char *tok_ip = strtok_r(buf_cpy, " ", &svptr);
    if(!tok_ip) {
        printf("\n\n%s, %s\n\n\n", buf, buf_cpy);
        free(buf_cpy);
        return UNEXPECTED_MSG_TYPE;
    }
    char *cls = strtok_r(NULL, " ", &svptr);
    if(!cls) {
        free(buf_cpy);
        return UNEXPECTED_MSG_TYPE;
    }
    int aclock = atoi(cls);
    char *tok_msg = strtok_r(NULL, " ", &svptr);

    if(tok_msg) {
        tok_msg[strcspn(tok_msg, "\n")] = '\0';
    }
    create_address(sender, tok_ip, aclock);
    MSG_TYPE ret_msg_type = UNEXPECTED_MSG_TYPE;
    if(!tok_msg);
    else if(strcmp(tok_msg, "HELLO") == 0)
        ret_msg_type = HELLO;
    else if(strcmp(tok_msg, "GET_PEERS") == 0)
        ret_msg_type = GET_PEERS;
    else if(strcmp(tok_msg, "PEER_LIST") == 0)
        ret_msg_type = PEER_LIST;
    else if(strcmp(tok_msg, "LS") == 0)
        ret_msg_type = LS;
    else if(strcmp(tok_msg, "LS_LIST") == 0)
        ret_msg_type = LS_LIST;
    else if(strcmp(tok_msg, "DL") == 0)
        ret_msg_type = DL;
    else if(strcmp(tok_msg, "FILE") == 0)
        ret_msg_type = FILEMSG;
    else if(strcmp(tok_msg, "BYE") == 0)
        ret_msg_type = BYE;
    if(buf_cpy) free(buf_cpy);
    return ret_msg_type;
}

// check if message received was read fully
char *check_msg_full(const char *buf, SOCKET sock, MSG_TYPE msg_type, void *args, ssize_t *valread) {
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
            case DL:;
                char *aux_buf = calloc(msg_size_files_list(1), sizeof(char));
                if(!aux_buf) {
                    fprintf(stderr, "Error: Failed allocating\n");
                    return NULL;
                }
                sprintf(aux_buf, "%s", buf);
                *valread = recv(sock, &aux_buf[*valread], msg_size_files_list(1) - *valread, 0);
                return aux_buf;
                break;
            default:;
                int spaces = 0;
                char *duplicate = calloc(MSG_SIZE, sizeof(char));
                if(!duplicate) {
                    fprintf(stderr, "Error: Failed allocating\n");
                    return NULL;
                }
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
                            int new_size = 0;
                            if(msg_type == LS_LIST)
                                new_size = msg_size_files_list(*rec_size);
                            else
                                new_size = msg_size_peer_list(*rec_size);
                            char *aux_buf = calloc(new_size, sizeof(char));
                            if(!aux_buf)
                                return NULL;
                            sprintf(aux_buf, "%s", buf);
                            *valread += recv(sock, &(aux_buf[*valread]), new_size - *valread, 0);
                            return aux_buf;
                        }
                    }
                }
                break;
            }
        }
    }
    else if(args) {
        int spaces = 0;
        char *duplicate = calloc(MSG_SIZE, sizeof(char));
        if(!duplicate) {
            fprintf(stderr, "Error: Failed allocating\n");
            return NULL;
        }
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
                    return NULL;
                }
            }
        }
    }
    return NULL;
}

ArrayList *receive_files(peer *server, pthread_mutex_t *clock_lock, ArrayList *peers) {
    ArrayList *files = alloc_list(sizeof(ls_files));

    for(int i = 0; i < peers->count; i++) {
        if(((peer *)peers->elements)[i].status == OFFLINE)
            continue;
        update_clock(server, clock_lock, 0);
        char *msg = build_message(server->con, server->p_clock, LS, NULL);
        if(!msg) {
            perror("Error: Failed building message\n");
            free_list(files);
            return NULL;
        }
        SOCKET req = send_message(msg, &(((peer *)peers->elements)[i]));
        if(is_invalid_sock(req)) {
            free(msg);
            continue;
        }
        char *buf = calloc(MSG_SIZE, sizeof(char));
        ssize_t valread = recv(req, buf, MSG_SIZE - 1, 0);
        if(valread <= 0) {
            fprintf(stderr, "\nError: Failed reading message\n");
            free(buf);
            free(msg);
            continue;
        }
        peer sender;
        MSG_TYPE msg_type = read_message(buf, &sender);
        sender.status = ONLINE;

        size_t rec_files_size = 0;
        char *temp = check_msg_full(buf, req, msg_type, (void *)&rec_files_size, &valread);
        if(temp) {
            free(buf);
            buf = temp;
        }
        buf[valread] = '\0';

        if(rec_files_size == 0) {
            printf("\tAnswer received: \"%.*s\"\n", (int)strcspn(buf, "\n"), buf);
            update_clock(server, clock_lock, sender.p_clock);
            free(buf);
            free(msg);
            sock_close(req);
            continue;
        }

        printf("\tAnswer received: \"%.*s\"\n", (int)strcspn(buf, "\n"), buf);
        update_clock(server, clock_lock, sender.p_clock);
        printf("\n");
        append_files_list(buf, files, sender, rec_files_size);
        free(buf);
        sock_close(req);
        free(msg);
    }
    return files;
}

void print_files_received(ArrayList *files) {
    if(!files)
        return;
    if(files->size_elements != sizeof(ls_files)) {
        fprintf(stderr, "Error: Unexpected list type\n");
    }
    uint16_t max_fname_size = 10;
    uint16_t max_fsize_size = 4;
    for(int i = 0; i < files->count; i++) {
        uint16_t s_f = strlen(((ls_files *)files->elements)[i].file.fname);
        uint16_t s_s = (uint16_t)log10(((ls_files *)files->elements)[i].file.fsize) + 1;
        if(s_f > max_fname_size)
            max_fname_size = s_f;
        if(s_s > max_fsize_size)
            max_fsize_size = s_s;
    }
    uint16_t count_size = floor(log10(files->count)) + 1;
    // print header
    printf("%*.s ", count_size + 2, "");
    printf("%-*s ", max_fname_size, "Name");
    printf("|");
    printf(" %-*s ", max_fsize_size, "Size");
    printf("|");
    printf(" %s\n", "Peer");
    // print cancel
    printf("[%0*d] ", count_size, 0);
    printf("%-*s ", max_fname_size, "<Cancel>");
    printf("|");
    printf(" %*.s ", max_fsize_size, "");
    printf("|");
    printf("\n");
    // print list
    for(int i = 0; i < files->count; i++) {
        ls_files ilsf = ((ls_files *)files->elements)[i];
        peer *holders = (peer *)ilsf.holders->elements;
        printf("[%0*d] ", count_size, i + 1);
        printf("%-*s", max_fname_size, ilsf.file.fname);
        printf(" | ");
        printf("%-*" PRIu64, max_fsize_size, (uint64_t)ilsf.file.fsize);
        printf(" | ");
        printf("%s:%d", inet_ntoa(holders[0].con.sin_addr), ntohs(holders[0].con.sin_port));
        for(int j = 1; j < ilsf.peers_size; j++)
            printf(", %s:%d ", inet_ntoa(holders[j].con.sin_addr), ntohs(holders[j].con.sin_port));
        printf("\n");
    }
}

void *download_file_thread(void *args) {
    work_download *a = (work_download *)args;
    peer *server = a->server;
    pthread_mutex_t *lock = a->lock;
    uint8_t id = a->thread_id;
    uint8_t threads_size = a->thread_l_size;
    ls_files nfile = a->file;
    int chunk = a->chunk_size;
    char *file_path = dir_file_path(a->dir_path, nfile.file.fname);
    int round = 0;
    uint64_t offset = id;
    FILE *file = fopen(file_path, "r+b");
    free(file_path);
    uint8_t patience = 0;
    uint8_t resistance = 0;
    ArrayList *holders_list = cpy_list(nfile.holders);
    char *fname = strdup(nfile.file.fname);
    while(offset * chunk < nfile.file.fsize) {
        peer *holders = holders_list->elements;
        size_t ppos = 0;
        if(holders_list->count > 0)
            ppos = (id + round) % holders_list->count;
        if(patience > 5) {
            resistance++;
            mssleep(100);
            if(resistance > 5)
                if(holders_list->count > 0)
                    remove_at(holders_list, ppos);
            patience = 0;
        }
        if(holders_list->count <= 0) {
            fprintf(stderr, "\nError: Failed downloading file, maximum # of tries exceeded\n");
            free_list(holders_list);
            free(fname);
            fclose(file);
            free(a);
            pthread_exit(NULL);
            return NULL;
        }
        peer req_peer = holders[ppos];
        dl_msg_args *dargs = malloc(sizeof(*dargs));
        if(!dargs) {
            perror("Error: Failed allocating args\n");
            patience++;
            continue;
        }
        dargs->chunk_size = chunk;
        dargs->fname = fname;
        dargs->offset = offset;
        update_clock(server, lock, 0);
        char *msg = build_message(server->con, server->p_clock, DL, (void *)dargs);
        free(dargs);
        if(!msg) {
            patience++;
            continue;
        }
        SOCKET req = send_message(msg, &(req_peer));
        if(is_invalid_sock(req)) {
            // fprintf(stderr, "\nErro: Falha com socket\n");
            // show_soc_error();
            free(msg);
            patience++;
            continue;
        }
        free(msg);
        size_t temp_size = MSG_SIZE + 26 + base64encode_len(chunk) + 1;
        char *buf = calloc(temp_size, sizeof(char));
        if(!buf) {
            perror("\nError: Failed allocating receiving buffer\n");
            sock_close(req);
            continue;
        }
        ssize_t total_received = 0;
        set_sock_timeout(req, 2);
        while(total_received < temp_size) {
            ssize_t valread = recv(req, buf + total_received, temp_size - total_received, 0);
            if(valread < 0) {
                fprintf(stderr, "\nError: Failed receiving\n");
                show_soc_error();
                free(buf);
                buf = 0;
                patience++;
                break;
            }
            if(!valread)
                break;
            bool done = false;
            for(int i = 0; i < valread; i++) {
                if(buf[total_received + i] == '\n') {
                    done = true;
                    break;
                }
            }
            total_received += valread;
            if(done)
                break;
        }
        if(!buf) {
            sock_close(req);
            continue;
        }
        if(total_received == 0) {
            fprintf(stderr, "Error: Bad formatted message\n");
            free(buf);
            sock_close(req);
            patience++;
            continue;
        }
        buf[total_received] = 0;
        sock_close(req);
        printf("\n");
        printf("\n\tAnswer received:  \"%.*s\"\n", (int)MIN(strcspn(buf, "\n"), MSG_SIZE), buf);

        int rec_chunk, soffset;
        int clock = 0;
        char *file_b64 = get_file_in_msg(buf, &clock, NULL, &rec_chunk, &soffset);
        free(buf);
        update_clock(server, lock, clock);
        if(!file_b64) {
            fprintf(stderr, "Error: Failed receiving file\n");
            patience++;
            continue;
        }
        if(soffset != offset || rec_chunk > chunk) {
            fprintf(stderr, "Error: Unexpected message\n");
            free(file_b64);
            continue;
        }
        size_t clen = base64decode_len(file_b64);
        char *file_decoded = malloc(sizeof(char) * (clen + 1));
        if(!file_decoded) {
            fprintf(stderr, "Error: Failed allocating\n");
            free(file_b64);
            continue;
        }
        int decode_size = base64_decode(file_decoded, file_b64);
        if(fseek(file, offset * chunk, SEEK_SET) != 0) {
            perror("Error: Failed seeking file\n");
            free(file_decoded);
            free(file_b64);
            patience++;
            continue;
        }
        size_t w = fwrite(file_decoded, 1, decode_size, file);
        if(w != decode_size) {
            fprintf(stderr, "Error: Failed writing file: expected %d, written %d\n", decode_size, (int)w);
            free(file_b64);
            free(file_decoded);
            patience++;
            continue;
        }
        free(file_b64);
        free(file_decoded);
        round++;
        resistance = 0;
        patience = 0;
        offset = id + (round * threads_size);
    }
    free_list(holders_list);
    free(fname);
    fclose(file);
    free(a);
    pthread_exit(NULL);
    return NULL;
}

int stat_in_list(stat_block *list, size_t list_size, stat_block new_stat) {
    for(int i = 0; i < list_size; i++) {
        if(list[i].chunk_used == new_stat.chunk_used && list[i].fsize == new_stat.fsize && list[i].n_peers == new_stat.n_peers)
            return i;
    }
    return -1;
}

double get_average_time(double *list, size_t list_size) {
    double avrg = 0;
    for(int i = 0; i < list_size; i++) {
        avrg += list[i];
    }
    if(!list_size) return 0;
    avrg /= list_size;
    return avrg;
}

double get_std_deviation(double *list, size_t list_size, double avrg) {
    double var = 0;
    for(int i = 0; i < list_size; i++) {
        var += (list[i] - avrg) * (list[i] - avrg);
    }
    if(!list_size) return 0;
    var /= list_size;
    return sqrt(var);
}

void download_file(peer *server, pthread_mutex_t *clock_lock, ls_files chosen_file, const char *dir_path, size_t size_chunk, ArrayList *statistics) {
    uint8_t size_thread_pool = 4;
    pthread_t *thread_pool = malloc(sizeof(pthread_t) * size_thread_pool);
    clock_t init = clock();
    for(int i = 0; i < size_thread_pool; i++) {
        work_download *a = malloc(sizeof(*a));
        a->server = server;
        a->lock = clock_lock;
        a->thread_id = i;
        a->thread_l_size = size_thread_pool;
        a->file = chosen_file;
        a->chunk_size = size_chunk;
        a->dir_path = dir_path;
        a->statistics = statistics;
        pthread_create(&(thread_pool[i]), NULL, download_file_thread, (void *)a);
    }
    for(int i = 0; i < size_thread_pool; i++) {
        //printf("Waiting for thread #%d\n", i);
        pthread_join(thread_pool[i], NULL);
    }

    free(thread_pool);
    double time_taken = ((double)(clock() - init)) / CLOCKS_PER_SEC;
    stat_block n = {
        .chunk_used = size_chunk,
        .fsize = chosen_file.file.fsize,
        .n_peers = chosen_file.holders->count,
        .n = 1,
        .avrg = time_taken,
        .std_dev = 0,
    };
    int i = stat_in_list((stat_block *)statistics->elements, statistics->count, n);
    if(i < 0) {
        n.times = alloc_list(sizeof(double));
        append_element(n.times, (void *)&n.avrg);
        append_element(statistics, (void *)&n);
    }
    else {
        ((stat_block *)statistics->elements)[i].n++;
        append_element(((stat_block *)statistics->elements)[i].times, (void *)&n.avrg);
        ArrayList *times = ((stat_block *)statistics->elements)[i].times;
        double avrg = get_average_time((double *)times->elements, times->count);
        ((stat_block *)statistics->elements)[i].avrg = avrg;
        ((stat_block *)statistics->elements)[i].std_dev = get_std_deviation((double *)times->elements, times->count, avrg);
    }
}

// append list received to known list
void append_files_list(const char *buf, ArrayList *list, peer sender, size_t rec_files_len) {
    char *cpy = strdup(buf);
    char *svptr = cpy;
    strtok_r(svptr, " ", &svptr); // ip
    strtok_r(NULL, " ", &svptr); // clock
    strtok_r(NULL, " ", &svptr); // type
    strtok_r(NULL, " ", &svptr); // size
    char *list_rec = strtok_r(NULL, "\n", &svptr);

    if(!list_rec) {
        fprintf(stderr, "Error: Files list not found\n");
        free(cpy);
        return;
    }

    char *p = NULL;

    bool first_iteration = false;

    if(is_list_empty(*list))
        first_iteration = true;

    for(int i = 0; i < rec_files_len; i++) {
        char *cpy_l = strdup(list_rec);
        char *svptr = cpy_l;
        for(int j = 0; j <= i; j++)
            if(j == 0)
                p = strtok_r(svptr, " ", &svptr);
            else
                p = strtok_r(NULL, " ", &svptr);
        svptr = p;
        char *infoname = strtok_r(svptr, ":", &svptr);
        char *infosize = strtok_r(NULL, "\0", &svptr);
        int position;

        if(first_iteration) {
            position = 0;
            ls_files a = {
                .file.fname = strdup(infoname),
                .file.fsize = atoi(infosize),
                .holders = alloc_list(sizeof(peer)),
                .peers_size = 1,
            };
            append_element(list, (void *)&a);
            first_iteration = false;
        }
        else {
            char *fname = strdup(infoname);
            size_t fsize = atoi(infosize);
            position = list->count;
            int j;
            for(j = 0; j < list->count; j++) {
                if(strcmp(fname, ((ls_files *)list->elements)[j].file.fname) == 0 && fsize == ((ls_files *)list->elements)[j].file.fsize) {
                    position = j;
                    free(fname);
                    break;
                }
            }
            if(position == list->count) {
                ls_files a = {
                    .file.fname = fname,
                    .file.fsize = fsize,
                    .holders = alloc_list(sizeof(peer)),
                    .peers_size = 0,
                };
                append_element(list, (void *)&a);
            }
            ((ls_files *)list->elements)[position].peers_size++;
        }
        append_element(((ls_files *)list->elements)[position].holders, (void *)&sender);

        free(cpy_l);
    }

    free(cpy);
}

// return the file in base64 format
char *get_file_in_msg(char *buf, int *clock, char **fname, int *chunk_size, int*offset) {
    //ip clock type name chunk offset content
    char *bcpy = strdup(buf);
    char *svptr = bcpy;
    strtok_r(svptr, " ", &svptr);           // ip
    char *cls = strtok_r(NULL, " ", &svptr);
    if (!cls) {
        free(bcpy);
        return NULL;
    }
    *clock = atoi(cls);
    strtok_r(NULL, " ", &svptr);           // type
    char *infos = strtok_r(NULL, "\n", &svptr);
    if (!infos) {
        free(bcpy);
        return NULL;
    }
    svptr = infos;
    char *n = strtok_r(svptr, " ", &svptr);
    if (fname && n) *fname = strdup(n);
    char *csl = strtok_r(NULL, " ", &svptr);
    if (!csl) {
        free(bcpy);
        return NULL;
    }
    *chunk_size = atoi(csl);
    char *ofs = strtok_r(NULL, " ", &svptr);
    if (!ofs) {
        free(bcpy);
        return NULL;
    }
    *offset = atoi(ofs);
    char *ret = strtok_r(NULL, "\n", &svptr);
    if (ret) ret = strdup(ret);
    free(bcpy);
    return ret;
}


// print statistics gathered
void print_statistics(ArrayList *statistics) {
    if(!statistics)
        return;
    if(statistics->size_elements != sizeof(stat_block)) {
        fprintf(stderr, "Error: Unexpected list type\n");
        return;
    }
    if(statistics->count == 0) {
        printf("No statistic found yet\n");
        return;
    }
    stat_block *lsf = (stat_block*)statistics->elements;
    uint16_t max_chunk_size = 10;
    uint16_t max_npeers = 7;
    uint16_t max_file_size = 9;
    uint16_t max_n = 1;
    uint16_t max_time = 8;
    uint8_t precision = 4;
    for(int i = 0; i < statistics->len; i++) {
        max_chunk_size = MAX(max_chunk_size, log10l(lsf[i].chunk_used) + 1);
        max_npeers = MAX(max_npeers, log10l(lsf[i].n_peers) + 1);
        max_file_size = MAX(max_file_size, log10l(lsf[i].fsize) + 1);
        max_n = MAX(max_n, log10l(lsf[i].n) + 1);
        max_time = MAX(max_time, log10l(lsf[i].avrg) + 1);
    }
    // print header
    printf("%-*s ", max_chunk_size, "Chunk size");
    printf("|");
    printf(" %*s ", max_npeers, "N peers");
    printf("|");
    printf(" %*s ", max_file_size, "File size");
    printf("|");
    printf(" %*s ", max_n, "N");
    printf("|");
    printf(" %*s ", max_time, "Time [s]");
    printf("|");
    printf(" %s \n", "Deviation");
    // print list
    for(int i = 0; i < statistics->count; i++) {
        printf("%-*d ", max_chunk_size, lsf[i].chunk_used);
        printf("|");
        printf(" %*d ", max_npeers, lsf[i].n_peers);
        printf("|");
        printf(" %*" PRIu64 " ", max_file_size, lsf[i].fsize);
        printf("|");
        printf(" %*d ", max_n, lsf[i].n);
        printf("|");
        printf(" %*.*lf ", max_time, precision, lsf[i].avrg);
        printf("|");
        printf(" %.*lf ", precision, lsf[i].std_dev);
        printf("\n");
    }
}

// change the local chunk_size
void change_chunk_size(int *chunk_size) {
    printf("\nInput new chunk size:\n>");
    int input;
    if(scanf("%d", &input) != 1) {
        while(getchar() != '\n')
            ;
        fprintf(stderr, "Error: Invalid input! Try again.\n");
        return;
    }
    else if(input <= 0) {
        fprintf(stderr, "Input must be bigger than 0! Try again.\n");
        return;
    }
    else *chunk_size = input;
}

// send a bye message to every peer in list
void bye_peers(peer *server, pthread_mutex_t *clock_lock, ArrayList *peers) {
    SOCKET server_soc = socket(AF_INET, SOCK_STREAM, 0);
    if(is_invalid_sock(server_soc)) {
        fprintf(stderr, "\nError: Failed creating socket");
        show_soc_error();
        return;
    }
    set_sock_block(server_soc, true);
    for(int i = 0; i < peers->count; i++) {
        peer neighbour = ((peer*)peers->elements)[i];
        if(neighbour.status == ONLINE) {
            update_clock(server, clock_lock, 0);
            char *msg = build_message(server->con, server->p_clock, BYE, NULL);
            if(!msg) continue;
            if(connect(server_soc, (const struct sockaddr *)&(neighbour.con), sizeof(neighbour.con)) != 0) {
                //fprintf(stderr, "\nErro: Falha ao conectar ao peer\n");
                //show_soc_error();
                free(msg);
                continue;
            }
            size_t len = strlen(msg);
            send_complete(server_soc, msg, len + 1, 0);
            SOCKET s = send_message(msg, &neighbour);
            sock_close(s);
            free(msg);
        }
    }
    sock_close(server_soc);
}
