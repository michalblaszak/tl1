#include <stdio.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 

#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "include/global.h"
#include "include/Command.h"
#include "include/Connection.h"
#include "include/Telnet.h"
#include "include/TL1_Command.h"


struct GlobalConfig globalConfig;

struct CommandQueue command_pool;
pthread_mutex_t command_pool_lock = PTHREAD_MUTEX_INITIALIZER; 

struct CommandQueue command_queue;
pthread_mutex_t command_queue_lock = PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t  command_queue_non_empty = PTHREAD_COND_INITIALIZER;

pthread_t serverThread_id;
enum EThreadStatus serverThreadStatus;

void init() {
    globalConfig.inputBufferSize = INPUT_BUFFER_SIZE;
    globalConfig.portHuman = PORT_HUMAN;
    globalConfig.portMachine = PORT_MACHINE;
    globalConfig.numPendingConnections = NUM_PENDING_CONNECTIONS;
    globalConfig.lineBufferExtent = LINE_BUFFER_EXTENT;
    globalConfig.readTimeout.tv_sec = 0;
    globalConfig.readTimeout.tv_usec = 500000;
    globalConfig.promptLen = 1;
}

void init_buffers() {
    initCommandQueue(&command_queue);
    initCommandQueue(&command_pool);
}

void free_resources() {
    freeCommandQueue(&command_queue, &command_queue_lock);
    freeCommandQueue(&command_pool, &command_pool_lock);
}

void clearTerminalLine(struct Connection *con) {
    if (con->win_w == 0) {
        // Nothing to do, the virtual screen hasn't been initialized yet
        resetString(&con->term_buffer);

        return;
    }
    
    pthread_mutex_lock(&con->line_buffer_mutex);

    int number_of_lines = (con->line_buffer.len) / con->win_w;
    int cursor_line = con->line_buffer.current_pos / con->win_w;
    printf("window width: %d\n", con->win_w);
    printf("line_buffer.len/line_buffer.current_pos: %d/%d\n", con->line_buffer.len, con->line_buffer.current_pos);
    printf("Number of lines/cursor line: %d/%d\n", number_of_lines, cursor_line);

    // Move cursor to the beginning of line
    // Move cursor to the first line of the command
    // Erase the screen below the cursor
    int n;
    
    if (cursor_line == 0) {
        n = sprintfString(&con->term_buffer, VT100_ERASE_LINE);
    } else {
        n = sprintfString(&con->term_buffer, VT100_ERASE_N_LINES, cursor_line);
    }

    pthread_mutex_unlock(&con->line_buffer_mutex);
}

void add_command_to_command_queue(struct Connection *con) {
    // Create a new node
    struct Command *command_tmp = getCommandCreate(&command_pool, &command_pool_lock);

    command_tmp->next = 0;
    if (con->line_buffer.len == 0) {
        command_tmp->command.len = 0;
    } else {
        setCommandString(command_tmp, con->line_buffer.buf, con->line_buffer.len);
    }

    command_tmp->con = con;

    // Add a new command to the command queue for processing
    // Add a new node to the end of the queue
    putCommand(&command_queue, command_tmp, &command_queue_lock, &command_queue_non_empty);

    con->line_buffer.len = 0;  // Make buffer ready for a new command
    con->line_buffer.current_pos = 0;

    //TODO: Put the command echo to the console
    //displayText(con);
}

void insertHighlight(struct Connection* con, struct Array_int* error_positions, int* cur_err_pos, int pos_start, int pos_end, int text_len) {
    while (*cur_err_pos >= 0
        && pos_start <= error_positions->data[*cur_err_pos]
        && pos_end >= error_positions->data[*cur_err_pos]) { // Handle highlighting the error position
        int first_part_len = error_positions->data[*cur_err_pos] - pos_start;
        int second_part_len = pos_end - error_positions->data[*cur_err_pos];

        if (first_part_len > 0)
            appendCString(&con->term_buffer, con->line_buffer.buf + pos_start, first_part_len);

        appendCString(&con->term_buffer, VT100_HIGHLIGHT, 4);
        appendCString(&con->term_buffer, VT100_INVERSE, 4);
        appendCString(&con->term_buffer, con->line_buffer.buf + error_positions->data[*cur_err_pos], 1);
        appendCString(&con->term_buffer, VT100_NORMALCHAR, 3);

        pos_start = error_positions->data[*cur_err_pos] + 1;
        text_len = second_part_len;
        pos_end = pos_start + text_len - 1;

        // if (second_part_len > 0)
        //     appendCString(&con->term_buffer, con->line_buffer.buf + error_positions.data[cur_err_pos] + 1, second_part_len);

        (*cur_err_pos)++; // Take the next error pos from the array
        if (*cur_err_pos == error_positions->len)
            *cur_err_pos = -1; // All errors have been processed
    } // ~while
    
    if (text_len > 0)
        appendCString(&con->term_buffer, con->line_buffer.buf + pos_start, text_len);
}

