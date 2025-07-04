#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <list.h>
#include <file.h>
#include <message.h>
#include <peer.h>
#include <sock.h>
#include <threading.h>

bool should_quit = false;
pthread_mutex_t clock_lock;

void *treat_request(void *args)
{
    peer *server;
    ArrayList *peers;
    // char *file;
    SOCKET n_sock;
    char *dir_path;
    get_args((listen_args *)args, &server, &peers, /*&file,*/ &n_sock, &dir_path);

    char *buf = calloc(MSG_SIZE, sizeof(char));

    ssize_t valread = recv(n_sock, buf, MSG_SIZE - 1, 0);

    if(valread <= 0) {
        perror("Error: Failed reading neighbour message\n");
        free(args);
        sock_close(n_sock);
        free(buf);
        return NULL;
    }

    peer sender;
    MSG_TYPE msg_type = read_message(buf, &sender);
    if(msg_type == UNEXPECTED_MSG_TYPE) {
        fprintf(stderr, "Error: Unexpected neighbour message\n");
        free(buf);
        sock_close(n_sock);
        free(args);
        pthread_exit(NULL);
        return NULL;
    }
    char *temp = check_msg_full(buf, n_sock, msg_type, NULL, &valread);
    if (temp)
    {
        free(buf);
        buf = temp;
    }

    buf[valread] = '\0';
    printf("\n");
    printf("\tReceived message: \"%.*s\"\n", (int)strcspn(buf, "\n"), buf);
    update_clock(server, &clock_lock, sender.p_clock);

    int i = peer_in_list(sender, (peer *)peers->elements, peers->count);
    if (i < 0)
    {
        sender.status = ONLINE;
        int res = append_peer(peers, sender, &i);
        if (res == -1)
            return NULL;
    }
    if (((peer *)peers->elements)[i].status == OFFLINE && msg_type != BYE)
    {
        ((peer *)peers->elements)[i].status = ONLINE;
        printf("\tUpdating peer %s:%d status %s\n", inet_ntoa(sender.con.sin_addr), ntohs(sender.con.sin_port), status_string[1]);
        ((peer *)peers->elements)[i].p_clock = MAX(((peer *)peers->elements)[i].p_clock, sender.p_clock);
    }
    switch (msg_type)
    {
    case GET_PEERS:
        share_peers_list(server, &clock_lock, n_sock, sender, peers);
        break;
    case LS:
        share_files_list(server, &clock_lock, n_sock, sender, dir_path);
        break;
    case DL:
        send_file(server, &clock_lock, buf, n_sock, sender, dir_path);
        break;
    case BYE:
        if (sender.status == ONLINE)
        {
            ((peer *)peers->elements)[i].status = OFFLINE;
            printf("\tUpdating peer %s:%d status %s\n", inet_ntoa(sender.con.sin_addr), ntohs(sender.con.sin_port), status_string[0]);
        }
        break;
    default:
        break;
    }
    free(buf);
    sock_close(n_sock);
    free(args);
    printf(">");
    fflush(stdout);
    pthread_exit(NULL);
    return NULL;
}

// routine for socket to listen to messages
void *listen_socket(void *args)
{
    peer *server;
    ArrayList *peers;
    // char *file;
    SOCKET a;
    char *dir_path;
    get_args((listen_args *)args, &server, &peers, /*&file,*/ &a, &dir_path);

    peer client;
    socklen_t addrlen = sizeof(server->con);

    int opt = 1;
    SOCKET server_soc;
    if (!create_server(&server_soc, server->con, opt)) {
        fprintf(stderr, "\nError: Failed creating server\n");
        show_soc_error();
        return NULL;
    }
    set_sock_block(server_soc, false);
    if (listen(server_soc, 3) != 0) {
        fprintf(stderr, "\nError: Failed making server listen\n");
        show_soc_error();
        return NULL;
    }
    while (!should_quit) {
        SOCKET n_sock = accept(server_soc, (struct sockaddr *)&(client.con), &addrlen);
        if (is_invalid_sock(n_sock))
            continue;
        pthread_t response_thread;
        listen_args *l_args = send_args(server, peers, /*file,*/ n_sock, dir_path);
        pthread_create(&response_thread, NULL, treat_request, (void *)l_args);
        pthread_detach(response_thread);
    }
    sock_close(server_soc);
    free(args);
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "Wrong usage!\nExpected: %s <address>:<port> <neighbours.txt> <shared_directory>", argv[0]);
        return 1;
    }
