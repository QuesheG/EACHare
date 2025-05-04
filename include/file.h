#ifndef _FILE_H
#define _FILE_H
#include <stdio.h>
#include <dirent.h>

typedef struct dirent dirent;

char ** read_peers(FILE *f, size_t *len); //read file with peers' ips and return them in text form
char ** get_dir_files(const char *dir, size_t *len); //store name of every file in directory
void show_files(const char **files, size_t file_len); //list filenames in memory
char *dir_file_path(const char *dir_path, const char *file_name); //append file_name to dir_path to get the entire path
int fsize(const char *file); //get size of file

#endif