void splitLines(struct Connection *con) {
    int line = 1;
    struct Array_int error_positions;
    int cur_err_pos = -1; // -1 stands for 'no errors' or 'all errors have been processed'

    initArray_int(&error_positions, 0);
    if (con->errorStack.error != PARSE_OK) {
        errorStack_GetPositions(&con->errorStack, &error_positions);
        if (error_positions.len > 0)
            cur_err_pos = 0;
    }

    while (1) {
        int pos_start;
        int text_len;
        int pos_end;

        if (line * con->win_w <= con->line_buffer.len + globalConfig.promptLen) {
            // Take the entire line
            if (line == 1) {
                pos_start = (line-1)*con->win_w;
                text_len = con->win_w - globalConfig.promptLen;

                appendCharString(&con->term_buffer, '<');
            } else { // Not the 1st line
                pos_start = (line-1)*con->win_w - globalConfig.promptLen;
                text_len = con->win_w;
            }

            pos_end = pos_start + text_len - 1;

            insertHighlight(con, &error_positions, &cur_err_pos, pos_start, pos_end, text_len);

            appendCString(&con->term_buffer, S_CRLF, 2);
            line++;
        } else {
            // It's the last line. Take the reminder
            if (line == 1) { // this is the first (and last) line so take entire buffer
                appendCharString(&con->term_buffer, '<');
                if (con->line_buffer.len > 0) {
                    pos_start = 0;
                    text_len = con->line_buffer.len;
                    pos_end = pos_start + text_len - 1;

                    insertHighlight(con, &error_positions, &cur_err_pos, pos_start, pos_end, text_len);
                    // appendCString(&con->term_buffer, con->line_buffer.buf, con->line_buffer.len);
                }
            } else {
                int bytes_to_copy = con->line_buffer.len - (line-1) * con->win_w + globalConfig.promptLen;
                if (bytes_to_copy > 0) {
                    pos_start = (line-1)*con->win_w - globalConfig.promptLen;
                    text_len = bytes_to_copy;
                    pos_end = pos_start + text_len - 1;

                    insertHighlight(con, &error_positions, &cur_err_pos, pos_start, pos_end, text_len);
                    // appendCString(&con->term_buffer, con->line_buffer.buf + (line-1)*con->win_w - globalConfig.promptLen, bytes_to_copy);
                }
            }
            break;
        }
    } // ~while

    freeArray_int(&error_positions);
}

// Parsing the content of 'raw_line_buffer' and moving processed string to 'line_buffer'
// Assumptions:
//  - the calling function must set 'line_buffer_mutex'
void pre_parseInput(struct Connection *con) {
    enum EInputLineState state = TL1_PLAIN;

    con->line_buffer.len = 0;
    con->line_buffer.current_pos = 0;
    con->line_buffer.buf = realloc(con->line_buffer.buf,con->raw_line_buffer.size); // Make the 'line_buffer" the same size as 'raw_line_buffer'

    for(int i=0; i<con->raw_line_buffer.len; i++) {
        printf("i=%d\n", i);
        char c = con->raw_line_buffer.buf[i];

        if (c == '"' && state == TL1_PLAIN) {
            state = TL1_QUOTATION;
            appendToLineBuffer(con, con->raw_line_buffer.buf[i]);
        } else if (c == '"' && state == TL1_QUOTATION) {
            state = TL1_PLAIN;
            appendToLineBuffer(con, con->raw_line_buffer.buf[i]);
        } else if (c == '\\' && state == TL1_QUOTATION) {
            state = TL1_ESCAPE_IN_QUOTATION;
            appendToLineBuffer(con, con->raw_line_buffer.buf[i]);
        } else if (c == ';' && state == TL1_PLAIN) {
            // Found a complete command - execute it
            appendToLineBuffer(con, con->raw_line_buffer.buf[i]);

            // Debug
            struct String str_cmd;
            initString(&str_cmd, 0);
            setCString(&str_cmd, con->raw_line_buffer.buf, con->raw_line_buffer.len);
            enum EParseStatus cmd = TL1CommandParse(&str_cmd, &con->errorStack);

            //_TL1CommandPrintBlocks(cmd);

            //TL1CmdFree(cmd);
            freeString(&str_cmd);
            // End debug

            clearTerminalLine(con);
            splitLines(con);
            appendCString(&con->term_buffer, S_CRLF, 2);
            write(con->sock_fd, con->term_buffer.str, con->term_buffer.len);
            add_command_to_command_queue(con);
            // Make room for a next command which maybe in 'raw_line_buffer'
            con->line_buffer.len = 0;
            con->line_buffer.current_pos = 0;
            errorStack_freeInit(&con->errorStack);

            // Move the rest of raw_line_buffer to the left after processing the previous command
            if (con->raw_line_buffer.len > (i+1)) {
                memmove(con->raw_line_buffer.buf,
                    con->raw_line_buffer.buf+i+1,
                    con->raw_line_buffer.len-(i+1));
            }
            con->raw_line_buffer.len = con->raw_line_buffer.len-(i+1);
            con->raw_line_buffer.current_pos = 0;
            printf("raw_line_buffer after ';':\n");
            printHex(con->raw_line_buffer.buf, con->raw_line_buffer.len);
            i=-1; // Will be incremented at the end of the 'for' cycle
        } else {
            if (state == TL1_ESCAPE_IN_QUOTATION) {
                state = TL1_QUOTATION;
                appendToLineBuffer(con, con->raw_line_buffer.buf[i]);
            } else if (state == TL1_QUOTATION) {
                appendToLineBuffer(con, con->raw_line_buffer.buf[i]);
            } else {
                appendToLineBuffer(con, toUpper(con->raw_line_buffer.buf[i]));
            }
        }
    }
}

