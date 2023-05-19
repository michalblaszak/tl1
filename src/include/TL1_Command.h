#ifndef __TL1_COMMAND_H__
#define __TL1_COMMAND_H__

#include "tl_string.h"
#include "tl_vector.h"
#include "tl_array.h"

// enum EAttributeNameParseStatus {
//     NAME_OK,
//     ERROR_NAME_NORMAL,
//     ERROR_NAME_NORMAL_EMPTY,
//     ERROR_NAME_QUOTED_UNFINISHED,
//     ERROR_NAME_QUOTED_EMPTY,
//     ERROR_NAME_QUOTED
// };

// struct AttributeName {
//     struct String text;
//     struct String parsed_text;

//     enum EAttributeNameParseStatus parse_status;
// };

// enum EAttributeValueParseStatus {
//     VALUE_OK,
//     ERROR_VALUE_CMD,
//     ERROR_EMPTY_CMD,
//     ERROR_UNFINISHED_CMD
// };

// struct AttributeValue {
//     struct String text;
//     struct String parsed_text;

//     enum EAttributeValueParseStatus parse_status;
// };

// struct Attribute {
//     struct AttributeName name;
//     struct AttributeValue value;

//     struct String text;

//     struct Attribute *next;
// };

// enum EBlock {
//     CMD_BLOCK,
//     TID_BLOCK,
//     AID_BLOCK,
//     CTAG_BLOCK,
//     GENERAL_BLOCK,
//     PAYLOAD_BLOCK
// };

// struct CommandBlock {
//     struct Attribute *first_attr;
//     struct Attribute *last_attr;

//     enum EBlock blockType;
//     struct String text;

//     struct CommandBlock *next;
// };

// enum EParseStatus_old {
//     PARSE_OK_OLD,
//     CMD_NAME_ERR,   // Malformed command name
// };

// struct TL1Cmd {
//     struct CommandBlock *first_cmd_block;
//     struct CommandBlock *last_cmd_block;

//     enum EParseStatus_old parseStatus;
//     int err_char_pos;   // The first character where the syntax error occurs (counted from 1; 0 - no error)
// };

// void TL1CmdInit(struct TL1Cmd *cmd);
// struct TL1Cmd* TL1CmdCreate();
// void TL1CommandBlockInit(struct CommandBlock *blk, char* s, int len, enum EBlock blockType);
// struct CommandBlock* TL1CommandBlockCreate(char* s, int len, enum EBlock blockType);
// void TL1CmdAddBlock(struct TL1Cmd *cmd, struct CommandBlock *blk);
// void TL1CmdAddAttribute(struct CommandBlock *blk, struct Attribute *attr);
// void TL1AttributeNameInit(struct AttributeName* name);
// void TL1AttributeValueInit(struct AttributeValue* value);
// void TL1AttributeNameFree(struct AttributeName* name);
// void TL1AttributeValueFree(struct AttributeValue* value);
// void TL1AttributeInit(struct Attribute *attr, char* s, int len);
// struct Attribute* TL1AttributeCreate(char* s, int len);
// void TL1AttributeFree(struct Attribute *attr);
// void TL1BlockFree(struct CommandBlock *blk);
// void TL1CmdFree(struct TL1Cmd *cmd);
// void TL1AttributeNameSet(struct Attribute* attr, char* s, int len);
// void TL1AttributeValueSet(struct Attribute* attr, char* s, int len);
// /* Parsers */
// void TL1AttributeNameParse(struct Attribute* attr);
// void TL1AttributeValueParse(char* s, int len);
// void TL1AttributeParse(struct Attribute *attr);
// void TL1BlockParse(struct CommandBlock *blk);
// struct TL1Cmd* TL1CommandParse_old(struct String *cmd_str);

// void _TL1CommandPrintBlocks(struct TL1Cmd* cmd);

struct Vector command_definitions;

/************************************************************************/

