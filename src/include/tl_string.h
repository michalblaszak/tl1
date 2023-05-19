#ifndef __TL_STRING_H__
#define __TL_STRING_H__

#define STANDARD_STRING_EXTENT 100

struct String {
    char *str;
    int _str_size;
    int len;
    int extent;
};

void initString(struct String *s, int size);
void resetString(struct String *s);
void freeString(struct String *s);
void setCString(struct String *s, char *s_in, int len);
void setSString(struct String *s, struct String *s_in);
void appendCString(struct String *s, char *s_in, int len);
void appendCharString(struct String *s, char c);
void appendSString(struct String *s, struct String *s_in);
int  compareString(struct String *s1, char *s2, int s2_len);
void stringToUpper(char *s, int len);
int  sprintfString(struct String *s, char *format, ...);
int  sprintfAppendString(struct String *s, char *format, ...);

#endif