// The function adds the VT100 sequence to position the cursor.
// Assumptions:
//  - the calling function must set 'line_buffer_mutex'
//  - the 'line_buffer' contains the string and the cursor position is there
//  - the 'term_buffer' is intiated with the text
void positionTheCursor(struct Connection *con) {
    int cursor_line, cursor_column;

    if (con->line_buffer.len == 0) {
        return;
    }

    // Move cursor to the beginning of the new text
    cursor_line = (con->line_buffer.len + globalConfig.promptLen) / con->win_w; // At this point cursor is at the end of the entire text

    if (cursor_line == 0) {
        sprintfAppendString(&con->term_buffer, S_CR);
    } else {
        sprintfAppendString(&con->term_buffer, VT100_CURSOR_TO_BEGIN, cursor_line);
    }
    
    // Move cursor to the correct place
    cursor_line   = (con->line_buffer.current_pos + globalConfig.promptLen) / con->win_w;
    cursor_column = (con->line_buffer.current_pos + globalConfig.promptLen) - cursor_line * con->win_w;

    if (cursor_line > 0) {
        sprintfAppendString(&con->term_buffer, VT100_CURSOR_DOWN, cursor_line);
    }

    if (cursor_column > 0) {
        sprintfAppendString(&con->term_buffer, VT100_CURSOR_RIGHT, cursor_column);
    }
}

void handleBackspace(struct Connection *con) {
    if (con->raw_line_buffer.current_pos == 0) {
        return;
    }

    pthread_mutex_lock(&con->line_buffer_mutex);

    // Remove a preceding character
    if (con->raw_line_buffer.current_pos == con->raw_line_buffer.len) { // Is the cursor at the end of the text?
        con->raw_line_buffer.current_pos--;
        con->raw_line_buffer.len--;
    } else {
        memmove(con->raw_line_buffer.buf+con->raw_line_buffer.current_pos-1,
               con->raw_line_buffer.buf+con->raw_line_buffer.current_pos,
               con->raw_line_buffer.len - con->raw_line_buffer.current_pos);
        con->raw_line_buffer.current_pos--;
        con->raw_line_buffer.len--;
    }

    pre_parseInput(con);
    // Restore the cursor position
    con->line_buffer.current_pos = con->raw_line_buffer.current_pos;

    // Split into lines
    clearTerminalLine(con);
    splitLines(con);

    // Position the cursor
    positionTheCursor(con);

    write(con->sock_fd, con->term_buffer.str, con->term_buffer.len);

    pthread_mutex_unlock(&con->line_buffer_mutex);
}

void handleDelete(struct Connection *con) {
    if (con->raw_line_buffer.current_pos == con->raw_line_buffer.len) {
        return;
    }

    pthread_mutex_lock(&con->line_buffer_mutex);

    // Remove a following character
    memmove(con->raw_line_buffer.buf+con->raw_line_buffer.current_pos,
            con->raw_line_buffer.buf+con->raw_line_buffer.current_pos+1,
            con->raw_line_buffer.len - (con->raw_line_buffer.current_pos+1));
    con->raw_line_buffer.len--;

    pre_parseInput(con);
    // Restore the cursor position
    con->line_buffer.current_pos = con->raw_line_buffer.current_pos;

    // Split into lines
    clearTerminalLine(con);
    splitLines(con);

    // Position the cursor
    positionTheCursor(con);

    write(con->sock_fd, con->term_buffer.str, con->term_buffer.len);

    pthread_mutex_unlock(&con->line_buffer_mutex);
}

void add_char_to_line_buf(struct Connection *con, char c) {
    pthread_mutex_lock(&con->line_buffer_mutex);

    // Make more room in a "line_buf" if needed.
    if (con->raw_line_buffer.len >= con->raw_line_buffer.size) {
        con->raw_line_buffer.size += globalConfig.lineBufferExtent;
        con->raw_line_buffer.buf = realloc(con->raw_line_buffer.buf, con->raw_line_buffer.size);
    }
    // Insert the new character to the "raw_line_buf" at the 'current_pos' position. Items on the right side have to be moved forward by one.
    if (con->raw_line_buffer.current_pos < con->raw_line_buffer.len) {
        memmove(con->raw_line_buffer.buf+con->raw_line_buffer.current_pos+1, 
               con->raw_line_buffer.buf+con->raw_line_buffer.current_pos, 
               con->raw_line_buffer.len - con->raw_line_buffer.current_pos);
    }

    con->raw_line_buffer.buf[con->raw_line_buffer.current_pos++] = c;
    con->raw_line_buffer.len++;

    printf("raw_line_buffer: %.*s\n", con->raw_line_buffer.len, con->raw_line_buffer.buf);
    // Parse 'raw_line_buffer' and move valid text to 'line_buffer'
    pre_parseInput(con);
    // Restore the cursor position
    con->line_buffer.current_pos = con->raw_line_buffer.current_pos;

    printf("line_buffer: %.*s\n", con->line_buffer.len, con->line_buffer.buf);
    printf("raw_line_buffer: %.*s\n", con->raw_line_buffer.len, con->raw_line_buffer.buf);
    // Split into lines
    clearTerminalLine(con);
    splitLines(con);

    // Position cursor
    positionTheCursor(con);

    printf("VT100:\n");
    printHex(con->term_buffer.str, con->term_buffer.len);
    putchar('\n');

    write(con->sock_fd, con->term_buffer.str, con->term_buffer.len);

    pthread_mutex_unlock(&con->line_buffer_mutex);
}