enum EParseStatus {
    PARSE_INITIAL                           = 0,
    PARSE_OK                                = 1,
    PARSE_ERROR_EXPECTED_LETTER             = 2,
    PARSE_ERROR_EXPECTED_DIGIT              = 3,
    PARSE_ERROR_EXPECTED_IDENT              = 4,
    PARSE_ERROR_EXPECTED_SYM_NAME           = 5,
    PARSE_ERROR_EXPECTED_ALPHANUM           = 6,
    PARSE_ERROR_EXPECTED_DECNUM             = 7,
    PARSE_ERROR_EXPECTED_CHAR               = 8,
    PARSE_ERROR_EXPECTED_ARITHEXPR          = 9,
    PARSE_ERROR_EXPECTED_HEXDIGIT           = 10,
    PARSE_ERROR_EXPECTED_OCTDIGIT           = 11,
    PARSE_ERROR_EXPECTED_BINDIGIT           = 12,
    PARSE_ERROR_EXPECTED_HEXNUM             = 13,
    PARSE_ERROR_EXPECTED_OCTNUM             = 14,
    PARSE_ERROR_EXPECTED_BINNUM             = 15,
    PARSE_ERROR_EXPECTED_KEYEDNUM           = 16,
    PARSE_ERROR_EXPECTED_INTEGER            = 17,
    PARSE_ERROR_EXPECTED_TEXTSTR            = 18,
    PARSE_ERROR_EXPECTED_STRINGCHAR         = 19,
    PARSE_ERROR_SEQUENCE_NO_MATCH           = 20,
    PARSE_ERROR_EXPECTED_CMDCODE            = 21,
    PARSE_ERROR_EXPECTED_TID                = 22,
    PARSE_ERROR_EXPECTED_AID                = 23,
    PARSE_ERROR_EXPECTED_AIDVALUE           = 24,
    PARSE_ERROR_EXPECTED_AIDNAME            = 25,
    PARSE_ERROR_EXPECTED_VALUECOMPLEX       = 26,
    PARSE_ERROR_EXPECTED_INPUTMESSAGE       = 27,
    PARSE_ERROR_EXPECTED_PARAMVALUE         = 28,
    PARSE_ERROR_EXPECTED_PARAMVALUECOMPLEX  = 29,
    PARSE_ERROR_EXPECTED_SEQCHAIN           = 30,
    PARSE_ERROR_EXPECTED_SIMPLESEQ          = 31,
    PARSE_ERROR_EXPECTED_COMPCHAIN          = 32,
    PARSE_ERROR_EXPECTED_CMPSEQ             = 33,
    PARSE_ERROR_EXPECTED_LINKEDSEQ          = 34,
    PARSE_ERROR_EXPECTED_CMPARG             = 35,
    PARSE_ERROR_EXPECTED_CTAG               = 36,
    PARSE_ERROR_EXPECTED_PAYLOADBLOCK       = 37,
    PARSE_ERROR_EXPECTED_POSDEFPARAMSEQ     = 38,
    PARSE_ERROR_EXPECTED_POSDEFPARAM        = 39,
    PARSE_ERROR_EXPECTED_NAMEDEFPARAMSEQ    = 40,
    PARSE_ERROR_EXPECTED_NAMEDEFPARAM       = 41,
    PARSE_ERROR_EXPECTED_INFOUNITCLASS      = 42,
    PARSE_EOF                               = 43,
    PARSE_ERROR_COMMANDCODE                 = 44,
    PARSE_ERROR_TID                         = 45,
    PARSE_ERROR_AID                         = 46,
    PARSE_ERROR_CTAG                        = 47,
    PARSE_ERROR_GENERALBLOCK                = 48,
    PARSE_ERROR_PAYLOADBLOCK                = 49,
    PARSE_ERROR_EXPECTED_COLON              = 50,
    PARSE_ERROR_UNKNOWN                     = 51,
    PARSE_ERROR_EXPECTED_QUOTATION          = 52,
    PARSE_ERROR_UNEXPECTEDCHARACTERS        = 53,
    PARSE_ERROR_EXPECTED_SYMNAME_PLUS_HASH_PERCENT  = 54,
    PARSE_ERROR_EXPECTED_SYMNAME_DIGIT              = 55,
    PARSE_ERROR_EXPECTED_SYMNAME_LETTER             = 56,
    PARSE_ERROR_EXPECTED_ALPHANUM_DIGIT_LETTER      = 57,
    PARSE_ERROR_EXPECTED_BRACKET_OPEN               = 58,
    PARSE_ERROR_EXPECTED_BRACKET_CLOSE              = 59,
    PARSE_ERROR_EXPECTED_KEYEDDIGIT                 = 60,
    PARSE_ERROR_KEYEDNUM_UNEXPECTEDSYMBOL           = 61,
    PARSE_ERROR_EXPECTED_CMPARG_MINUS               = 62,
    PARSE_ERROR_EXPECTED_SEQCHAIN_COMMA_GROUP_COLON_SEMICOLON   = 63,
    PARSE_ERROR_EXPECTED_PARAMVALUE_COMMA_COLON_SEMICOLON       = 64,
    PARSE_ERROR_EXPECTED_EQUAL                                  = 65,
    PARSE_ERROR_EXPECTED_CTAG_COLON_SEMICOLON                   = 66,
    PARSE_ERROR_EXPECTED_PARAMNAME                              = 67,
    PARSE_ERROR_EXPECTED_PAYLOAD_COLON_SEMICOLON                = 68,
    PARSE_ERROR_EXPECTED_SEQCHAIN_SIMPLESEQ_COMMA_GROUP_COLON_SEMICOLON = 69,
    PARSE_ERROR_EXPECTED_SEQCHAIN_CMPCHAIN_COMMA_GROUP_COLON_SEMICOLON  = 70,
    PARSE_ERROR_EXPECTED_PAYLOAD_POSDEF_COLON_SEMICOLON                 = 71,
    PARSE_ERROR_EXPECTED_PAYLOAD_NAMEDEF_COLON_SEMICOLON                = 72
};

extern char *parseErrorNames[];

struct SErrorElement {
    enum EParseStatus error;
    int pos;
    struct Vector suberrors;
};

void errorStackInit(struct SErrorElement* e);
void free_errorElement(void* t);
void errorStack_freeInit(struct SErrorElement* e);
void errorStack_GetPositions(struct SErrorElement* errorStack, struct Array_int* error_positions);

void tokenize(struct String *s, struct Vector* tokens);
enum EParseStatus TL1CommandParse(struct String* str_command, struct SErrorElement* errorStack);

#endif