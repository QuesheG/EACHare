#ifndef FILE_H
#define FILE_H
#include <stdio.h>
#include <dirent.h>

typedef struct dirent dirent;

char ** read_peers(FILE *f, size_t *len); //read file with peers' ips and return them in text form
char ** get_dir_files(DIR *dir, size_t *len); //store name of every file in directory
void show_files(char **files, size_t file_len); //list filenames in memory

#endif