void printTelnetCommand(struct Connection *con) {
    printf("Telnet:");
    printHex(con->telnetCommand, con->telnetCommand_len);
    putchar('\n');
}

void printVT100Command(struct Connection *con) {
    printf("VT100:");
    printHex(con->vt100Command, con->vt100Command_len);
    putchar('\n');
}

void askForTerminalType(struct Connection *con) {
    if(con->ask_for_terminalType) {
        struct Command* rsp = getCommandCreate(&command_pool, &command_pool_lock);

        setCommandString(rsp, IAC_SB_TERMINAL_TYPE_SEND_IAC_SE, TEL_SIZEOF(IAC_SB_TERMINAL_TYPE_SEND_IAC_SE));

        putCommand(&con->response_queue, rsp, &con->response_mutex, &con->response_condition);
    }
}

void requestTerminalAutoWrap(struct Connection *con) {
    if(con->ask_for_terminalType) {
        struct Command* rsp = getCommandCreate(&command_pool, &command_pool_lock);

        setCommandString(rsp, VT100_AUTO_WRAP, TEL_SIZEOF(VT100_AUTO_WRAP));

        putCommand(&con->response_queue, rsp, &con->response_mutex, &con->response_condition);
    }
}

void processTelnetCommand(struct Connection *con) {
    printTelnetCommand(con);

    // The Client accpented negotiating the terminal type.
    // Start asking for available terminal types
    if (memcmp(con->telnetCommand, IAC_WILL_TERMINAL_TYPE, TEL_SIZEOF(IAC_WILL_TERMINAL_TYPE)) == 0) {
        askForTerminalType(con);
    } else 
    if (memcmp(con->telnetCommand, IAC_SB_NAWS, TEL_SIZEOF(IAC_SB_NAWS)) == 0) {
        // Also refreshes the command in the terminal
        pthread_mutex_lock(&con->line_buffer_mutex);
        
        printf("NAWS: %02x %02x: %02x %02x\n", (unsigned char)con->telnetCommand[3], (unsigned char)con->telnetCommand[4], (unsigned char)con->telnetCommand[5], (unsigned char)con->telnetCommand[6]);

        clearTerminalLine(con);

        con->win_w = (unsigned char)con->telnetCommand[3] * 256 + (unsigned char)con->telnetCommand[4];
        con->win_h = (unsigned char)con->telnetCommand[5] * 256 + (unsigned char)con->telnetCommand[6];
        printf("NAWS: %d, %d\n", con->win_w, con->win_h);

        // Terminal should handle wrapping so the cursor shoulld be in a row after changing the windows size
        splitLines(con);
        // Position cursor
        positionTheCursor(con);

        write(con->sock_fd, con->term_buffer.str, con->term_buffer.len);

        pthread_mutex_unlock(&con->line_buffer_mutex);
    } else
    if (memcmp(con->telnetCommand, IAC_SB_TERMINAL_TYPE_IS, TEL_SIZEOF(IAC_SB_TERMINAL_TYPE_IS)) == 0) {
        int terminal_name_len = strcspn(con->telnetCommand+4, S_IAC);
        stringToUpper(con->telnetCommand+4, terminal_name_len);

        printf("Terminal type: %.*s", terminal_name_len, con->telnetCommand+4);

        if (memcmp("VT100", con->telnetCommand+4, 5) == 0) {
            // This is what we were looking for.
            // Stop asking for other terminal types
        } else {
            // Is it the last terminal type supported by the client?
            if (compareString(&con->last_terminal_read, con->telnetCommand+4, terminal_name_len)) {
                // Ask one more time to take the first one offerred by the client
                con->ask_for_terminalType = 0; // Stop asking for more terminal types
            }

            askForTerminalType(con); // Will not ask if con->ask_for_terminalType == 0
        }
        setCString(&con->last_terminal_read, con->telnetCommand+4, terminal_name_len);
    }
}

enum ECursorDirection {
    CURSOR_LEFT, CURSOR_RIGHT, CURSOR_UP, CURSOR_DOWN, CURSOR_HOME, CURSOR_END
};

