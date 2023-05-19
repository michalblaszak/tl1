#include <stdlib.h>
#include <string.h>
#include "include\Command.h"

void setCommandString(struct Command* cmd, char* str, int len) {
    if (cmd != NULL) {
        setCString(&cmd->command, str, len);
    }
}

void setCommandSString(struct Command* cmd, struct String *str) {
    if (cmd != NULL && str != NULL) {
        setSString(&cmd->command, str);
    }
}

void appendCommandString(struct Command* cmd, char* str, int len) {
    if (cmd != NULL) {
        appendCString(&cmd->command, str, len);
    }
}

void appendCommandSString(struct Command* cmd, struct String *str) {
    if (cmd != NULL && str != NULL) {
        appendSString(&cmd->command, str);
    }
}