#ifndef __TELNET_H__
#define __TELNET_H__

/*********************************************************************
 * TELNET
 * -------------------------------------------------------------------
 * rfc854  TELNET PROTOCOL SPECIFICATION
 * rfc855  TELNET OPTION SPECIFICATIONS
 * rfc856  TELNET BINARY TRANSMISSION
 * rfc857  TELNET ECHO OPTION
 * rfc858  TELNET SUPPRESS GO AHEAD OPTION
 * rfc859  TELNET STATUS OPTION
 * rfc860  TELNET TIMING MARK OPTION
 * rfc861  TELNET EXTENDED OPTIONS - LIST OPTION
 * rfc779  TELNET SEND-LOCATION Option
 * rfc1091 Telnet Terminal-Type Option
 * rfc1010 ASSIGNED NUMBERS    - lists official terminal type names
 * rfc1073 Telnet Window Size Option
 * 
 * ------------------------------------------------------------------
 * VT100
 * ------------------------------------------------------------------
 * https://www.csie.ntu.edu.tw/~r92094/c++/VT100.html
 * http://ascii-table.com/ansi-escape-sequences-vt-100.php
 * https://en.wikipedia.org/wiki/ANSI_escape_code
 * 
 ********************************************************************/

enum ETelnetCommands {
    TEL_NUL  = 0,
    TEL_LF   = 10,  // 0A
    TEL_CR   = 13,  // 0D
    TEL_NOP  = 241,
    TEL_SB   = 250, // FA
    TEL_SE   = 240, // F0
    TEL_IAC  = 255,
    TEL_WILL = 251,
    TEL_WONT = 252,
    TEL_DO   = 253,
    TEL_DONT = 254,
    TEL_SEND = 1,
    TEL_IS   = 0
};

enum ETelnetOptions {
    TEL_ECHO          = 1,
    TEL_SGA           = 3,   // Suppress Go Ahead
    TEL_STATUS        = 5,
    TEL_TIMING_MARK   = 6,
    TEL_TERMINAL_TYPE = 24,
    TEL_NAWS          = 31, // 1F
    TEL_EXOPL         = 255 // FF Extended Option List
};

#define VT100_ESC '\x1B'
#define TEL_SIZEOF(a) (sizeof(a)/sizeof(char))

static char S_IAC[]  = "\xFF";
static char S_ESC[]  = "\x1B";
static char S_CR[]   = "\x0D";
static char S_CRLF[] = "\x0D\x0A";

static char IAC_WILL_TERMINAL_TYPE[]           = {TEL_IAC, TEL_WILL, TEL_TERMINAL_TYPE};
static char IAC_SB_TERMINAL_TYPE_SEND_IAC_SE[] = {TEL_IAC, TEL_SB, TEL_TERMINAL_TYPE, TEL_SEND, TEL_IAC, TEL_SE};
static char IAC_SB_TERMINAL_TYPE_IS[]          = {TEL_IAC, TEL_SB, TEL_TERMINAL_TYPE, TEL_IS};
static char IAC_SB_NAWS[]                      = {TEL_IAC, TEL_SB, TEL_NAWS};

static char VT100_ERASE_LINE[]       = "\x0D\x1B\x5BJ";
static char VT100_ERASE_N_LINES[]    = "\x0D\x1B\x5B%dA\x1B\x5BJ"; // Parametrized (%d) to be used in 'sprintf'
static char VT100_AUTO_WRAP[]        = "\x1B\x5b?7l";
static char VT100_CURSOR_TO_BEGIN[]  = "\x0D\x1B\x5B%dA"; // Parametrized (%d) to be used in 'sprintf'
static char VT100_CURSOR_DOWN[]      = "\x1B\x5B%dB"; // Parametrized (%d) to be used in 'sprintf'
static char VT100_CURSOR_RIGHT[]     = "\x1B\x5B%dC"; // Parametrized (%d) to be used in 'sprintf'

static char VT100_LEFT[]   = {'\x1B', '\x5B', '\x44'};
static char VT100_UP[]     = {'\x1B', '\x5B', '\x41'};
static char VT100_RIGHT[]  = {'\x1B', '\x5B', '\x43'};
static char VT100_DOWN[]   = {'\x1B', '\x5B', '\x42'};
static char VT100_HOME1[]  = {'\x1B', '\x5B', '\x48'};
static char VT100_HOME2[]  = {'\x1B', '\x5B', '\x31', '\x7E'};
static char VT100_END1[]   = {'\x1B', '\x5B', '\x46'};
static char VT100_END2[]   = {'\x1B', '\x5B', '\x34', '\x7E'};
static char VT100_DELETE[] = {'\x1B', '\x5B', '\x33', '\x7E'};
static char VT100_HIGHLIGHT[] = {'\x1B', '\x5B', '1', 'm'};
static char VT100_UNDERLINE[] = {'\x1B', '\x5B', '4', 'm'};
static char VT100_INVERSE[]   = {'\x1B', '\x5B', '7', 'm'};
static char VT100_NORMALCHAR[] = {'\x1B', '\x5B', 'm'};

#define C_LF  '\x0A'
#define C_CR  '\x0D'
#define C_BS  '\x7F'     // Backspace (on Win it may mean Del but can be swapped by a telnet user manually)
#define C_DEL '\x08'     // Backspace as defined in Telnet and implemented in Win however not used in xterm and putty. So Win client has to swap and BS and DEL so this one will act as DEL

static char telnet_header[] = {
    TEL_IAC, TEL_WILL, TEL_ECHO,
    TEL_IAC, TEL_DONT, TEL_ECHO,
    TEL_IAC, TEL_WILL, TEL_SGA,
    TEL_IAC, TEL_DO,   TEL_NAWS,
    TEL_IAC, TEL_DO,   TEL_TERMINAL_TYPE
};

/*********************************************************
* Proposed Negotiation
*---------------------------------------------------------
* Server:
*   IAC WILL ECHO,
*   IAC DONT ECHO,
*   IAC WILL SGA,
*   IAC DO   NAWS,
*   IAC DO   TERMINAL_TYPE
*=========================================================
* Measured Responses
*---------------------------------------------------------
* Msys2 on Win10
*---------------------------------------------------------
*  IAC DO   ECHO
*  IAC DO   SGA
*  IAC WILL NAWS
*  IAC SB   NAWS 003B 0010 IAC SE
*  IAC WILL TERMINAL_TYPE
*---------------------------------------------------------
* CMD Telnet on Win10
*---------------------------------------------------------
*  IAC DO   ECHO
*  IAC WONT ECHO
*  IAC DO   SGA
*  IAC WILL NAWS
*  IAC SB   NAWS 003B 0010 IAC SE
*  IAC WILL TERMINAL_TYPE
*---------------------------------------------------------
* Putty on Win10
*---------------------------------------------------------
*  IAC WILL NAWS
*  IAC WILL TERMINAL_SPEED
*  IAC WILL TERMINAL_TYPE
*  IAC WILL 39
*  IAC DO   ECHO
*  IAC WILL SGA
*  IAC DO   SGA 
*  IAC SB   NAWS 0050 0018 IAC SE
*********************************************************/

#endif