void moveCursor(struct Connection *con, enum ECursorDirection direction) {
    int cursor_line, cursor_column;

    pthread_mutex_lock(&con->line_buffer_mutex);

    // Move cursror to the beginning
    cursor_line = (con->raw_line_buffer.current_pos + globalConfig.promptLen) / con->win_w;

    if (cursor_line == 0) {
        sprintfString(&con->term_buffer, S_CR);
    } else {
        sprintfString(&con->term_buffer, VT100_CURSOR_TO_BEGIN, cursor_line);
    }

    // Move cursor one character to the left
    switch (direction) {
        case CURSOR_LEFT:
            con->raw_line_buffer.current_pos--;
            break;
        case CURSOR_RIGHT:
            con->raw_line_buffer.current_pos++;
            break;
        case CURSOR_UP:
            con->raw_line_buffer.current_pos -= con->win_w;
            break;
        case CURSOR_DOWN:
            con->raw_line_buffer.current_pos += con->win_w;
            break;
        case CURSOR_HOME:
            con->raw_line_buffer.current_pos = 0;
            break;
        case CURSOR_END:
            con->raw_line_buffer.current_pos = con->raw_line_buffer.len;
            break;
    } // ~switch

    con->line_buffer.current_pos = con->raw_line_buffer.current_pos;

    cursor_line   = (con->raw_line_buffer.current_pos + globalConfig.promptLen) / con->win_w;
    cursor_column = (con->raw_line_buffer.current_pos + globalConfig.promptLen) - cursor_line * con->win_w;

    if (cursor_line > 0) {
        sprintfAppendString(&con->term_buffer, VT100_CURSOR_DOWN, cursor_line);
    }

    if (cursor_column > 0) {
        sprintfAppendString(&con->term_buffer, VT100_CURSOR_RIGHT, cursor_column);
    }

    printHex(con->term_buffer.str, con->term_buffer.len);
    write(con->sock_fd, con->term_buffer.str, con->term_buffer.len);

    pthread_mutex_unlock(&con->line_buffer_mutex);
}

void processVT100Command(struct Connection *con) {
    printVT100Command(con);

    if (memcmp(con->vt100Command, VT100_LEFT, TEL_SIZEOF(VT100_LEFT)) == 0) {
        if (con->raw_line_buffer.current_pos > 0) {
            moveCursor(con, CURSOR_LEFT);
        }
    } else if (memcmp(con->vt100Command, VT100_UP, TEL_SIZEOF(VT100_UP)) == 0) {
        if (con->raw_line_buffer.current_pos < con->win_w) { // Are we in the first line
            // TODO: Take the previous from the history
        } else {
            moveCursor(con, CURSOR_UP);
        }
    } else if (memcmp(con->vt100Command, VT100_RIGHT, TEL_SIZEOF(VT100_RIGHT)) == 0) {
        if (con->raw_line_buffer.current_pos < con->raw_line_buffer.len) {
            moveCursor(con, CURSOR_RIGHT);
        }
    } else if (memcmp(con->vt100Command, VT100_DOWN, TEL_SIZEOF(VT100_DOWN)) == 0) {
        if (con->raw_line_buffer.current_pos + con->win_w > con->raw_line_buffer.len) { // Are we in the 'last' line
            // TODO: Take the next from the history
        } else {
            moveCursor(con, CURSOR_DOWN);
        }
    } else if (memcmp(con->vt100Command, VT100_HOME1, TEL_SIZEOF(VT100_HOME1)) == 0 ||
               memcmp(con->vt100Command, VT100_HOME2, TEL_SIZEOF(VT100_HOME2)) == 0) {
        if (con->raw_line_buffer.current_pos > 0) {
            moveCursor(con, CURSOR_HOME);
        }
    } else if (memcmp(con->vt100Command, VT100_END1, TEL_SIZEOF(VT100_END1)) == 0 ||
               memcmp(con->vt100Command, VT100_END2, TEL_SIZEOF(VT100_END2)) == 0) {
        if (con->raw_line_buffer.current_pos < con->raw_line_buffer.len) {
            moveCursor(con, CURSOR_END);
        }
    } else if (memcmp(con->vt100Command, VT100_DELETE, TEL_SIZEOF(VT100_DELETE)) == 0) {
        handleDelete(con);
    } else {
        printf("Unhandled VT100 sequence.");
    }
}

#define APPEND_TO_TELNET_COMMAND (con->telnetCommand[con->telnetCommand_len++]=c)
#define EXIT_TELNET_PROCESSING ( {\
    con->processingMode = TEXT; \
    *telnet_state = TEL_NONE; \
    con->telnetCommand_len = 0; \
})
#define APPEND_TO_VT100_COMMAND  (con->vt100Command[con->vt100Command_len++]=c)
#define EXIT_VT100_PROCESSING ({ \
    con->processingMode = TEXT; \
    *vt100_state = VT100_NONE; \
    con->vt100Command_len = 0; \
})

