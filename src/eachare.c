#include <stdio.h>
#include <sock.h>
#include <file.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
#include <pthread.h>

/*
    TODO:
        Att do clock a cada mensagem
    FIXME:
        Msg de bye
*/

bool should_quit = false;

typedef struct listen_args {
    peer server;
    peer *neighbours;
    size_t peers_size;
    int *clock;
}listen_args;

//routine for socket to listen to messages
//TODO: definir semaforos para clock e should quit
void * listen_socket(void *args) {
    peer server = ((listen_args*)args)->server;
    peer *peers = ((listen_args*)args)->neighbours;
    size_t peers_size = ((listen_args*)args)->peers_size;
    int clock = *((listen_args*)args)->clock;

    int opt = 1;
    SOCKET server_soc;
    if(!create_server(&server_soc, server.con, opt)) {
        printf("Error creating server\n");
        return NULL;
    }
    while(!should_quit) {
        if(listen(server_soc, 3) != 0) {
            printf("Error listening in socket\n");
            continue;
        }
        socklen_t addrlen = sizeof(server.con);
    
        SOCKET n_sock = accept(server_soc, (struct sockaddr *)&server.con, &addrlen);
        if(is_invalid_sock(n_sock) && !should_quit) {
            printf("Error creating new socket\n");
        }
    
        char buf[MSG_SIZE] = {0};
    
        ssize_t valread = recv(n_sock, buf, MSG_SIZE - 1, 0);
    
        if(valread <= 0 && !should_quit) {
            printf("Error reading from neighbour\n");
            continue;
        }
    
        buf[valread] = '\0';
        printf("Mensagem recebida : \"%s\"", buf);
        peer sender;
        MSG_TYPE msg_type = read_message(server, buf, &clock, &sender);
        int i = peer_in_list(sender, peers, peers_size);
        switch(msg_type) {
            case HELLO:
                if(i >= 0) {
                    peers[i].status = ONLINE;
                    //TODO: append sender to known neighbours;
                    //update list in file
                }
                break;
            case BYE:
                peers[i].status = OFFLINE;
            default:
                break;
        }
    }
    sock_close(server_soc);
    pthread_exit(args);
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        printf("Wrong arguments!\nExpected: %s <ip>:<port> <neighbours.txt> <shareable_dir>", argv[0]);
        return 1;
    }
    #ifdef WIN
    if (init_win_sock()) {
        printf("Error creating Windows socket\n");
        return 1;
    }
    #endif

    
    peer server;
    peer *peers;
    int loc_clock = 0, comm;
    size_t peers_size = 0, files_len = 0;
    char **peers_txt, **files;
    pthread_t listener_thread;


    create_address(&server, argv[1]);


    // read file to get neighbours
    FILE *f = fopen(argv[2], "r");
    if(!f) {
        printf("Error opening file %s\n", argv[2]);
        return 1;
    }
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
        if(scanf("%d", &comm) != 1) {
            while(getchar() != '\n');
            printf("Unexpected command\n");
            continue;
        }
        switch (comm)
        {
        case 1:
            show_peers(server, &loc_clock, peers, peers_size);
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


    bye_peers(server, &loc_clock, peers, peers_size);
    free(peers);
    free(args);
    free(files);
    #ifdef WIN
    WSACleanup();
    #endif

    return 0;
}