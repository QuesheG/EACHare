#ifndef FILE_H
#define FILE_H
#include <stdio.h>
#include <dirent.h>

typedef struct dirent dirent;

//read file with peers' ips and return them in text form
char ** read_peers(FILE *f, int *len);
//store name of every file in directory
char ** get_dir_files(DIR *dir, int *len);

void show_files(char **files, int file_len);

#endif