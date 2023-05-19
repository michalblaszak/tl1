#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "include/global.h"
#include "include/Connection.h"
#include "include/Command.h"
#include "include/CommandQueue.h"

struct Connection* createConnection(int sock_fd, 
                                    enum EConnectionType type, 
                                    struct sockaddr_in *cli, 
                                    void *(*thread_proc) (void *),
                                    void *(*rsp_thread_proc) (void *))
{
    struct Connection *con = malloc(sizeof(struct Connection));

    con->sock_fd = sock_fd;
    con->conn_type = type;

    memcpy(&con->cli, cli, sizeof(struct sockaddr_in));

    con->input_buffer.size = globalConfig.inputBufferSize;
    con->input_buffer.len = 0;
    con->input_buffer.current_pos = 0;
    con->input_buffer.buf = malloc(con->input_buffer.size);

    con->raw_line_buffer.size = globalConfig.lineBufferExtent;
    con->raw_line_buffer.len = 0;
    con->raw_line_buffer.current_pos = 0;
    con->raw_line_buffer.buf = malloc(con->raw_line_buffer.size);

    con->line_buffer.size = globalConfig.lineBufferExtent;
    con->line_buffer.len = 0;
    con->line_buffer.current_pos = 0;
    con->line_buffer.buf = malloc(con->line_buffer.size);

    con->next = 0;

    con->inputLineState = TL1_PLAIN;
    con->processingMode = TEXT;
    con->telnetState = TEL_NONE;
    con->vt100State = VT100_NONE;

    con->telnetCommand_len = 0;
    con->vt100Command_len = 0;

    con->win_w = 0;
    con->win_h = 0;
    con->cursor_pos = 0;

    con->ask_for_terminalType = 1; // Ask a client for supported terminal types
    initString(&con->last_terminal_read, 0);

    initString(&con->term_buffer, TERM_BUFFER_EXTENT);

    errorStackInit(&con->errorStack);

    initCommandQueue(&con->response_queue);

    if (pthread_create(&con->thread_id, NULL, thread_proc, con) == 0) {
        con->status = RUNNING;
    } else {
        con->status = ERROR;
    }

    con->line_buffer_mutex = PTHREAD_MUTEX_INITIALIZER; 
    con->response_mutex = PTHREAD_MUTEX_INITIALIZER;
    con->response_condition = PTHREAD_COND_INITIALIZER;

    pthread_mutexattr_init(&con->line_buffer_mta);
    pthread_mutexattr_settype(&con->line_buffer_mta, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&con->line_buffer_mutex, &con->line_buffer_mta);

    if (pthread_create(&con->rsp_thread_id, NULL, rsp_thread_proc, con) == 0) {
        con->rsp_status = RUNNING;
    } else {
        con->rsp_status = ERROR;
    }

    return con;
}

void freeConnection(struct Connection* con) {
    close(con->sock_fd);
    free(con->input_buffer.buf);
    free(con->line_buffer.buf);
    freeString(&con->term_buffer);
    freeString(&con->last_terminal_read);

    free_errorElement(&con->errorStack);

    free(con);
}

void addConnection(struct Connection* con) {

}

void sendGreeting(struct Connection* con) {
    static char *greeting = "Welcome to the TL1 Server\x0D\x0A";

    struct Command *cmd = getCommandCreate(&command_pool, &command_pool_lock);
    
    setCommandString(cmd, greeting, strlen(greeting));
    cmd->con = con;

    putCommand(&con->response_queue, cmd, &con->response_mutex, &con->response_condition);
}

void appendToLineBuffer(struct Connection *con, char c) {
    con->line_buffer.buf[con->line_buffer.current_pos++] = c;
    con->line_buffer.len++;
}