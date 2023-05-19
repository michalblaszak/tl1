#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <pthread.h>
#include "Connection.h"
#include "tl_string.h"

struct Command {
    struct Connection *con;
    pthread_t thread_id;

    struct String command;
    struct Command* next;
};

void setCommandString(struct Command* cmd, char* str, int len);
void setCommandSString(struct Command* cmd, struct String *str);
void appendCommandString(struct Command* cmd, char* str, int len);
void appendCommandSString(struct Command* cmd, struct String *str);
#endif