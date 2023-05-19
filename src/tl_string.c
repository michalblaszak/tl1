#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "include/global.h"
#include "include/tl_string.h"

void initString(struct String *s, int size) {
    if (s == NULL)
        return;

    s->str = NULL;
    s->_str_size = 0;
    s->extent = (size == 0 ? STANDARD_STRING_EXTENT : size);

    s->len = 0;
}

void resetString(struct String *s) {
    if (s != NULL) {
        s->len = 0;
    }
}

void freeString(struct String *s) {
    free(s->str);
    s->len = 0;
    s->_str_size = 0;
}

// !!! Not prototyped in the header file
void _extendString(struct String *s, int len) {
    if (s != NULL && len > 0) {
        if (len > s->_str_size) {
            s->_str_size = s->extent * (1 + len/s->extent);
            s->str = realloc((void*)s->str, s->_str_size);
        }
    }
}

void setCString(struct String *s, char *s_in, int len) {
    if (s != NULL) {
        if(s_in != NULL && len > 0) {
            _extendString(s, len);

            memmove(s->str, s_in, len);
            s->len = len;
        } else {
            s->len = 0;
        }

    }
}

void setSString(struct String *s, struct String *s_in) {
    if (s_in != NULL) {
        setCString(s, s_in->str, s_in->len);
    } else {
        s->len = 0;
    }
}

void appendCharString(struct String *s, char c) {
    if (s != NULL) {
        int new_len = s->len + 1;
        _extendString(s, new_len);
        s->str[s->len] = c;
        s->len = new_len;
    }
}

void appendCString(struct String *s, char *s_in, int len) {
    if (s != NULL && s_in != NULL && len > 0) {
        int new_len = s->len + len;
        _extendString(s, new_len);

        memmove(s->str+s->len, s_in, len);
        s->len = new_len;
    }
}

void appendSString(struct String *s, struct String *s_in) {
    if (s_in != NULL) {
        appendCString(s, s_in->str, s_in->len);
    }
}

// Returns:
//  0 - strings are different
//  1 - strings are equal
int  compareString(struct String *s1, char *s2, int s2_len) {
    if (s1->len < s2_len) {
        return 0;
    } else {
        return memcmp(s1->str, s2, s2_len) == 0 ? 1 : 0;
    }
}

void stringToUpper(char *s, int len) {
    for(int i = 0; i < len; i++) {
        s[i] = toUpper(s[i]);
    }
}

int sprintfString(struct String *s, char *format, ...) {
    va_list args;
    va_start(args, format);
    
    int n = vsnprintf(NULL, 0, format, args);
    _extendString(s, n+1);
    vsprintf(s->str, format, args);
    s->len = n;

    va_end(args);

    return n;
}

int sprintfAppendString(struct String *s, char *format, ...) {
    va_list args;
    va_start(args, format);
    
    int n = vsnprintf(NULL, 0, format, args);
    _extendString(s, s->len + n+1);
    vsprintf(s->str + s->len, format, args);
    s->len = s->len + n;

    va_end(args);

    return n;
}
