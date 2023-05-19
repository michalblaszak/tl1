#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <pthread.h>
#include <netinet/in.h>
#include "CommandQueue.h"
#include "global.h"
#include "tl_string.h"
#include "TL1_Command.h"

enum EConnectionType {HUMAN, MACHINE};
enum EThreadStatus {RUNNING, STOPPED, ERROR};
enum EInputLineState {
    TL1_PLAIN, TL1_QUOTATION, TL1_ESCAPE_IN_QUOTATION, 
    TEL_NONE, TEL_START, TEL_COMMAND, TEL_SUBCOMMAND, TEL_MAYBE_END_SUBCOMMAND,
    VT100_NONE, VT100_ESC, VT100_ESC_BRAC, VT100_DEL, VT100_HOME, VT100_END};
enum EProcessingMode {TEXT, TELNET, VT100};

struct Buffer {
    char* buf;
    int len; // position of the last character (-1 == empty)
    int current_pos; // The current position to enter new elements
    int size; // Storage size (buffer capacity)
};

struct Connection {
    int sock_fd;
    enum EConnectionType conn_type;
    pthread_t thread_id;
    pthread_t rsp_thread_id;
    struct sockaddr_in cli;
    struct Buffer input_buffer; // Collects character comming from the socket
    struct Buffer raw_line_buffer; // Character collected as arrived from the line_buffer
    struct Buffer line_buffer; // The edit line buffer (what is visible in the user's console)
    pthread_mutex_t line_buffer_mutex;
    pthread_mutexattr_t line_buffer_mta;
    enum EInputLineState inputLineState;
    enum EInputLineState telnetState;
    enum EInputLineState vt100State;
    struct CommandQueue response_queue;
    pthread_mutex_t response_mutex;
    pthread_cond_t response_condition;

    enum EThreadStatus status;
    enum EThreadStatus rsp_status;
    enum EProcessingMode processingMode;

    char telnetCommand[30]; // Telnet command incoming from the client
    int  telnetCommand_len;

    char vt100Command[10]; // VT100 escape sequence incoming from the client
    int  vt100Command_len;

    struct String term_buffer;
    int win_w;
    int win_h;
    int cursor_pos;

    char ask_for_terminalType; // 0 - no, 1 - yes
    struct String last_terminal_read; // This is to control Telnet client providing available terminal types

    struct SErrorElement errorStack;

    struct Connection *next;
};

struct Connetion *connections;

struct Connection* createConnection(int sock_fd, 
                                    enum EConnectionType type, 
                                    struct sockaddr_in *cli, 
                                    void *(*thread_proc) (void *),
                                    void *(*rsp_thread_proc) (void *));
void addConnection(struct Connection *con);
void freeConnection(struct Connection *con);

void sendGreeting(struct Connection *con);

void appendToLineBuffer(struct Connection *con, char c);

#endif