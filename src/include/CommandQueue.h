#ifndef __COMMAND_QUEUE_H__
#define __COMMAND_QUEUE_H__

#include <pthread.h>
#include "Command.h"

struct CommandQueue {
    struct Command *first;
    struct Command *last;
};

void initCommandQueue(struct CommandQueue *queue);
void freeCommandQueue(struct CommandQueue *queue, pthread_mutex_t *mutex);
struct Command* getCommandCreate(struct CommandQueue *queue, pthread_mutex_t *mutex);
struct Command* getCommand(struct CommandQueue *queue, pthread_mutex_t *mutex);
void putCommand(struct CommandQueue* queue, struct Command* command, pthread_mutex_t *mutex, pthread_cond_t *condition);
void freeCommand(struct Command* cmd);

#endif