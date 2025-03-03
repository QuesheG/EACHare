#include <util.h>
#include <stdlib.h>
#include <stdbool.h>

//copy str2 into str1
void cpy_str(char * str1, char * str2, int n) {
    for(int i = 0; i < n; i++) {
        str1[i] = str2[i];
    }
}

//size of str
int size_str(char * str) {
    int i;
    for(i = 0; str[i] != '\0'; i++);
    return i + 1;
}

//receive a string and separate it in two
char ** separator(char *str, char tok) {
    char ** res = malloc(sizeof(char *) * 2);
    for(int i = 0; str[i] != '\0'; i++) {
        if(str[i] == tok) {
            char * n_str = malloc(sizeof(char) * i + 1);
            cpy_str(n_str, str, i);
            n_str[i] = '\0';
            res[0] = n_str;
            int remainder = size_str(str) - i - 1;
            n_str = malloc(sizeof(char) * remainder);
            cpy_str(n_str, str + i + 1, remainder);
            res[1] = n_str;
            return res;
        }
    }
    free(res);
    return NULL;
}

//read file with peers' ips and return them in text form
char ** read_peers(FILE *f, int *len) {
    char ** peers;
    char ip[22] = {0}; //max ip size = 255.255.255.255:65535 -> 3+1+3+1+3+1+3+1+5 = 21 + null terminator
    while(true) {
        //read each line of file and put it in peers
        fgets(ip, 22, f); //TODO: tratar \n
    }
}
