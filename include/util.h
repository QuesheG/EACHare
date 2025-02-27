#ifndef UTIL
#define UTIL
//copy str2 into str1
void cpy_str(char * str1, char * str2, int n);
//size of str
int size_str(char * str);
//receive a string and separate it in two
char ** separator(char *str, char tok);
#endif