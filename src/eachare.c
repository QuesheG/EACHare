#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <file.h>
#include <message.h>
#include <peer.h>
#include <sock.h>
#include <threading.h>

bool should_quit = false;
pthread_mutex_t clock_lock;

void *treat_request(void *args) {
    peer server;
    peer **peers;
    size_t *peers_size;
    char *file;
    SOCKET n_sock;
    char *dir_path;
    get_args((listen_args *)args, &server, &peers, &peers_size, &file, &n_sock, &dir_path);

    char *buf = malloc(sizeof(char) * MSG_SIZE);

    ssize_t valread = recv(n_sock, buf, MSG_SIZE - 1, 0);

    if(valread <= 0 && !should_quit) {
        fprintf(stderr, "\nErro: Falha lendo mensagem de vizinho\n");
        return NULL;
    }

    peer sender;
    MSG_TYPE msg_type = read_message(buf, &sender);
    char *temp = check_msg_full(buf, n_sock, msg_type, NULL, &valread);
    if(temp) {
        free(buf);
        buf = temp;
    }

    buf[valread] = '\0';

    pthread_mutex_lock(&clock_lock);
    server.p_clock = max(server.p_clock, sender.p_clock);
    pthread_mutex_unlock(&clock_lock);
    
    printf("\n");
    printf("\tMensagem recebida: \"%.*s\"\n", (int)strcspn(buf, "\n"), buf);
    pthread_mutex_lock(&clock_lock);
    server.p_clock++;
    pthread_mutex_unlock(&clock_lock);
    printf("\t=> Atualizando relogio para %d\n", server.p_clock);
    int i = peer_in_list(sender, *peers, *peers_size);
    if(i < 0) {
        sender.status = ONLINE;
        int res = append_peer(peers, peers_size, sender, &i, file);
        if(res == -1) return NULL;
    }
    if((*peers)[i].status == OFFLINE && msg_type != BYE) {
        (*peers)[i].status = ONLINE;
        printf("\tAtualizando peer %s:%d status %s\n", inet_ntoa((*peers)[i].con.sin_addr), ntohs((*peers)[i].con.sin_port), status_string[1]);
        (*peers)[i].p_clock = max((*peers)[i].p_clock, sender.p_clock);
    }
    switch(msg_type) {
    case GET_PEERS:
        share_peers_list(&server, &clock_lock, n_sock, &(*peers)[i], *peers, *peers_size);
        break;
    case LS:
        share_files_list(&server, &clock_lock, n_sock, dir_path);
        break;
    case DL:
        send_file(&server, &clock_lock, buf, n_sock, dir_path);
        break;
    case BYE:
        if((*peers)[i].status == ONLINE) {
            (*peers)[i].status = OFFLINE;
            printf("\tAtualizando peer %s:%d status %s\n", inet_ntoa((*peers)[i].con.sin_addr), ntohs((*peers)[i].con.sin_port), status_string[0]);
        }
        break;
    default:
        break;
    }
    if(msg_type != PEER_LIST) printf(">");
    free(buf);
    sock_close(n_sock);
    pthread_exit(args);
    return NULL;
}

// routine for socket to listen to messages
void *listen_socket(void *args) {
    peer server;
    peer **peers;
    size_t *peers_size;
    char *file;
    SOCKET a;
    char *dir_path;
    get_args((listen_args *)args, &server, &peers, &peers_size, &file, &a, &dir_path);

    peer client;
    socklen_t addrlen = sizeof(server.con);

    int opt = 1;
    SOCKET server_soc;
    if(!create_server(&server_soc, server.con, opt)) {
        fprintf(stderr, "\nErro: Falha com criacao de server\n");
        show_soc_error();
        return NULL;
    }
    if(listen(server_soc, 3) != 0) {
        fprintf(stderr, "\nErro: Falha colocando socket para escutar\n");
        show_soc_error();
        return NULL;
    }
    while(!should_quit) {
        SOCKET n_sock = accept(server_soc, (struct sockaddr *)&(client.con), &addrlen);
        if(is_invalid_sock(n_sock) && !should_quit) {
            fprintf(stderr, "\nErro: Falha aceitando socket de conexao\n");
        }
        if(should_quit) return NULL;

        pthread_t response_thread;
        listen_args *l_args = send_args(server, peers, peers_size, file, n_sock, dir_path);
        pthread_create(&response_thread, NULL, treat_request, (void *)l_args);
    }
    sock_close(server_soc);
    pthread_exit(args);
    return NULL;
}

