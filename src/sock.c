#include <sock.h>
#include <str.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef WIN
int init_win_sock(void) {
    WSADATA d;
    int iResult = WSAStartup(MAKEWORD(2, 2), &d);
    if (iResult != 0) {
        fprintf(stderr, "Failed to initialize. %d\n", WSAGetLastError());
        return 1;
    }
    return iResult;
}
#endif

//standard function for socket creation
void sock_close(SOCKET sock) {
    #ifdef WIN
        closesocket(sock);
        return;
    #else
        close(sock);
    #endif
}

void create_address(sockaddr_in *address, char *ip) {
    char **addr = separator(ip, ':');
    address->sin_addr.s_addr = inet_addr(addr[0]);
    address->sin_family = AF_INET;
    address->sin_port = htons((unsigned short)(atoi(addr[1]))); //make string into int, then the int into short, then the short into a network byte order short
}

//create peers
peer * create_peers(char **peers_ip, int peers_size) {
    peer * peers = malloc(sizeof(peer)* peers_size);
    for (int i = 0; i < peers_size; i++) {
        create_address(&peers[i].con, peers_ip[i]);
        peers[i].status = OFFLINE;
        printf("Adding new peer %s status OFFLINE\n", peers_ip[i]);
    }
    return peers;
}

void show_peers(peer *peers, int peers_size) {
    printf("List peers:\n");
    printf("\t[0] Go back\n");
    for(int i = 0; i < peers_size; i++) {
        printf("\t[%d] %s:%d\n", i+1, inet_ntoa(peers[i].con.sin_addr), peers[i].con.sin_port);
    }
    int input;
    scanf("%d", &input);
    if(input == 0) return;
    //TODO: get input and act upon the chosen file
}