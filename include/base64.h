/*
This product includes software developed by the Apache Group for use in the Apache HTTP server project (http://www.apache.org/).
*/
#ifndef _BASE64_H
#define _BASE64_H

int base64encode_len(int len); //size when encoded
int base64_encode(char *encoded, const char *string, int len); //ascii -> base64
int base64decode_len(const char *bufcoded); //size when decoded
int base64_decode(char *bufplain, const char *bufcoded); //base64 -> ascii

#endif