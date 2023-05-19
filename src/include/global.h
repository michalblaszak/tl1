#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <sys/time.h>
//#include "CommandQueue.h"

#define INPUT_BUFFER_SIZE 80
#define PORT_HUMAN 8080
#define PORT_MACHINE 8082
#define NUM_PENDING_CONNECTIONS 10
#define LINE_BUFFER_EXTENT 80
#define TERM_BUFFER_EXTENT 100

struct GlobalConfig {
    int inputBufferSize;
    int portHuman;
    int portMachine;
    int numPendingConnections;
    int lineBufferExtent;
    struct timeval readTimeout;
    int promptLen;
};

extern struct GlobalConfig globalConfig;

extern struct CommandQueue command_pool;
extern pthread_mutex_t command_pool_lock; 

char toUpper(char c);
void printHex(char* s, int len);

#endif