#ifdef WIN
    if (init_win_sock())
    {
        fprintf(stderr, "Error: Failed creating Windows socket\n");
        show_soc_error();
        return 1;
    }
#endif

    if (pthread_mutex_init(&clock_lock, NULL) != 0)
    {
        perror("Error: Failed initiaing mutex");
        return EXIT_FAILURE;
    }

    peer *server = malloc(sizeof(peer));
    int comm, chunk_size = 256;
    ArrayList *peers = alloc_list(sizeof(peer));
    ArrayList *peers_txt = alloc_list(sizeof(char *));
    ArrayList *files = alloc_list(sizeof(char *));
    ArrayList *statistics = alloc_list(sizeof(stat_block));
    pthread_t listener_thread;

    if (!peers || !peers_txt || !files)
    {
        fprintf(stderr, "Error: Failed initial memory allocation\n");
        return 1;
    }

    //TODO: tratar argv[1~3]
    //argv[1] => checar se é <ip>:<porta>
    //argv[2] => tirar links a diretorios pais, checar se todas linhas sao <ip>:<porta>
    //argv[3] => tirar links a pais 

    create_address(server, argv[1], 0);

    // read file to get neighbours
    FILE *f = fopen(argv[2], "r");
    if (!f)
    {
        fprintf(stderr, "Error: Failed opening file %s\n", argv[2]);
        perror(NULL);
        return 1;
    }
    size_t txt_len = 0;
    char **peers_read = read_peers(f, &txt_len);
    append_many(peers_txt, peers_read, txt_len, sizeof(char *));
    free(peers_read);
    create_peers(peers, peers_txt);
    fclose(f);
    for (int i = 0; i < peers_txt->count; i++)
        free(((char **)peers_txt->elements)[i]);
    free_list(peers_txt);

    // read directory
    size_t files_len = 0;
    char **files_read = get_dir_files(argv[3], &files_len);
    append_many(files, files_read, files_len, sizeof(char *));
    free(files_read);

    listen_args *args = send_args(server, peers /*, argv[2]*/, 0, argv[3]);
    pthread_create(&listener_thread, NULL, listen_socket, (void *)args);

    while (!should_quit)
    {
        printf("Pick a command:\n"
               "\t[1] List peers\n"
               "\t[2] Get peers\n"
               "\t[3] List local files\n"
               "\t[4] Search files\n"
               "\t[5] Show statistics\n"
               "\t[6] Change chunk size\n"
               "\t[9] Quit\n");
        printf(">");
        if (scanf("%d", &comm) != 1)
        {
            while (getchar() != '\n')
                ;
            printf("Unexpected command\n");
            continue;
        }
        switch (comm)
        {
        case 1:
            show_peers(server, &clock_lock, peers);
            break;
        case 2:
            get_peers(server, &clock_lock, peers /*, argv[2]*/);
            break;
        case 3:
            show_files(files);
            break;
        case 4:
            get_files(server, &clock_lock, peers, argv[3], files, chunk_size, statistics);
            break;
        case 5:
            print_statistics(statistics);
            break;
        case 6:
            change_chunk_size(&chunk_size);
            printf("\tChanged chunk size: %d\n", chunk_size);
            break;
        case 9:
            printf("Quiting...\n");
            should_quit = true;
            pthread_cancel(listener_thread);
            pthread_join(listener_thread, NULL);
            break;
        default:
            printf("Unexpected command\n");
            break;
        }
    }

    bye_peers(server, &clock_lock, peers);
    if(args) free(args);
    pthread_mutex_destroy(&clock_lock);
    free(server);
    free_list(peers);
    for (int i = 0; i < files->count; i++)
        free(((char **)files->elements)[i]);
    free_list(files);
    for(int i = 0; i < statistics->count; i++)
        free_list(((stat_block*)statistics->elements)[i].times);
    free_list(statistics);
#ifdef WIN
    WSACleanup();
#endif

    return 0;
}
