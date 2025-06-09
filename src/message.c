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

char *message_string[] = {"UNEXPECTED_MSG_TYPE", "HELLO", "GET_PEERS", "PEER_LIST", "LS", "LS_LIST", "DL", "FILE", "BYE"};

// print the peers in list
void show_peers(peer *server, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size)
{
    printf("Lista de peers:\n");
    printf("\t[0] voltar para o menu anterior\n");

    for (int i = 0; i < peers_size; i++)
    {
        printf("\t[%d] %s:%u ", i + 1, inet_ntoa(peers[i].con.sin_addr), ntohs(peers[i].con.sin_port));
        if (peers[i].status == ONLINE)
            printf("%s\n", status_string[1]);
        else
            printf("%s\n", status_string[0]);
    }

    printf(">");

    int input;
    if (scanf("%d", &input) != 1)
    {
        while (getchar() != '\n')
            ;
        fprintf(stderr, "Erro: Entrada invalida! Tente de novo.\n");
        return;
    }
    else if (input == 0)
        return;
    else if (input > 0 && input <= peers_size)
    {
        pthread_mutex_lock(clock_lock);
        server->p_clock++;
        pthread_mutex_unlock(clock_lock);
        printf("\t=> Atualizando relogio para %d\n", server->p_clock);
        char *msg = build_message(server->con, server->p_clock, HELLO, NULL);
        if (!msg)
        {
        }
        if (!msg)
        {
            fprintf(stderr, "Erro: Falha na construcao da mensagem!\n");
            return;
        }
        SOCKET s = send_message(msg, &(peers[input - 1]), HELLO);
        sock_close(s);
        free(msg);
    }
    else
    {
        fprintf(stderr, "Erro: Entrada invalida! Tente de novo.\n");
        return;
    }
}

// request the peers list of every known peer
void get_peers(peer *server, pthread_mutex_t *clock_lock, peer **peers, size_t *peers_size /*, char *file*/)
{
    size_t loop_len = *peers_size;
    for (int i = 0; i < loop_len; i++)
    {
        pthread_mutex_lock(clock_lock);
        server->p_clock++;
        pthread_mutex_unlock(clock_lock);
        printf("\t=> Atualizando relogio para %d\n", server->p_clock);
        char *msg = build_message(server->con, server->p_clock, GET_PEERS, NULL);
        if (!msg)
        {
        }
        if (!msg)
        {
            perror("Erro: Falha na construcao da mensagem!\n");
            return;
        }
        SOCKET req = send_message(msg, &((*peers)[i]), GET_PEERS);
        if (is_invalid_sock(req))
        {
            free(msg);
            continue;
        }
        char *buf = calloc(MSG_SIZE, sizeof(char));
        ssize_t valread = recv(req, buf, MSG_SIZE - 1, 0);
        if (valread <= 0)
        {
            fprintf(stderr, "\nErro: Falha lendo mensagem\n");
            free(buf);
            free(msg);
            continue;
        }
        peer sender;
        MSG_TYPE msg_type = read_message(buf, &sender);
        int rec_peers_size = 0;
        char *temp = check_msg_full(buf, req, msg_type, (void *)&rec_peers_size, &valread);
        if (temp)
        {
            free(buf);
            buf = temp;
        }

        buf[valread] = '\0';

        printf("\n");
        printf("\tResposta recebida: \"%.*s\"\n", (int)strcspn(buf, "\n"), buf);
        append_list_peers(buf, peers, peers_size, rec_peers_size /*, file*/);
        sock_close(req);
        free(buf);
        free(msg);
    }
}

