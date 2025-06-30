#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sock.h>
#include <peer.h>

char *status_string[] = {"OFFLINE", "ONLINE"};
// TODO: make peer func && print peer func

// compare two peers
bool is_same_peer(peer a, peer b)
{
    return (strcmp(inet_ntoa(a.con.sin_addr), inet_ntoa(b.con.sin_addr)) == 0 && (a.con.sin_port == b.con.sin_port));
}

// create an IPv4 sockaddr_in
void create_address(peer *address, const char *ip, uint64_t clock)
{
    char *ip_cpy = strdup(ip);
    char *svptr = ip_cpy;
    char *tokip = strtok_r(svptr, ":", &svptr);
    char *tokport = strtok_r(NULL, ":", &svptr);
    if(!tokip || !tokport) {
        free(ip_cpy);
        return;
    }
    address->con.sin_addr.s_addr = inet_addr(tokip);
    address->con.sin_family = AF_INET;
    address->con.sin_port = htons((unsigned short)(atoi(tokport))); // make string into int, then the int into short, then the short into a network byte order short
    address->status = OFFLINE;
    address->p_clock = clock;
    free(ip_cpy);
}

// bind a server to the socket
bool create_server(SOCKET *server, sockaddr_in address, int opt)
{
    *server = socket(AF_INET, SOCK_STREAM, 0);
    if (is_invalid_sock(*server))
    {
        fprintf(stderr, "Error: Failed upon socket creation\n");
        return false;
    }
    if (setsockopt(*server, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) != 0)
    {
        fprintf(stderr, "Error: Failed making server\n");
        show_soc_error();
        return false;
    }
    if (bind(*server, (const struct sockaddr *)&address, sizeof(address)) != 0)
    {
        fprintf(stderr, "Error: Failed connecting server\n");
        show_soc_error();
        return false;
    }
    return true;
}

// create peers
void create_peers(ArrayList *peers, ArrayList *peers_txt)
{
    for (int i = 0; i < peers_txt->count; i++)
    {
        printf("Adding new peer %s status %s\n", ((char **)peers_txt->elements)[i], status_string[0]);
        peer np = {0};
        create_address(&np, ((char **)peers_txt->elements)[i], 0);
        append_element(peers, (void *)&np);
    }
}

// append new peer
int append_peer(ArrayList *peers, peer new_peer, int *i /*, char *file*/)
{
    append_element(peers, (void *)&new_peer);
    (*i) = peers->count - 1;
    // FILE *f = fopen(file, "a");
    // fprintf(f, "%s:%d\n", inet_ntoa((*peers)[(*i)].con.sin_addr), ntohs((*peers)[(*i)].con.sin_port));
    // fclose(f);
    printf("\tAdding new peer %s:%d status %s\n", inet_ntoa(((peer *)peers->elements)[*i].con.sin_addr), ntohs(((peer *)peers->elements)[*i].con.sin_port), status_string[((peer *)peers->elements)[*i].status]);

    return 1;
}

// check if peer is in list of known peers
int peer_in_list(peer a, peer *neighbours, size_t peers_size)
{
    for (int i = 0; i < peers_size; i++)
    {
        if (is_same_peer(a, neighbours[i]))
            return i;
    }
    return -1;
}

// append received list to known peer list
void append_list_peers(const char *buf, ArrayList *peers, size_t rec_peers_size /*, char *file*/) {
    char *cpy = strdup(buf);
    char *svptr = cpy;
    strtok_r(svptr, " ", &svptr);  // ip
    strtok_r(NULL, " ", &svptr); // clock
    strtok_r(NULL, " ", &svptr); // type
    strtok_r(NULL, " ", &svptr); // size
    char *list = strtok_r(NULL, "\n", &svptr);

    if(!list) {
        fprintf(stderr, "Error: Empty peer list\n");
        free(cpy);
        return;
    }

    char *p = NULL;
    peer *rec_peers_list = malloc(sizeof(peer) * rec_peers_size);

    if(!rec_peers_list) {
        fprintf(stderr, "Error: Failed allocating received peer list");
        free(cpy);
        return;
    }
    size_t info_count = 0;
    for(int i = 0; i < rec_peers_size; i++, info_count++) {
        char *cpy_l = strdup(list);
        svptr = cpy_l;
        for(int j = 0; j <= info_count; j++)
            if(j == 0)
                p = strtok_r(svptr, " ", &svptr);
            else
                p = strtok_r(NULL, " ", &svptr);
        svptr = p;
        char *infon = strtok_r(svptr, ":", &svptr);
        if(!infon) break;
        for(int j = 0; j < 4; j++) {// hardcoding infos size :D => ip1:port2:status3:num4
            if(!infon) break;
            if(j == 0)
                rec_peers_list[i].con.sin_addr.s_addr = inet_addr(infon);
            if(j == 1)
                rec_peers_list[i].con.sin_port = htons((unsigned short)atoi(infon));
            if(j == 2) {
                if (strcmp(infon, "ONLINE") == 0)
                    rec_peers_list[i].status = ONLINE;
                else
                    rec_peers_list[i].status = OFFLINE;
            }
            if (j == 3)
                rec_peers_list[i].p_clock = atoi(infon);
            infon = strtok_r(NULL, ":", &svptr);
        }

        bool add = true;
        for(int j = 0; j < peers->count; j++) {
            if(is_same_peer(rec_peers_list[i], ((peer *)peers->elements)[j])) {
                add = false;
                break;
            }
        }
        if(add) {
            int j = 0;
            rec_peers_list[i].con.sin_family = AF_INET;
            int res = append_peer(peers, rec_peers_list[i], &j /*, file*/);
            if (res == -1)
                free(rec_peers_list);
        }
        free(cpy_l);
    }

    free(cpy);
    free(rec_peers_list);
}

// update clock
void update_clock(peer *a, pthread_mutex_t *clock_lock, uint64_t n_clock)
{
    pthread_mutex_lock(clock_lock);
    a->p_clock = MAX(a->p_clock, n_clock) + 1;
    pthread_mutex_unlock(clock_lock);
    printf("\t=> Updating clock to %" PRIu64 "\n", a->p_clock);
}

#ifdef WIN
/* 
 * public domain strtok_r() by Charlie Gordon
 *
 *   from comp.lang.c  9/14/2007
 *
 *      http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 *
 *     (Declaration that it's public domain):
 *      http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 */

char* strtok_r(
    char *str, 
    const char *delim, 
    char **nextp)
{
    char *ret;

    if (str == NULL)
    {
        str = *nextp;
    }

    str += strspn(str, delim);

    if (*str == '\0')
    {
        return NULL;
    }

    ret = str;

    str += strcspn(str, delim);

    if (*str)
    {
        *str++ = '\0';
    }

    *nextp = str;

    return ret;
}
#endif
