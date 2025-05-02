#include <file.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

//read file with peers' ips and return them in text form
char **read_peers(FILE *f, size_t *len) {
    char **peers = NULL;
    *len = 0;
    char ip[23] = { 0 }; //max ip size = 255.255.255.255:65535 -> 3+1+3+1+3+1+3+1+5 = 21 + possible newline + null terminator => 23
    while(true) {
        //read each line of file and put it in peers
        if(!fgets(ip, sizeof(ip), f)) break;
        char **temp = realloc(peers, sizeof(char *) * (*len + 1));
        if(!temp) {
            for(int i = 0; i < *len; i++) {
                free(peers[i]);
            }
            free(peers);
            *len = 0;
            return NULL;
        }
        peers = temp;
        int ip_len = strlen(ip) + 1;
        if(ip_len > 0 && ip[ip_len - 2] == '\n') {
            ip[ip_len - 2] = '\0';
            ip_len -= 1;
        }
        peers[*len] = malloc(sizeof(char) * ip_len);
        if(!peers[*len]) {
            // Handle memory allocation failure
            for(int i = 0; i < *len; i++) {
                free(peers[i]);
            }
            free(peers);
            *len = 0;
            return NULL;
        }
        strncpy(peers[*len], ip, ip_len);
        *len += 1;
    }
    return peers;
}

//store name of every file in directory
char **get_dir_files(DIR *dir, size_t *len) {
    char **files = NULL;
    *len = 0;
    while(true) {
        dirent *dir_ent = readdir(dir);
        if(!dir_ent) break;
        if(dir_ent->d_name[0] == '.') continue;
        char **temp = realloc(files, sizeof(char *) * (*len + 1));
        if(!temp) {
            for(int i = 0; i < *len; i++) {
                free(files[i]);
            }
            free(files);
            return NULL;
        }
        files = temp;
        int d_len = strlen(dir_ent->d_name) + 1;
        files[*len] = malloc(sizeof(char) * d_len);
        strncpy(files[*len], dir_ent->d_name, d_len);
        *len += 1;
    }
    return files;
}

//list filenames in memory
void show_files(char **files, size_t file_len) {
    if(file_len <= 0) {
        printf("Nao foi possivel encontrar arquivos\n");
        return;
    }
    for(int i = 0; i < file_len; i++) {
        printf("  %s\n", files[i]);
    }
}

char *dir_file_path(char *dir_path, char *file_name) {
    size_t d_path_size = strlen(dir_path);
    char *file_path = malloc(sizeof(char) * (strlen(file_name) + d_path_size));
    strncpy(file_path, dir_path, d_path_size);
    file_path[d_path_size] = "/";
    strncpy(file_path + d_path_size, file_name, strlen(file_name));
    return file_path;
}

int fsize(char *file) {
    FILE *fp = fopen(file, "r");
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}