// share the peers list with who requested
void share_peers_list(peer *server, pthread_mutex_t *clock_lock, SOCKET soc, peer sender, peer *peers, size_t peers_size)
{
    peer_list_args args;
    args.sender = sender;
    args.peers = peers;
    args.peers_size = peers_size;
    pthread_mutex_lock(clock_lock);
    server->p_clock++;
    pthread_mutex_unlock(clock_lock);
    printf("\t=> Atualizando relogio para %d\n", server->p_clock);
    char *msg = build_message(server->con, server->p_clock, PEER_LIST, (void *)&args);
    if (!msg)
    {
        return;
    }
    printf("\tEncaminhando mensagem \"%.*s\" para %s:%d\n", (int)strcspn(msg, "\n"), msg, inet_ntoa(sender.con.sin_addr), ntohs(sender.con.sin_port));
    send_complete(soc, msg, strlen(msg) + 1, 0);
    sock_close(soc);
    free(msg);
}

// asks for files of all peers
void get_files(peer *server, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size, char *dir_path, char ***files_list, size_t *files_len, int size_chunk)
{
    ls_files **files = malloc(sizeof(*files));
    *files = NULL;
    size_t files_list_len = 0;
    for (int i = 0; i < peers_size; i++)
    {
        if (peers[i].status == OFFLINE)
            continue;
        pthread_mutex_lock(clock_lock);
        server->p_clock++;
        pthread_mutex_unlock(clock_lock);
        printf("\t=> Atualizando relogio para %d\n", server->p_clock);
        char *msg = build_message(server->con, server->p_clock, LS, NULL);
        if (!msg)
        {
            perror("Erro: Falha na construcao da mensagem!\n");
            return;
        }
        SOCKET req = send_message(msg, &(peers[i]), LS);
        if (is_invalid_sock(req))
        {
            free(msg);
            continue;
        }
        char *buf = calloc(MSG_SIZE, sizeof(char));
        ssize_t valread = recv(req, buf, MSG_SIZE - 1, 0);
        if (valread <= 0)
        {
            fprintf(stderr, "\nErro: Falha lendo mensagem\n");
            free(buf);
            free(msg);
            continue;
        }
        peer sender;
        MSG_TYPE msg_type = read_message(buf, &sender);

        size_t rec_files_size = 0;
        char *temp = check_msg_full(buf, req, msg_type, (void *)&rec_files_size, &valread);
        if (temp)
        {
            free(buf);
            buf = temp;
        }
        buf[valread] = '\0';

        pthread_mutex_lock(clock_lock);
        server->p_clock = MAX(server->p_clock, sender.p_clock) + 1;
        pthread_mutex_unlock(clock_lock);
        if (rec_files_size == 0)
        {
            printf("\tResposta recebida: \"%.*s\"\n", (int)strcspn(buf, "\n"), buf);
            printf("\t=> Atualizando relogio para %d\n", server->p_clock);
            free(buf);
            free(msg);
            sock_close(req);
            continue;
        }

        printf("\tResposta recebida: \"%.*s\"\n", (int)strcspn(buf, "\n"), buf);
        printf("\t=> Atualizando relogio para %d\n", server->p_clock);
        printf("\n");
        append_files_list(buf, files, &files_list_len, sender, rec_files_size);
        free(buf);
        sock_close(req);
        free(msg);
    }
    uint16_t max_fname_size = 10;
    uint16_t max_fsize_size = 7;
    for (int i = 0; i < files_list_len; i++)
    {
        uint16_t s_f = strlen((*files)[i].file.fname);
        uint16_t s_s = (uint16_t)log10((*files)[i].file.fsize) + 1;
        if (s_f > max_fname_size)
            max_fname_size = s_f;
        if (s_s > max_fsize_size)
            max_fsize_size = s_s;
    }
    uint16_t count_size = floor(log10(files_list_len)) + 1;
    // print header
    printf("%*.s ", count_size + 2, "");
    printf("%-*s ", max_fname_size, "Nome");
    printf("|");
    printf(" %-*s ", max_fsize_size, "Tamanho");
    printf("|");
    printf(" %s\n", "Peer");
    // print cancel
    printf("[%0*d] ", count_size, 0);
    printf("%-*s ", max_fname_size, "<Cancelar>");
    printf("|");
    printf(" %*.s ", max_fsize_size, "");
    printf("|");
    printf("\n");
    // print list
    for (int i = 0; i < files_list_len; i++)
    {
        printf("[%0*d] ", count_size, i + 1);
        printf("%-*s", max_fname_size, (*files)[i].file.fname);
        printf(" | ");
        printf("%-*d", max_fsize_size, (int)(*files)[i].file.fsize);
        printf(" | ");
        printf("%s:%d", inet_ntoa((*files)[i].holders[0].con.sin_addr), ntohs((*files)[i].holders[0].con.sin_port));
        for(int j = 1; j < (*files)[i].peers_size; i++) {
            printf(", %s:%d ", inet_ntoa((*files)[i].holders[j].con.sin_addr), ntohs((*files)[i].holders[j].con.sin_port));
        }
        printf("\n");
    }
    printf("Digite o numero do arquivo para fazer o download:\n");
    int f_to_down = 0;
    printf(">");
    if (scanf("%d", &f_to_down) != 1)
    {
        while (getchar() != '\n')
            ;
        printf("Comando inesperado\n");
        f_to_down = 0;
    }
    if (!f_to_down)
        return;
    if (f_to_down > files_list_len)
        return;
    f_to_down--;
    
    
    ls_files chosen_file = (*files)[f_to_down];
    char *file_path = dir_file_path(dir_path, chosen_file.file.fname);
    FILE *new_file = fopen(file_path, "wb");
    if(!new_file) {
        fprintf(stderr, "Erro: Falha abrindo arquivo");
        return;
    }
    uint64_t round = 0;
    size_t written = 0;
    while(written < chosen_file.file.fsize) {
        for(int i = 0; i < chosen_file.peers_size; i++) {
            dl_msg_args *args = malloc(sizeof(dl_msg_args));
            args->fname = chosen_file.file.fname;
            args->chunk_size = size_chunk;
            args->offset = i + (round * chosen_file.peers_size);
            char *msg = build_message(server->con, server->p_clock, DL, (void *)args);
            free(args);
            if(!msg) return;
            SOCKET req = send_message(msg, &(chosen_file.holders[i]), DL);
            if(is_invalid_sock(req)) {
                fprintf(stderr, "\nErro: Falha com socket\n");
                free(msg);
                return;
            }
            free(msg);
            size_t temp_size = MSG_SIZE + 26 + base64encode_len(size_chunk);
            char *buf = calloc(temp_size + 1, sizeof(char));
            if(!buf) {
                fprintf(stderr, "\nErro: Falha na alocacao de recebimento!\n");
                sock_close(req);
                return;
            }
            ssize_t total_received = 0;
            while(total_received < temp_size) {
                ssize_t valread = recv(req, buf + total_received, temp_size - total_received, 0);
                if(valread < 0) {
                    fprintf(stderr, "\nErro: Falha no recebimento\n");
                    free(buf);
                    return;
                }
                if(!valread)
                    break;
                total_received += valread;
            }
            buf[total_received] = 0;

            printf("\n");
            printf("\nResposta recebida:  \"%.*s\"\n", (int)strcspn(buf, "\n"), buf);

            int sent_chunk, offset;
            char *file_b64 = get_file_in_msg(buf, NULL, &sent_chunk, &offset);
            if(!file_b64) {
                fprintf(stderr, "Erro: Falha no recebimento do arquivo\n");
                free(buf);
                return;
            }
            char *file_decoded = malloc(sizeof(char) * (size_chunk + 1));
            if(!file_decoded) {
                fprintf(stderr, "Erro: Falha na alocacao\n");
                free(buf);
                return;
            }
            int decode_size = base64_decode(file_decoded, file_b64);
            if (decode_size != chosen_file.file.fsize)
            {
                fprintf(stderr, "Erro: Arquivo com tamanho diferente que esperado");
                free(file_decoded);
                free(buf);
                return;
            }
            written += fwrite(file_decoded, 1, decode_size, new_file);
            free(file_decoded);
            free(buf);
        }
        round++;
    }
    char **temp = realloc(*files_list, sizeof(char *) * (*files_len + 1));
    if (!temp)
    {
        fprintf(stderr, "Erro: Falha na alocacao");
        free(file_path);
        fclose(new_file);
        return;
    }
    *files_list = temp;
    char *n_entry = strdup(chosen_file.file.fname);
    if (!n_entry)
    {
        fprintf(stderr, "Erro: Falha ao acrescentar novo arquivo à lista\n");
        for (int i = 0; i < files_list_len; i++)
            free((*files)[i].file.fname);
        free((*files));
        free(file_path);
        fclose(new_file);
        return;
    }
    (*files_list)[*files_len] = n_entry;
    (*files_len)++;
    for (int i = 0; i < files_list_len; i++)
        free((*files)[i].file.fname);
    free(*files);
    free(files);
    free(file_path);
    fclose(new_file);
}

