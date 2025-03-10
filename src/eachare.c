#include <stdio.h>
#include <sock.h>
#include <str.h>
#include <file.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
// #include <pthread.h>

#define SIZE 1024

int main(int argc, char **argv)
{
    if (argc != 4) {
        printf("Wrong arguments!\nExpected: %s <ip>:<port> <neighbours.txt> <shareable_dir>", argv[0]);
        return 1;
    }
    #ifdef WIN
    if (init_win_sock()) return 1;
    #endif

    SOCKET server, n_sock;
    int opt = 1, peers_size = 0, files_len = 0, loc_clock = 0, comm;
    sockaddr_in address;
    socklen_t addrlen;
    char buf[SIZE] = {0};
    ssize_t valread;
    char **peers_txt, **files;
    peer *peers;
    bool should_quit = false;

    // create server
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (is_invalid_sock(server)) {
        printf("Error upon socket creation.\n");
        return 1;
    }
    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) != 0) {
        printf("Error creating server");
        return 1;
    }
    create_address(&address, argv[1]);
    if (bind(server, (const struct sockaddr *)&address, sizeof(address)) != 0) {
        printf("Error binding server");
        return 1;
    }

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

    // TODO: paralelizar o "escutamento" do socket e a checagem de comandos
    //  if(listen(server, 3) != 0) {
    //      printf("Error listening in socket\n");
    //      return 1;
    //  }

    // addrlen = sizeof(address);

    // n_sock = accept(server, (struct sockaddr *)&address, &addrlen);
    // if(is_invalid_sock(n_sock)) {
    //     printf("Error creating new socket\n");
    // }

    // valread = recv(n_sock, buf, SIZE - 1, 0);

    // if(valread <= 0) {
    //     printf("Error reading from client\n");
    //     return 1;
    // }

    // buf[valread] = '\0';
    // printf("%s\n", buf);
    ///////////////////////////////////////////////////////////////////////

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
            show_peers(peers, peers_size);
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

    sock_close(server);

    #ifdef WIN
    WSACleanup();
    #endif

    return 0;
}