void process_input(struct Connection *con, int len) {
    // Strip /n
    // Detect ';'
    // Be quotation-sensitive
    for(int i=0; i<len; i++) {
        char c = con->input_buffer.buf[i];
        enum EInputLineState *state = &con->inputLineState; // This is just a shotcut. A pointer as we want changes made to 'state' be stored in the connection object
        enum EInputLineState *telnet_state = &con->telnetState; // This is just a shotcut. A pointer as we want changes made to 'telnet_state' be stored in the connection object
        enum EInputLineState *vt100_state = &con->vt100State; // This is just a shotcut. A pointer as we want changes made to 'vt100_state' be stored in the connection object

        printf("%02x ", (unsigned char)c);

        switch(con->processingMode) {
            case TEXT: {
                if (c == C_LF || c == C_CR) {
                     // New line.
                     // Just skip it.
                } else if (c >= 0x20 && c <= 0x7E) {
                    add_char_to_line_buf(con, c);
                //     // Printable characters.
                //     if (c == '"' && *state == TL1_PLAIN) {
                //         *state = TL1_QUOTATION;
                //         add_char_to_line_buf(con, c);
                //     } else
                //     if (c == '"' && *state == TL1_QUOTATION) {
                //         *state = TL1_PLAIN;
                //         add_char_to_line_buf(con, c);
                //     } else
                //     if (c == '\\' && *state == TL1_QUOTATION) {
                //         *state = TL1_ESCAPE_IN_QUOTATION;
                //         add_char_to_line_buf(con, c);
                //     } else
                //     if (c == ';' && *state == TL1_PLAIN) {
                //         // ';' is a TL-1 end-of line
                //         add_char_to_line_buf(con, c);
                //         add_command_to_command_queue(con);

                //         // Debug
                //         struct String str_cmd;
                //         initString(&str_cmd, con->line_buffer.len);
                //         setCString(&str_cmd, con->line_buffer.buf, con->line_buffer.len);
                //         struct TL1Cmd* cmd = TL1CommandParse(&str_cmd);

                //         _TL1CommandPrintBlocks(cmd);

                //         TL1CmdFree(cmd);
                //         freeString(&str_cmd);
                //         // End debug
                //     } else {
                //         if (*state == TL1_ESCAPE_IN_QUOTATION) {
                //             *state = TL1_QUOTATION;
                //             add_char_to_line_buf(con, c);
                //         } else if (*state == TL1_QUOTATION) {
                //             add_char_to_line_buf(con, c);
                //         } else {
                //             add_char_to_line_buf(con, toUpper(c));
                //         }
                //     }
                } else if (c == (char)TEL_IAC) { // Maybe Telnet
                    con->processingMode = TELNET;
                    *telnet_state = TEL_START;
                    APPEND_TO_TELNET_COMMAND;
                } else if (c == VT100_ESC) {
                    con->processingMode = VT100;
                    *vt100_state = VT100_ESC;
                    APPEND_TO_VT100_COMMAND;
                } else if (c == C_BS) { // Backspace (on Win it may mean Del but can be swapped by a telnet user manually)
                    handleBackspace(con);
                } else if (c == C_DEL) {
                    handleDelete(con);
                } else {
                    // Other unprintable characters.
                    // Just skip it.
                }

                break;
            }   // ~case TEXT

            case TELNET: {
                if (*telnet_state == TEL_START) {
                    if (c == (char)TEL_WILL || c == (char)TEL_WONT || c == (char)TEL_DO || c == (char)TEL_DONT) {
                        *telnet_state = TEL_COMMAND;
                        APPEND_TO_TELNET_COMMAND;
                    } else if (c == (char)TEL_SB) {
                        *telnet_state = TEL_SUBCOMMAND;
                        APPEND_TO_TELNET_COMMAND;
                    } else {
                        // Other Telnet command -> igonore it
                        EXIT_TELNET_PROCESSING;
                    }
                } else if (*telnet_state == TEL_COMMAND) {
                    APPEND_TO_TELNET_COMMAND;

                    processTelnetCommand(con);

                    EXIT_TELNET_PROCESSING;
                } else if (*telnet_state == TEL_SUBCOMMAND) {
                    if (c == (char)TEL_IAC) {
                        *telnet_state = TEL_MAYBE_END_SUBCOMMAND;
                        APPEND_TO_TELNET_COMMAND;
                    } else {
                        APPEND_TO_TELNET_COMMAND;
                    }
                } else if (*telnet_state == TEL_MAYBE_END_SUBCOMMAND) {
                    if (c == (char)TEL_IAC) { // It's not the end of subcommand, it's a repeated 255. We do not add it to the buffer
                        *telnet_state = TEL_SUBCOMMAND;
                        // con->telnetCommand[con->telnetCommand_len++] = c;
                    } else if (c == (char)TEL_SE) { // Real end of subcommand
                        APPEND_TO_TELNET_COMMAND;

                        processTelnetCommand(con);

                        EXIT_TELNET_PROCESSING;
                    } else { // Looks like a incorectly formed Telnet subcommand (expectd SE after IAC)
                        // Ignore it
                        EXIT_TELNET_PROCESSING;
                    }
                } else {
                    // We shouldn't land here.
                    // Something is wrong with the Telnet state machine
                    APPEND_TO_TELNET_COMMAND;
                    printf("Incorrect telnet command.\n");
                    printTelnetCommand(con);

                    EXIT_TELNET_PROCESSING;
                }

                break;
            } // ~case TELNET

            case VT100: {
                if (*vt100_state == VT100_ESC) {
                    if ( c == '\x5B') {
                        APPEND_TO_VT100_COMMAND;
                        *vt100_state = VT100_ESC_BRAC;
                    } else {
                        // Invalid symbol found
                        APPEND_TO_VT100_COMMAND;
                        printf("Incorrect VT100 command.\n");
                        printVT100Command(con);
                        
                        EXIT_VT100_PROCESSING;
                    }
                } else if (*vt100_state == VT100_ESC_BRAC) {
                    switch (c) { 
                        case '\x44':   // Left arrow
                        case '\x41':   // Up arrow
                        case '\x43':   // Right arrow
                        case '\x42':   // Down arrow
                        case '\x48':   // Home on xterm
                        case '\x46': { // End on xterm
                            APPEND_TO_VT100_COMMAND;
                            processVT100Command(con);
                            EXIT_VT100_PROCESSING;

                            break;
                        }

                        case '\x33': { // Possibly Del on xterm and Putty
                            APPEND_TO_VT100_COMMAND;
                            *vt100_state = VT100_DEL;
                            break;
                        }

                        case '\x31': { // Possibly Home on win and Putty
                            APPEND_TO_VT100_COMMAND;
                            *vt100_state = VT100_HOME;
                            break;
                        }

                        case '\x34': { // Possibly End on win and Putty
                            APPEND_TO_VT100_COMMAND;
                            *vt100_state = VT100_END;
                            break;
                        }
                    } // ~switch c
                } else if (*vt100_state == VT100_DEL || *vt100_state == VT100_HOME || *vt100_state == VT100_END) { // Several sequences end with the same code
                    if (c == '\x7E') { // Del on xterm and Putty; Home, End on win and Putty
                        APPEND_TO_VT100_COMMAND;
                        processVT100Command(con);
                        EXIT_VT100_PROCESSING;
                    } else {
                        // Incorrect VT100 esc sequence
                        APPEND_TO_VT100_COMMAND;
                        printf("Incorrect VT100 command.\n");
                        printVT100Command(con);

                        EXIT_VT100_PROCESSING;
                    }
                } else {
                    // We shouldn't land here.
                    // Something is wrong with the VT100 state machine
                    APPEND_TO_VT100_COMMAND;
                    printf("Incorrect VT100 command.\n");
                    printVT100Command(con);

                    EXIT_VT100_PROCESSING;
                }

                break;
            } // ~case VT100
        } // ~switch
    }
}

