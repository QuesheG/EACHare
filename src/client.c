#include <stdio.h>
#include <sock.h>
#include <util.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if(argc != 2) { //FIXME: MUDAR AQUI PARA COMPRIMENTO CERTO;
        printf("Wrong arguments!\nExpected: %s <ip>", argv[0]); //FIXME: CONSERTAR COM ARGUMENTOS CERTOS
        return 1;
    }
    #ifdef WIN
    if(init_win_sock()) return 1;
    #endif

    SOCKET client;
    sockaddr_in server;
    char msg[] = "Fala cara!\n";

    client = socket(AF_INET, SOCK_STREAM, 0);
    if(is_invalid_sock(client)) {
        printf("Error upon socket creation.\n");
        return 1;
    }
    
    char ** addr = separator(argv[1], ':');
    server.sin_addr.s_addr = inet_addr(addr[0]);
    server.sin_family = AF_INET;
    server.sin_port = htons((unsigned short)(atoi(addr[1]))); //make string into int, then the int into short, then the short into a network byte order short

    if(connect(client, (const struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Error establishing connection.\n");
        return 1;
    }

    printf("Connection established!\n");


    send(client, msg, sizeof(msg), 0);
    printf("Message sent\n");

    sock_close(client);
    
    #ifdef WIN
    WSACleanup();
    #endif

    return 0;
}