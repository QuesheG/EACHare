#ifndef FILE_H
#define FILE_H
#include <stdio.h>
#include <dirent.h>

typedef struct dirent dirent;

//receive a string and separate it in two
char ** separator(char *str, char tok);

//read file with peers' ips and return them in text form
char ** read_peers(FILE *f, size_t *len);

//store name of every file in directory
char ** get_dir_files(DIR *dir, size_t *len);

//list filenames in memory
void show_files(char **files, size_t file_len);

#endif