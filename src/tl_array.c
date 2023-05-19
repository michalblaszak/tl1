#include <stdlib.h>
#include "include/global.h"
#include "include/tl_array.h"

void initArray_int(struct Array_int *s, int size) {
    if (s == NULL)
        return;

    if (size == 0) {
        s->data = NULL;
        s->_data_size = 0;
        s->extent = STANDARD_ARRAY_EXTENT;
    } else if (s->_data_size == 0) {
        s->data = malloc(sizeof(int) * size);
        s->_data_size = size;
        s->extent = size;
    } else {
        s->extent = size;
    }

    s->len = 0;
}

void freeArray_int(struct Array_int *s) {
    free(s->data);
    s->len = 0;
    s->_data_size = 0;
}

// !!! Not prototyped in the header file
void _extendInt(struct Array_int *s, int len) {
    if (s != NULL && len > 0) {
        if (len > s->_data_size) {
            s->_data_size = s->extent * (1 + len/s->extent);
            s->data = realloc((int*)s->data, sizeof(int) * s->_data_size);
        }
    }
}


void append_int(struct Array_int *s, int c) {
    if (s != NULL) {
        int new_len = s->len + 1;
        _extendInt(s, new_len);
        s->data[s->len] = c;
        s->len = new_len;
    }
}

void addSorted_int(struct Array_int *s, int c) {
    if (s != NULL) {
        int i;

        // Find a place
        int found_i = s->len; // -1 means existing duplicate found

        for(i=0; i<s->len; i++) {
            if (s->data[i] == c) {
                found_i = -1;
                break;
            }
            if (s->data[i] > c) {
                found_i = i;
                break;
            }
        }

        // Make room till the found place and put the new value
        if (found_i != -1) { 
            int new_len = s->len + 1;
            _extendInt(s, new_len);

            for(i=s->len-1; i>=found_i; i--) {
                s->data[i+1] = s->data[i];
            }

            s->data[i+1] = c;
            s->len++;
        }
    }
}