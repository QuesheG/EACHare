#include <stdio.h>
#include <file.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
#include <pthread.h>

#include <sock.h>
#include <peer.h>
#include <message.h>

bool should_quit = false;
pthread_mutex_t clock_lock;

typedef struct listen_args
{
    peer server;
    peer **neighbours;
    size_t *peers_size;
    int *clock;
    char *file;
    DIR *dir;
} listen_args;

// routine for socket to listen to messages
void *listen_socket(void *args)
{
    listen_args *list_args = (listen_args *)args;
    peer server = list_args->server;
    peer **peers = list_args->neighbours;
    size_t *peers_size = list_args->peers_size;
    int *clock = list_args->clock;
    char *file = list_args->file;
    // DIR *dir = list_args->dir;
    socklen_t addrlen = sizeof(server.con);
    
    int opt = 1;
    SOCKET server_soc;
    if(!create_server(&server_soc, server.con, opt)) {
        fprintf(stderr, "Erro: Falha com criacao de server\n");
        show_soc_error();
        return NULL;
    }
    if(listen(server_soc, 3) != 0) {
        fprintf(stderr, "Erro: Falha colocando socket para escutar\n");
        show_soc_error();
        return NULL;
    }
    while(!should_quit) {
        // TODO: - criar connectingpeer;
        //       - ligar ele em accept;
        //       - verificar se ip dele bate com mensagem;
        //       - procurar ele na lista;
        //       - criar ele como peer;
        // accept() não leva o endereço do server, e sim o endereço que o cliente se conectando tem!
        SOCKET n_sock = accept(server_soc, (struct sockaddr *)&(server.con), &addrlen); // FIXME: change server to client
        if(is_invalid_sock(n_sock) && !should_quit) {
            fprintf(stderr, "Erro: Falha aceitando socket de conexao\n");
        }
        if(should_quit) return NULL;
        
        // TODO: criar threads
        
        char *buf = malloc(sizeof(char) * MSG_SIZE);
        
        ssize_t valread = recv(n_sock, buf, MSG_SIZE - 1, 0);
        
        if(valread <= 0 && !should_quit) {
            fprintf(stderr, "Erro: Falha lendo mensagem de vizinho\n");
            continue;
        }
        
        int rec_peers_size = 0;
        char *temp = check_msg_full(buf, n_sock, &rec_peers_size, &valread);
        if(temp) {
            buf = temp;
        }

        buf[valread] = '\0';
        printf("Mensagem recebida: \"%.*s\"\n", (int)strcspn(buf, "\n"), buf);
        pthread_mutex_lock(&clock_lock);
        (*clock)++;
        pthread_mutex_unlock(&clock_lock);
        printf("\t=> Atualizando relogio para %d\n", *clock);

        peer sender;
        MSG_TYPE msg_type = read_message(buf, clock, &sender);
        int i = peer_in_list(sender, *peers, *peers_size);
        if(i < 0) {
            FILE *f = fopen(file, "a");
            append_peer(peers, peers_size, sender);
            i = *peers_size - 1;
            fprintf(f, "%s:%d\n", inet_ntoa((*peers)[i].con.sin_addr), ntohs((*peers)[i].con.sin_port));
            fclose(f);
        }
        switch(msg_type) {
        case HELLO:
            if((*peers)[i].status == OFFLINE) {
                (*peers)[i].status = ONLINE;
                printf("Atualizando peer %s:%d para status ONLINE\n", inet_ntoa((*peers)[i].con.sin_addr), ntohs((*peers)[i].con.sin_port));
            }
            break;
        case GET_PEERS:
            if((*peers)[i].status == OFFLINE) {
                (*peers)[i].status = ONLINE;
                printf("Atualizando peer %s:%d para status ONLINE\n", inet_ntoa((*peers)[i].con.sin_addr), ntohs((*peers)[i].con.sin_port));
            }
            share_peers_list(server, clock, &clock_lock, &(*peers)[i], *peers, *peers_size);
            break;
        case PEER_LIST:
            append_list_peers(buf, peers, peers_size, rec_peers_size);
            break;
        case BYE:
            (*peers)[i].status = OFFLINE;
            printf("Atualizando peer %s:%d para status OFFLINE\n", inet_ntoa((*peers)[i].con.sin_addr), ntohs((*peers)[i].con.sin_port));
        default:
            printf("Tipo de mensagem nao reconhecido\n");
            break;
        }
        sock_close(n_sock);
    }
    sock_close(server_soc);
    pthread_exit(args);
    return NULL;
}

void treat_request(void *args) {
    
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
    int loc_clock = 0, comm;
    size_t *peers_size = malloc(sizeof(size_t)), files_len = 0;
    char **peers_txt, **files;
    pthread_t listener_thread;

    if(!peers || !peers_size) {
        fprintf(stderr, "Erro: falha na alocação de memoria\n");
        return 1;
    }

    create_address(&server, argv[1]);

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
    DIR *shared_dir = opendir(argv[3]);
    if(!shared_dir) {
        perror("Erro: Diretorio nao achado, cheque escrita ou existencia do diretorio");
        return 1;
    }
    files = get_dir_files(shared_dir, &files_len);

    listen_args *args = malloc(sizeof(listen_args));
    args->server = server;
    args->neighbours = peers;
    args->peers_size = peers_size;
    args->clock = &loc_clock;
    args->file = argv[2];
    args->dir = shared_dir;
    pthread_create(&listener_thread, NULL, listen_socket, (void *)args);

    while(!should_quit) {
        printf("Escolha um comando:\n"
            "\t[1] Listar peers\n"
            "\t[2] Obter peers\n"
            "\t[3] Listar arquivos locais\n"
            "\t[4] Buscar arquivos -> WIP\n"
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
            show_peers(server, &loc_clock, &clock_lock, *peers, *peers_size);
            break;
        case 2:
            get_peers(server, &loc_clock, &clock_lock, *peers, *peers_size);
            break;
        case 3:
            show_files(files, files_len);
            break;
        case 4:
            //get_files();
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
    bye_peers(server, &loc_clock, *peers, *peers_size);
    free(*peers);
    free(peers);
    free(args);
    free(files);
    closedir(shared_dir);
#ifdef WIN
    WSACleanup();
#endif

    return 0;
}