// send list of files
void share_files_list(peer *server, pthread_mutex_t *clock_lock, SOCKET con, peer sender, char *dir_path)
{
    size_t files_len = 0;
    char **files = get_dir_files(dir_path, &files_len);
    if (!files)
    {
        lslist_msg_args *llargs = malloc(sizeof(lslist_msg_args));
        llargs->list_len = 0;
        char *msg = build_message(server->con, server->p_clock, LS_LIST, (void *)llargs);
        if (!msg)
        {
        }
        send_complete(con, msg, strlen(msg) + 1, 0);
        sock_close(con);
        printf("\tEncaminhando mensagem \"%.*s\" para %s:%d\n", (int)strcspn(msg, "\n"), msg, inet_ntoa(sender.con.sin_addr), ntohs(sender.con.sin_port));
        free(llargs);
        free(msg);
        return;
    }
    lslist_msg_args *llargs = malloc(sizeof(lslist_msg_args));
    if (!llargs)
    {
        fprintf(stderr, "Erro: Falha na alocacao de llargs\n");
        return;
    }
    llargs->list_file_len = malloc(sizeof(size_t) * files_len);
    if (!llargs->list_file_len)
    {
        fprintf(stderr, "Erro: Falha na alocacao de list_file_len\n");
        return;
    }
    llargs->list = files;
    for (size_t i = 0; i < files_len; i++)
    {
        char *file = dir_file_path(dir_path, files[i]);
        llargs->list_file_len[i] = fsize(file);
        free(file);
    }
    llargs->list_len = files_len;
    char *msg = build_message(server->con, server->p_clock, LS_LIST, (void *)llargs);
    if (msg)
        if (send_complete(con, msg, strlen(msg) + 1, 0) == 0)
            printf("\tEncaminhando mensagem \"%.*s\" para %s:%d\n", (int)strcspn(msg, "\n"), msg, inet_ntoa(sender.con.sin_addr), ntohs(sender.con.sin_port));
    sock_close(con);
    for (int i = 0; i < files_len; i++)
    {
        free(files[i]);
    }
    free(files);
    free(llargs->list_file_len);
    free(llargs);
    free(msg);
}