int process_read(struct Connection *con) {
    int len;

    bzero(con->input_buffer.buf, con->input_buffer.size); 
    con->input_buffer.len = 0;
    printf("Waiting for input.\n");
    // read the message from client and copy it to buffer 
    len = recv(con->sock_fd, con->input_buffer.buf, con->input_buffer.size, 0);
    
    if (len < 0) { // Error reading
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("Read timeout.\n");
            // OK - just timeout on read
        } else {
            printf("Error reading from client.\n");
            return 1;
        }
    } else if (len == 0) { // The client has disconnected
        return 2;
    } else {
        printf("Received %d bytes.\n", len);
        con->input_buffer.len = len;
        process_input(con, len);
    }

    return 0;
}

void sendTelnetHeader(struct Connection *con) {
    write(con->sock_fd, telnet_header, TEL_SIZEOF(telnet_header));

//    int count;

//    ioctl(con->sock_fd, FIONREAD, &count);
//    len = recv(con->sock_fd, con->input_buffer.buf, con->input_buffer.size, 0);

}

/* [void *con] = [struct Connection *] */
void* clientLoop(void *con) {
    // Initial TELNET negotiation
    sendTelnetHeader((struct Connection *)con);

    sendGreeting((struct Connection *)con);

    // Main client loop
    while(1) { 
        if (process_read((struct Connection *)con) != 0) {
            break;
        }
    }

    freeConnection(con);
    pthread_exit(NULL);
}

void* rspClientLoop(void *con) {
    struct Command* cmd;
    struct Connection* conp = (struct Connection*) con; // This one is to simplify usage below

    while(1) {
        pthread_mutex_lock(&conp->response_mutex);
        if (conp->response_queue.first == NULL) {
            pthread_cond_wait(&conp->response_condition, &conp->response_mutex);
        }

        // Process all elements from the queue
        pthread_mutex_lock(&conp->line_buffer_mutex);

        while ( (cmd = getCommand(&conp->response_queue, NULL)) != NULL ) {
            clearTerminalLine(conp);
            appendSString(&conp->term_buffer, &cmd->command);
            // Repaint the current line_buffer
            if (conp->win_w > 0) { // The terminal has been initialized
                splitLines(conp);
                positionTheCursor(conp);
            }

            write(conp->sock_fd, conp->term_buffer.str, conp->term_buffer.len);

            putCommand(&command_pool, cmd, &command_pool_lock, NULL);
        } // ~while(cmd)
        
        pthread_mutex_unlock(&conp->line_buffer_mutex);


        pthread_mutex_unlock(&conp->response_mutex);
    } // ~while(1)
}

