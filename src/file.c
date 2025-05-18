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
char **get_dir_files(const char *dir_path, size_t *len) {
    DIR *dir = opendir(dir_path);
    if(!dir) {
        perror("Erro: Diretorio nao achado, cheque escrita ou existencia do diretorio");
        return NULL;
    }
    char **files = NULL;
    *len = 0;
    while(true) {
        dirent *dir_ent = readdir(dir);
        if(!dir_ent) break;
        if(dir_ent->d_name[0] == '.') continue;
        if(dir_ent->d_type != DT_REG) continue;
        char **temp = realloc(files, sizeof(char *) * (*len + 1));
        if(!temp) {
            for(int i = 0; i < *len; i++) {
                free(files[i]);
            }
            free(files);
            closedir(dir);
            return NULL;
        }
        files = temp;
        files[*len] = strdup(dir_ent->d_name);
        if(!files[*len]) {
            for(size_t i = 0; i < *len; i++) {
                free(files[i]);
            }
            free(files);
            closedir(dir);
            return NULL;
        }
        *len += 1;
    }
    closedir(dir);
    return files;
}

//list filenames in memory
void show_files(const char **files, size_t file_len) {
    if(file_len <= 0) {
        printf("Nao foi possivel encontrar arquivos\n");
        return;
    }
    for(int i = 0; i < file_len; i++) {
        printf("  %s\n", files[i]);
    }
}

char *dir_file_path(const char *dir_path, const char *file_name) {
    size_t d_path_size = strlen(dir_path);
    size_t ch_file_size = strlen(file_name);
    char *file_path = malloc(sizeof(char) * (ch_file_size + d_path_size + 2));
    strncpy(file_path, dir_path, d_path_size);
    file_path[d_path_size] = '/';
    strncpy(file_path + d_path_size + sizeof(char), file_name, ch_file_size);
    file_path[d_path_size + ch_file_size + 1] = '\0';
    return file_path;
}

int fsize(const char *file) {
    FILE *fp = fopen(file, "r");
    if(!fp) {
        fprintf(stderr, "Erro: Falha lendo arquivo!\n");
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    fclose(fp);
    return sz;
}