// send file
void send_file(peer *server, pthread_mutex_t *clock_lock, char *buf, SOCKET con, peer sender, char *dir_path)
{
    file_msg_args *fargs = malloc(sizeof(file_msg_args));
    if (!fargs)
        return;
    char *a = get_file_in_msg(buf, &(fargs->file_name), &(fargs->chunk_size), &(fargs->offset));
    if (a)
        free(a);
    char *file_path = dir_file_path(dir_path, fargs->file_name);
    if (!file_path)
    {
        free(fargs);
        return;
    }
    FILE *file = fopen(file_path, "rb");
    if (!file)
    {
        fprintf(stderr, "Erro: Falha abrindo arquivo para compartilhar\n");
        free(file_path);
        free(fargs);
        return;
    }
    fargs->file_size = fsize(file_path);
    char *file_cont = malloc(sizeof(char) * fargs->chunk_size);
    if (!file_cont)
    {
        fprintf(stderr, "Erro: Falha alocando conteudo do arquivo\n");
        fclose(file);
        free(file_path);
        free(fargs);
        return;
    }
    fseek(file, fargs->chunk_size * fargs->offset, SEEK_SET);
    fread(file_cont, fargs->chunk_size, 1, file);
    char *encoded = malloc(sizeof(char) * base64encode_len(fargs->chunk_size));
    if (!encoded)
    {
        fprintf(stderr, "Erro: Falha alocando arquivo codificado\n");
        fclose(file);
        free(file_path);
        free(fargs);
        free(file_cont);
        return;
    }
    base64_encode(encoded, file_cont, fargs->chunk_size);
    fargs->contentb64 = encoded;
    char *msg = build_message(server->con, server->p_clock, FILEMSG, (void *)fargs);
    if (msg)
    {
        printf("\tEncaminhando mensagem \"%.*s\" para %s:%d\n", (int)strcspn(msg, "\n"), msg, inet_ntoa(sender.con.sin_addr), ntohs(sender.con.sin_port));
        send_complete(con, msg, strlen(msg) + 1, 0);
    }
    sock_close(con);
    fclose(file);
    free(file_path);
    free(fargs->file_name);
    free(encoded);
    free(fargs);
    free(file_cont);
    free(msg);
}