void* serverLoop() {
    struct Command* cmd;
    struct String rsp_str;

    initString(&rsp_str, 0);

    while(1) {
        pthread_mutex_lock(&command_queue_lock);
        if (command_queue.first == NULL) {
            pthread_cond_wait(&command_queue_non_empty, &command_queue_lock);
        }
        printf("Server Loop: new command.\n");

        // Process all elements from the queue
        while ( (cmd = getCommand(&command_queue, NULL)) != NULL ) {
            printf("Server Loop: got a command: %.*s\n", cmd->command.len, cmd->command.str);
            if (cmd != 0) {
                // Process the command and generate the response
                if (compareString(&cmd->command, "CANC-USER", 9)) {
                    struct Command* rsp = getCommandCreate(&command_pool, &command_pool_lock);

                    setCommandString(rsp, "Bye", 3);
                    appendCommandString(rsp, S_CRLF, 2);

                    putCommand(&cmd->con->response_queue, rsp, &cmd->con->response_mutex, &cmd->con->response_condition);
                    putCommand(&command_pool, cmd, &command_pool_lock, NULL);
                } else if (compareString(&cmd->command, "RTRV-SYS-ATTR", 13)) {
                    struct Command* rsp = getCommandCreate(&command_pool, &command_pool_lock);

                    resetString(&rsp_str);
                    setCString(&rsp_str, "TERMINAL-TYPE=", 14);
                    appendSString(&rsp_str, &cmd->con->last_terminal_read);
                    appendCString(&rsp_str, S_CRLF, 2);
                    appendCString(&rsp_str, "TERMINAL-SIZE=", 14);
                    char size_buf[20];
                    int n = sprintf(size_buf, "%dx%d", cmd->con->win_w, cmd->con->win_h);
                    
                    appendCString(&rsp_str, size_buf, n);
                    appendCString(&rsp_str, S_CRLF, 2);

                    setCommandSString(rsp, &rsp_str);

                    putCommand(&cmd->con->response_queue, rsp, &cmd->con->response_mutex, &cmd->con->response_condition);
                    putCommand(&command_pool, cmd, &command_pool_lock, NULL);
                } else {
                    struct Command* rsp = getCommandCreate(&command_pool, &command_pool_lock);
                    setCommandString(rsp, "Response:", 3);
                    appendCommandString(rsp, S_CRLF, 2);

                    putCommand(&cmd->con->response_queue, rsp, &cmd->con->response_mutex, &cmd->con->response_condition);
                    putCommand(&command_pool, cmd, &command_pool_lock, NULL);
                }
            }
        } // ~while(cmd)

        pthread_mutex_unlock(&command_queue_lock);
    } // ~while(1)

    freeString(&rsp_str);
}

void initServerThread() {
    if (pthread_create(&serverThread_id, NULL, serverLoop, NULL) == 0) {
        serverThreadStatus = RUNNING;
    } else {
        serverThreadStatus = ERROR;
    }
}

int prepare_socket(int port) {
    int sockfd;
    struct sockaddr_in srvaddr;

	/*              IPv4,    TCP,        IP */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		printf("Error: socket creation failed.\n");
		exit(0);
	}

	bzero(&srvaddr, sizeof(srvaddr));

	// assign IP, PORT 
    srvaddr.sin_family = AF_INET; 
    srvaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    srvaddr.sin_port = htons(port); 

	//int setsockopt(sockfd, int level, int optname,  
    //               const void *optval, socklen_t optlen);

	if( bind(sockfd, (struct sockaddr *)&srvaddr, sizeof(srvaddr)) !=0) {
		printf("Error: socket bind failed for port %d.\n", port);
		exit(0);
	}

    if ((listen(sockfd, NUM_PENDING_CONNECTIONS)) != 0) { 
        printf("Error: listen failed for port %d...\n", port); 
        exit(0); 
    } 

    printf("Waiting for a connection on port %d...\n", port);

    return sockfd;
}

struct Connection *acceptConnection(int sockfd, int *confd, enum EConnectionType connectionType) {
    struct sockaddr_in cli;
    int len;

    len = sizeof(cli);

    // Accept the data packet from client and verification 
    *confd = accept(sockfd, (struct sockaddr *)&cli, &len); 
    if (*confd < 0) { 
        printf("Error: acccepting connection failed...\n"); 
        exit(0); 
    } 

    // Setup a read timeout
 //   setsockopt(*confd, SOL_SOCKET, SO_RCVTIMEO, (const void*)&globalConfig.readTimeout, sizeof(struct timeval));
    //fcntl(client_sd, F_SETFL, O_NONBLOCK);

    char *client_ip_address = inet_ntoa(cli.sin_addr);
    printf("Connection arrived on %s port:\n", connectionType == HUMAN ? "HUMAN" : "MACHINE");
    printf("  Client address: %s\n", client_ip_address);
    printf("  Client port: %d\n", ntohs(cli.sin_port));

    struct Connection *conn = createConnection(*confd, connectionType, &cli, clientLoop, rspClientLoop);

    return conn;
}

int main(void) {
    int sockfd_human, sockfd_machine, confd, result_select;
    fd_set fdset, fdset_loop;

    init();
    init_buffers();

    initServerThread();

    printf("TL1 Server is up\n");

    sockfd_human = prepare_socket(PORT_HUMAN);
    sockfd_machine = prepare_socket(PORT_MACHINE);

    FD_ZERO(&fdset);    /* initialize the fd set */
    FD_SET(sockfd_human, &fdset);
    FD_SET(sockfd_machine, &fdset); /* add socket fd */

    // Main Server Loop
    while(1) {
        fdset_loop = fdset;

        result_select=select(FD_SETSIZE, &fdset_loop, 0, 0, 0);
        printf("Someone connected ...\n");

        if (result_select < 0) {
            printf("Error: accepting the socket set failed.\n");
            exit(0);
        }

        if (FD_ISSET(sockfd_human, &fdset_loop)) {
            // Accept the data packet from client and create the connection object. 
            struct Connection *conn = acceptConnection(sockfd_human, &confd, HUMAN);
        }

        if (FD_ISSET(sockfd_machine, &fdset_loop)) {
            // Accept the data packet from client and verification 
            struct Connection *conn = acceptConnection(sockfd_machine, &confd, MACHINE);
        }
    } // ~while(1)

    free_resources();

    // After chatting close the socket 
    close(sockfd_human); 
    close(sockfd_machine); 
}