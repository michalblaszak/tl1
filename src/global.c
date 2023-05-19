#include <stdio.h>

#include "include/global.h"


char toUpper(char c) {
    if (c >= 'a' && c <= 'z')
        return c - (char)32;
    else
        return c;
}

void printHex(char* s, int len) {
    for (int i = 0; i < len; i++) {
        printf(" %02x", (unsigned char)s[i]);
    }
}
