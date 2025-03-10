#include <file.h>
#include <str.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <stdbool.h>

//read file with peers' ips and return them in text form
char ** read_peers(FILE *f, int *len) {
    char ** peers = NULL;
    *len = 0;
    char ip[23] = {0}; //max ip size = 255.255.255.255:65535 -> 3+1+3+1+3+1+3+1+5 = 21 + possible newline + null terminator => 23
    while(true) {
        //read each line of file and put it in peers
        if(!fgets(ip, sizeof(ip), f)) break;
        char **temp= realloc(peers, sizeof(char*) * (*len + 1));
        if(!temp) {
            for(int i = 0; i < *len; i++) {
                free(peers[i]);
            }
            free(peers);
            return NULL;
        }
        peers = temp;
        int ip_len = size_str(ip);
        if(ip_len > 0 && ip[ip_len - 2] == '\n') {
            ip[ip_len - 2] = '\0';
            ip_len -= 1;
        }
        peers[*len] = malloc(sizeof(char) * ip_len);
        if(!peers[*len]) {
            // Handle memory allocation failure
            for (int i = 0; i < *len; i++) {
                free(peers[i]);
            }
            free(peers);
            return NULL;
        }
        cpy_str(peers[*len], ip, ip_len);
        *len += 1;
    }
    return peers;
}

//store name of every file in directory
char ** get_dir_files(DIR *dir, int *len) {
    char ** files = NULL;
    *len = 0;
    while(true) {
        dirent * dir_ent = readdir(dir);
        if(!dir_ent) break;
        if(dir_ent->d_name[0] == '.') continue;
        printf("%s\n", dir_ent->d_name);
        char **temp = realloc(files, sizeof(char*) * (*len + 1));
        if(!temp) {
            for(int i = 0; i < *len; i++) {
                free(files[i]);
            }
            free(files);
            return NULL;
        }
        files = temp;
        int d_len = size_str(dir_ent->d_name);
        files[*len] = malloc(sizeof(char) * d_len);
        cpy_str(files[*len], dir_ent->d_name, d_len);
        *len += 1;
        free(dir_ent);
    }
    return files;
}

void show_files(char **files, int file_len) {
    printf("List files:\n");
    printf("\t[0] Go back\n");
    for(int i = 0; i < file_len; i++) {
        printf("\t[%d] %s\n", i+1, files[i]);
    }
    int input;
    scanf("%d", &input);
    if(input == 0) return;
    //TODO: get input and act upon the chosen file
}