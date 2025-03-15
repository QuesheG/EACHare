#include <stdio.h>
#include <sock.h>
#include <file.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
#include <pthread.h>

bool should_quit = false;

typedef struct listen_args {
    SOCKET server_soc;
    peer server;
    peer *neighbours;
    size_t peers_size;
    int *clock;
}listen_args;

//routine for socket to listen to messages
//TODO: definir semaforos para clock
void * listen_socket(void *args) {
    SOCKET server_soc = ((listen_args*)args)->server_soc;
    peer server = ((listen_args*)args)->server;
    peer *peers = ((listen_args*)args)->neighbours;
    size_t peers_size = ((listen_args*)args)->peers_size;
    int clock = *((listen_args*)args)->clock;
    while(!should_quit) {
        if(listen(server_soc, 3) != 0) {
            printf("Error listening in socket\n");
            continue;
        }
        socklen_t addrlen = sizeof(server.con);
    
        SOCKET n_sock = accept(server_soc, (struct sockaddr *)&server.con, &addrlen);
        if(is_invalid_sock(n_sock)) {
            printf("Error creating new socket\n");
        }
    
        char buf[MSG_SIZE] = {0};
    
        ssize_t valread = recv(n_sock, buf, MSG_SIZE - 1, 0);
    
        if(valread <= 0) {
            printf("Error reading from client\n");
            continue;
        }
    
        buf[valread] = '\0';
        printf("Mensagem recebida : \"%s\"", buf);
        peer sender;
        MSG_TYPE msg_type = read_message(server, buf, &clock, &sender);
        switch(msg_type) {
            case HELLO:
                sender.status = ONLINE;
                if(peer_in_list(sender, peers, peers_size)) {
                    //append sender to known neighbours;
                    //update list in file
                }
                break;
            case BYE:
                sender.status = OFFLINE;
            default:
                break;
        }
    }
    pthread_exit(args);
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        printf("Wrong arguments!\nExpected: %s <ip>:<port> <neighbours.txt> <shareable_dir>", argv[0]);
        return 1;
    }
    #ifdef WIN
    if (init_win_sock()) return 1;
    #endif


    SOCKET server_soc;
    peer server;
    peer *peers;
    int opt = 1, loc_clock = 0, comm;
    size_t peers_size = 0, files_len = 0;
    char **peers_txt, **files;
    pthread_t listener_thread;


    // create serve
    create_address(&server, argv[1]);
    if(!create_server(&server_soc, server.con, opt)) return 1;
    

    // read file to get neighbours
    FILE *f = fopen(argv[2], "r");
    peers_txt = read_peers(f, &peers_size);
    peers = create_peers(peers_txt, peers_size);
    fclose(f);
    for (int i = 0; i < peers_size; i++) {
        free(peers_txt[i]);
    }
    free(peers_txt);


    // read directory
    DIR *shared_dir = opendir(argv[3]);
    if (!shared_dir) {
        printf("Directory not found, try check spelling or existance of said directory\n");
        return 1;
    }
    files = get_dir_files(shared_dir, &files_len);
    closedir(shared_dir);

    listen_args *args = malloc(sizeof(listen_args));
    args->server_soc = server_soc;
    args->server = server;
    args->neighbours = peers;
    args->peers_size = peers_size;
    args->clock = &loc_clock;
    pthread_create(&listener_thread, NULL, listen_socket, (void *)args);
    
    while (!should_quit) {
        printf("Choose a command:\n"
            "\t[1] List peers\n"
            "\t[2] Get peers -> WIP\n"
            "\t[3] List local files\n"
            "\t[4] Get files -> WIP\n"
            "\t[5] Exhibit statistics -> WIP\n"
            "\t[6] Change chuck size -> WIP\n"
            "\t[9] Exit\n"
            );
        scanf("%d", &comm);
        switch (comm)
        {
        case 1:
            show_peers(server, server_soc, &loc_clock, peers, peers_size);
            break;
        case 3:
            show_files(files, files_len);
            break;
        case 5:
            // show_statistics();
            break;
        case 9:
            printf("Exiting...\n");
            // TODO: msg peers
            should_quit = true;
            break;
        default:
            printf("Unexpected command\n");
            break;
        }
    }

    sock_close(server_soc);

    bye_peers(server, server_soc, &loc_clock, peers, peers_size);
    free(peers);
    free(args);
    free(files);
    #ifdef WIN
    WSACleanup();
    #endif

    return 0;
}