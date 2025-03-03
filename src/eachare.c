#include <stdio.h>
#include <sock.h>
#include <util.h>
#include <stdlib.h>
#include <sys/types.h>
// #include <pthread.h>

#define SIZE 1024

int main(int argc, char **argv) {
    if(argc != 4) { //FIXME: MUDAR AQUI PARA COMPRIMENTO CERTO;
        printf("Wrong arguments!\nExpected: %s <ip>:<port> <neighbours.txt> <shareable_dir>", argv[0]);
        return 1;
    }
    #ifdef WIN
    if(init_win_sock()) return 1;
    #endif

    SOCKET server, n_sock;
    int opt = 1;
    sockaddr_in address;
    socklen_t addrlen;
    char buf[SIZE] = {0};
    ssize_t valread;
    char **addr;

    server = socket(AF_INET, SOCK_STREAM, 0);
    if(is_invalid_sock(server)) {
        printf("Error upon socket creation.\n");
        return 1;
    }

    if(setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        printf("Error creating server");
        return 1;
    }

    addr = separator(argv[1], ':');
    address.sin_addr.s_addr = inet_addr(addr[0]);
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)(atoi(addr[1]))); //make string into int, then the int into short, then the short into a network byte order short
    
    if(bind(server, (const struct sockaddr *)&address, sizeof(address)) != 0) {
        printf("Error binding server");
        return 1;
    }

    printf("Server created!\n");

    //read file to get neighbours
    int peers_size = 0;
    char **peers_txt = read_peers(argv[2], &peers_size);
    peer *peers = create_peers(peers_txt, peers_size);

    if(listen(server, 3) != 0) {
        printf("Error listening in socket\n");
        return 1;
    }

    addrlen = sizeof(address);

    n_sock = accept(server, (struct sockaddr *)&address, &addrlen);
    if(is_invalid_sock(n_sock)) {
        printf("Error creating new socket\n");
    }

    valread = recv(n_sock, buf, SIZE - 1, 0);

    if(valread <= 0) {
        printf("Error reading from client\n");
        return 1;
    }

    buf[valread] = '\0';
    
    printf("%s\n", buf);

    sock_close(server);

    // while(true) {

    // }

    #ifdef WIN
    WSACleanup();
    #endif

    return 0;
}