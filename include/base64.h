/*
This product includes software developed by the Apache Group for use in the Apache HTTP server project (http://www.apache.org/).
*/
#ifndef BASE64_H
#define BASE64_H

int base64_encode(char *encoded, const char *string, int len); //ascii -> base64
int base64_decode(char *bufplain, const char *bufcoded); //base64 -> ascii

#endif