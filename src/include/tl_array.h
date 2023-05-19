#ifndef __TL_ARRAY_H__
#define __TL_ARRAY_H__

#define STANDARD_ARRAY_EXTENT 100

struct Array_int {
    int *data;
    int _data_size;
    int len;
    int extent;
};

void initArray_int(struct Array_int *s, int size);
void freeArray_int(struct Array_int *s);
void append_int(struct Array_int *s, int c);
void addSorted_int(struct Array_int *s, int c);
#endif
