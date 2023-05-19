#include <stdlib.h>
#include <stdio.h>

#include "include\Command.h"

void initCommandQueue(struct CommandQueue *queue) {
    queue->first = queue->last = 0;
}

void freeCommandQueue(struct CommandQueue *queue, pthread_mutex_t *mutex) {
    struct Command* node;

    pthread_mutex_lock(mutex);

    while(queue->first) {
        node = queue->first->next;
        freeCommand(queue->first);
        queue->first = node;
    }

    pthread_mutex_unlock(mutex);
}

void freeCommand(struct Command* cmd) {
    freeString(&cmd->command);
    free(cmd);
}

// The function used to take a Command from the pool.
// Member fields get undefined values
struct Command* getCommandCreate(struct CommandQueue *queue, pthread_mutex_t *mutex) {
    struct Command *ret;

    if (queue->first == 0) {
        ret = malloc(sizeof(struct Command));
        initString(&ret->command, 0);
    } else {
        pthread_mutex_lock(mutex);

        ret = queue->first;

        queue->first = queue->first->next;
        if (queue->first == 0) {
            queue->last = 0;
        }

        pthread_mutex_unlock(mutex);
    }

    return ret;
}

struct Command* getCommand(struct CommandQueue *queue, pthread_mutex_t *mutex) {
    struct Command* ret;

    if (mutex != NULL) {
        pthread_mutex_lock(mutex);
    }

    ret = queue->first;

    if (ret != 0) {
        queue->first = queue->first->next;

        if (queue->first == 0) {
            queue->last = 0;
        }
    }

    if (mutex != NULL) {
        pthread_mutex_unlock(mutex);
    }

    return ret;
}

void putCommand(struct CommandQueue* queue, struct Command* command, pthread_mutex_t *mutex, pthread_cond_t *condition) {
    pthread_mutex_lock(mutex);

    command->next = NULL;

    if (queue->first == 0) {
        queue->first = queue->last = command;
    } else {
        queue->last->next = command;
        queue->last = command;
    }

    if (condition != NULL) {
        pthread_cond_signal(condition); 
    }
    
    pthread_mutex_unlock(mutex);
}