// create a message with the sender ip, its clock and the message type
char *build_message(sockaddr_in sender_ip, int clock, MSG_TYPE msg_type, void *args)
{
    char *ip = inet_ntoa(sender_ip.sin_addr);
    u_short port = ntohs(sender_ip.sin_port);
    char *msg = malloc(sizeof(char) * MSG_SIZE);
    if (!msg)
    {
        fprintf(stderr, "Erro: Falha na alocacao de msg\n");
        return NULL;
    }
    if (msg_type < 0 || msg_type > BYE)
    {
        free(msg);
        msg = NULL;
        return msg;
    }
    char *temp;
    switch (msg_type)
    {
        /*
        MESSAGE FORMATING: <sender_ip>:<sender_port> <sender_clock> <MSG_TYPE> [<ARGS...>]\n <- important newline
        */
    case PEER_LIST:;
        peer_list_args *list_args = (peer_list_args *)args;
        peer sender = list_args->sender;
        size_t peers_size = list_args->peers_size;
        peer *peers = list_args->peers;
        temp = realloc(msg, sizeof(char) * msg_size_peer_list(peers_size));
        if (!temp)
        {
            fprintf(stderr, "Erro: Falha na alocacao da mensagem\n");
            return NULL;
        }
        msg = temp;
        sprintf(msg, "%s:%u %d PEER_LIST %d", ip, port, clock, (int)peers_size - 1);
        for (int i = 0; i < peers_size; i++)
        {
            if (is_same_peer(sender, peers[i]))
                continue;
            sprintf(msg, "%s %s:%d:%s:%d", msg, inet_ntoa(peers[i].con.sin_addr), ntohs(peers[i].con.sin_port), status_string[peers[i].status], peers[i].p_clock);
        }
        sprintf(msg, "%s\n", msg);
        break;
    case LS_LIST:;
        lslist_msg_args *llargs = (lslist_msg_args *)args;
        if (llargs->list_len == 0)
        {
            sprintf(msg, "%s:%u %d LS_LIST 0\n", ip, port, clock);
            return msg;
        }
        temp = realloc(msg, sizeof(char) * msg_size_files_list(llargs->list_len));
        if (!temp)
        {
            fprintf(stderr, "Erro: Falha na alocacao da mensagem!\n");
            return NULL;
        }
        msg = temp;
        sprintf(msg, "%s:%u %d LS_LIST %d", ip, port, clock, (int)llargs->list_len);
        for (int i = 0; i < llargs->list_len; i++)
        {
            sprintf(msg, "%s %s:%d", msg, llargs->list[i], (int)llargs->list_file_len[i]);
        }
        sprintf(msg, "%s\n", msg);
        break;
    case DL:;
        dl_msg_args *dargs = (dl_msg_args *)args;
        temp = realloc(msg, sizeof(char) * msg_size_files_list(1));
        if (!temp)
        {
            fprintf(stderr, "Erro: Falha na alocacao da mensagem!\n");
            return NULL;
        }
        msg = temp;
        sprintf(msg, "%s:%u %d DL %s %d %d\n", ip, port, clock, dargs->fname, dargs->chunk_size, dargs->offset);
        break;
    case FILEMSG:;
        file_msg_args *fargs = (file_msg_args *)args;
        temp = realloc(msg, sizeof(char) * (msg_size_files_list(1) + base64encode_len(fargs->file_size)));
        if (!temp)
        {
            fprintf(stderr, "Erro: Falha na alocacao da mensagem!\n");
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

// send full message
int send_complete(SOCKET sock, const void *buf, size_t len, int flag)
{
    size_t sent = 0;
    while (sent < len)
    {
        ssize_t n = send(sock, (char *)buf + sent, len - sent, flag);
        if (n <= 0)
            return -1;
        sent += n;
    }
    return 0;
}

// send a built message to an known peer socket
SOCKET send_message(const char *msg, peer *neighbour, MSG_TYPE msg_type)
{
    if (!msg)
    {
        fprintf(stderr, "\nErro: Mensagem de envio vazia!\n");
        return INVALID_SOCKET;
    }
    SOCKET server_soc = socket(AF_INET, SOCK_STREAM, 0);
    if (is_invalid_sock(server_soc))
    {
        fprintf(stderr, "\nErro: Falha na criacao do socket");
        show_soc_error();
        return INVALID_SOCKET;
    }
    if (connect(server_soc, (const struct sockaddr *)&(neighbour->con), sizeof(neighbour->con)) != 0)
    {
        fprintf(stderr, "\nErro: Falha ao conectar ao peer\n");
        show_soc_error();
        sock_close(server_soc);
        return INVALID_SOCKET;
    }
    size_t len = strlen(msg);
    printf("\tEncaminhando mensagem \"%.*s\" para %s:%d\n", (int)strcspn(msg, "\n"), msg, inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port));
    if (send_complete(server_soc, msg, len + 1, 0) == 0)
    {
        if (neighbour->status == OFFLINE)
        {
            neighbour->status = ONLINE;
            printf("\tAtualizando peer %s:%d status %s\n", inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port), status_string[1]);
        }
    }
    else
    {
        if (neighbour->status == ONLINE)
        {
            neighbour->status = OFFLINE;
            printf("\tAtualizando peer %s:%d status %s\n", inet_ntoa(neighbour->con.sin_addr), ntohs(neighbour->con.sin_port), status_string[0]);
        }
        show_soc_error();
    }
    return server_soc;
}

// read message, mark its sender and return the message type
MSG_TYPE read_message(const char *buf, peer *sender)
{
    char *buf_cpy = calloc(MSG_SIZE, sizeof(char));
    if (!buf_cpy)
        return UNEXPECTED_MSG_TYPE;
    strncpy(buf_cpy, buf, MSG_SIZE);
    char *tok_ip = strtok(buf_cpy, " ");
    int aclock = atoi(strtok(NULL, " "));
    char *tok_msg = strtok(NULL, " ");

    if (tok_msg)
    {
        tok_msg[strcspn(tok_msg, "\n")] = '\0';
    }
    create_address(sender, tok_ip, aclock);
    if (strcmp(tok_msg, "HELLO") == 0)
    {
        free(buf_cpy);
        return HELLO;
    }
    if (strcmp(tok_msg, "GET_PEERS") == 0)
    {
        free(buf_cpy);
        return GET_PEERS;
    }
    if (strcmp(tok_msg, "PEER_LIST") == 0)
    {
        free(buf_cpy);
        return PEER_LIST;
    }
    if (strcmp(tok_msg, "LS") == 0)
    {
        free(buf_cpy);
        return LS;
    }
    if (strcmp(tok_msg, "LS_LIST") == 0)
    {
        free(buf_cpy);
        return LS_LIST;
    }
    if (strcmp(tok_msg, "DL") == 0)
    {
        free(buf_cpy);
        return DL;
    }
    if (strcmp(tok_msg, "FILE") == 0)
    {
        free(buf_cpy);
        return FILEMSG;
    }
    if (strcmp(tok_msg, "BYE") == 0)
    {
        free(buf_cpy);
        return BYE;
    }
    return UNEXPECTED_MSG_TYPE;
}

// check if message received was read fully
char *check_msg_full(const char *buf, SOCKET sock, MSG_TYPE msg_type, void *args, ssize_t *valread)
{
    bool is_full_msg = false;
    if (*valread == MSG_SIZE - 1)
    {
        for (int i = 0; i < *valread; i++)
        {
            if (buf[i] == '\n')
            {
                is_full_msg = true;
                break;
            }
        }
        if (!is_full_msg)
        {
            switch (msg_type)
            {
            case DL:;
                char *aux_buf = calloc(msg_size_files_list(1), sizeof(char));
                if (!aux_buf)
                {
                    fprintf(stderr, "Erro: Falha na alocacao!\n");
                    return NULL;
                }
                sprintf(aux_buf, "%s", buf);
                *valread = recv(sock, &aux_buf[*valread], msg_size_files_list(1) - *valread, 0);
                return aux_buf;
                break;
            default:;
                int spaces = 0;
                char *duplicate = calloc(MSG_SIZE, sizeof(char));
                if (!duplicate)
                {
                    fprintf(stderr, "Erro: Falha na alocacao!\n");
                    return NULL;
                }
                int *rec_size = (int *)args;
                for (int i = 0; i < *valread; i++)
                {
                    if (buf[i] == ' ')
                    {
                        spaces++;
                        if (spaces == 3)
                        {
                            strncpy(duplicate, &buf[i + 1], 11);
                        }
                        if (spaces == 4 && duplicate)
                        {
                            duplicate[strcspn(duplicate, " ")] = '\0';
                            *rec_size = atoi(duplicate);
                            free(duplicate);
                            int new_size = 0;
                            if (msg_type == LS_LIST)
                                new_size = msg_size_files_list(*rec_size);
                            else
                                new_size = msg_size_peer_list(*rec_size);
                            char *aux_buf = calloc(new_size, sizeof(char));
                            if (!aux_buf)
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
    else if (args)
    {
        int spaces = 0;
        char *duplicate = calloc(MSG_SIZE, sizeof(char));
        if (!duplicate)
        {
            fprintf(stderr, "Erro: Falha na alocacao!\n");
            return NULL;
        }
        int *rec_size = (int *)args;
        for (int i = 0; i < *valread; i++)
        {
            if (buf[i] == ' ')
            {
                spaces++;
                if (spaces == 3)
                {
                    strncpy(duplicate, &buf[i + 1], 11);
                }
                if (spaces == 4 && duplicate)
                {
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

// append list received to known list
void append_files_list(const char *buf, ls_files **list, size_t *list_len, peer sender, size_t rec_files_len)
{
    char *cpy = strdup(buf);
    strtok(cpy, " ");  // ip
    strtok(NULL, " "); // clock
    strtok(NULL, " "); // type
    strtok(NULL, " "); // size
    char *list_rec = strtok(NULL, "\n");

    if (!list_rec)
    {
        fprintf(stderr, "Erro: Lista de arquivos nao encontrada!\n");
        free(cpy);
        return;
    }

    char *p = NULL;

    size_t info_count = 0;

    bool first_iteration = false;

    if (!*list)
    {
        first_iteration = true;
        *list_len += rec_files_len;
        (*list) = malloc(sizeof(ls_files) * (*list_len));
    }

    for (int i = 0; i < rec_files_len; i++, info_count++)
    {
        char *cpy_l = strdup(list_rec);
        for (int j = 0; j <= info_count; j++)
        {
            if (j == 0)
                p = strtok(cpy_l, " ");
            else
                p = strtok(NULL, " ");
        }
        char *infoname = strtok(p, ":");
        char *infosize = strtok(NULL, "\0");
        int position;

        if (first_iteration)
        {
            position = info_count;
            (*list)[position].file.fname = strdup(infoname);
            (*list)[position].file.fsize = atoi(infosize);
            (*list)[position].peers_size = 1;
        }
        else
        {
            char *fname = strdup(infoname);
            size_t fsize = atoi(infosize);
            int repeated_index;
            for (int j = 0; j < *list_len; j++)
            {
                if (strcmp(fname, (*list)[j].file.fname) == 0 && fsize == (*list)[j].file.fsize)
                {
                    repeated_index = j;
                    position = j;
                    break;
                }
            }
            if (!repeated_index)
            {
                *list_len += 1;
                ls_files *temp1 = realloc((*list), sizeof(ls_files) * (*list_len));
                if (!temp1)
                {
                    free(fname);
                    return;
                }
                (*list) = temp1;
                position = *list_len - 1;
                (*list)[position].file.fname = fname;
                (*list)[position].file.fsize = fsize;
                free(temp1);
            }
            (*list)[position].peers_size += 1;

            free(fname);
        }

        peer *tempH = realloc((*list)[position].holders, sizeof(peer) * ((*list)[position].peers_size));
        if (!tempH)
        {
            free(cpy_l);
            return;
        }
        (*list)[position].holders = tempH;
        (*list)[position].holders[(*list)[position].peers_size - 1] = sender;

        free(tempH);
        free(cpy_l);
    }

    free(cpy);
}

// return the file in base64 format
char *get_file_in_msg(char *buf, char **fname, int *chunk_size, int *offset)
{
    strtok(buf, " ");  // ip
    strtok(NULL, " "); // clock
    strtok(NULL, " "); // type
    char *infos = strtok(NULL, "\n");
    char *n = strtok(infos, " ");
    if (fname && n)
        *fname = strdup(n);
    *chunk_size = atoi(strtok(NULL, " "));
    *offset = atoi(strtok(NULL, " "));
    return strtok(NULL, "\n");
}

// change the local chunk_size
void change_chunk_size(int *chunk_size)
{
    printf("\nDigite novo tamanho de chunk:\n>");
    int input;
    if (scanf("%d", &input) != 1)
    {
        while (getchar() != '\n')
            ;
        fprintf(stderr, "Erro: Entrada invalida! Tente de novo.\n");
        return;
    }
    else if (input <= 0)
    {
        fprintf(stderr, "A entrada deve ser maior que 0! Tente de novo.\n");
        return;
    }
    else
    {
        *chunk_size = input;
    }
}

// send a bye message to every peer in list
void bye_peers(peer *server, pthread_mutex_t *clock_lock, peer *peers, size_t peers_size)
{
    for (int i = 0; i < peers_size; i++)
    {
        if (peers[i].status == ONLINE)
        {
            pthread_mutex_lock(clock_lock);
            server->p_clock++;
            pthread_mutex_unlock(clock_lock);
            printf("\t=> Atualizando relogio para %d\n", server->p_clock);
            char *msg = build_message(server->con, server->p_clock, BYE, NULL);
            if (!msg)
                continue;
            SOCKET s = send_message(msg, &peers[i], BYE);
            if (!is_invalid_sock(s))
                sock_close(s);
            free(msg);
        }
    }
}