int main(int argc, char **argv)
{
    if(argc != 4) {
        fprintf(stderr, "Argumentos errados!\nEsperado: %s <endereco>:<porta> <vizinhos.txt> <diretorio_compartilhado>", argv[0]);
        return 1;
    }
#ifdef WIN
    if(init_win_sock()) {
        fprintf(stderr, "Erro: Falha criando socket Windows\n");
        show_soc_error();
        return 1;
    }
#endif

    if(pthread_mutex_init(&clock_lock, NULL) != 0) {
        perror("Erro: Falha ao inicializar o mutex");
        return EXIT_FAILURE;
    }

    peer server;
    peer **peers = malloc(sizeof(peer *));
    int comm;
    size_t *peers_size = malloc(sizeof(size_t)), files_len = 0;
    char **peers_txt, **files;
    pthread_t listener_thread;

    if(!peers || !peers_size) {
        fprintf(stderr, "Erro: falha na alocação de memoria\n");
        return 1;
    }

    create_address(&server, argv[1], 0);

    // read file to get neighbours
    FILE *f = fopen(argv[2], "r");
    if(!f) {
        fprintf(stderr, "Erro: Falha abrindo arquivo %s\n", argv[2]);
        perror(NULL);
        return 1;
    }
    peers_txt = read_peers(f, peers_size);
    *peers = create_peers((const char **)peers_txt, *peers_size);
    fclose(f);
    for(int i = 0; i < *peers_size; i++) {
        free(peers_txt[i]);
    }
    free(peers_txt);

    // read directory
    files = get_dir_files(argv[3], &files_len);
    
    listen_args *args = send_args(server, peers, peers_size, argv[2], 0, argv[3]);
    pthread_create(&listener_thread, NULL, listen_socket, (void *)args);

    while(!should_quit) {
        printf("Escolha um comando:\n"
            "\t[1] Listar peers\n"
            "\t[2] Obter peers\n"
            "\t[3] Listar arquivos locais\n"
            "\t[4] Buscar arquivos\n"
            "\t[5] Exibir estatisticas -> WIP\n"
            "\t[6] Alterar tamanho da chunk -> WIP\n"
            "\t[9] Sair\n");
        printf(">");
        if(scanf("%d", &comm) != 1) {
            while(getchar() != '\n');
            printf("Comando inesperado\n");
            continue;
        }
        switch(comm) {
        case 1:
            show_peers(&server, &clock_lock, *peers, *peers_size);
            break;
        case 2:
            get_peers(&server, &clock_lock, peers, peers_size, argv[2]);
            break;
        case 3:
            show_files((const char **)files, files_len);
            break;
        case 4:
            get_files(&server, &clock_lock, *peers, *peers_size, argv[3], files, &files_len);
            break;
        case 5:
            // show_statistics();
            break;
        case 6:
            // change_chunk_size();
            break;
        case 9:
            printf("Saindo...\n");
            should_quit = true;
            break;
        default:
            printf("Comando inesperado\n");
            break;
        }
    }

    pthread_mutex_destroy(&clock_lock);
    bye_peers(server, *peers, *peers_size);
    free(*peers);
    free(peers);
    free(peers_size);
    free(args);
    free(files);
#ifdef WIN
    WSACleanup();
#endif

    return 0;
}