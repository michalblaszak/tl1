
//#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "include\TL1_Command.h"
#include "include\global.h"
#include "include\tl_vector.h"

// /***************************************************************************/
char *parseErrorNames[] = {
    "PARSE_INITIAL",
    "PARSE_OK",
    "PARSE_ERROR_EXPECTED_LETTER",
    "PARSE_ERROR_EXPECTED_DIGIT",
    "PARSE_ERROR_EXPECTED_IDENT",
    "PARSE_ERROR_EXPECTED_SYM_NAME",
    "PARSE_ERROR_EXPECTED_ALPHANUM",
    "PARSE_ERROR_EXPECTED_DECNUM",
    "PARSE_ERROR_EXPECTED_CHAR",
    "PARSE_ERROR_EXPECTED_ARITHEXPR",
    "PARSE_ERROR_EXPECTED_HEXDIGIT",
    "PARSE_ERROR_EXPECTED_OCTDIGIT",
    "PARSE_ERROR_EXPECTED_BINDIGIT",
    "PARSE_ERROR_EXPECTED_HEXNUM",
    "PARSE_ERROR_EXPECTED_OCTNUM",
    "PARSE_ERROR_EXPECTED_BINNUM",
    "PARSE_ERROR_EXPECTED_KEYEDNUM", 
    "PARSE_ERROR_EXPECTED_INTEGER",
    "PARSE_ERROR_EXPECTED_TEXTSTR",
    "PARSE_ERROR_EXPECTED_STRINGCHAR",
    "PARSE_ERROR_SEQUENCE_NO_MATCH",
    "PARSE_ERROR_EXPECTED_CMDCODE",
    "PARSE_ERROR_EXPECTED_TID",
    "PARSE_ERROR_EXPECTED_AID",
    "PARSE_ERROR_EXPECTED_AIDVALUE",
    "PARSE_ERROR_EXPECTED_AIDNAME",
    "PARSE_ERROR_EXPECTED_VALUECOMPLEX",
    "PARSE_ERROR_EXPECTED_INPUTMESSAGE",
    "PARSE_ERROR_EXPECTED_PARAMVALUE",
    "PARSE_ERROR_EXPECTED_PARAMVALUECOMPLEX",
    "PARSE_ERROR_EXPECTED_SEQCHAIN",
    "PARSE_ERROR_EXPECTED_SIMPLESEQ",
    "PARSE_ERROR_EXPECTED_COMPCHAIN",
    "PARSE_ERROR_EXPECTED_CMPSEQ",
    "PARSE_ERROR_EXPECTED_LINKEDSEQ",
    "PARSE_ERROR_EXPECTED_CMPARG",
    "PARSE_ERROR_EXPECTED_CTAG",
    "PARSE_ERROR_EXPECTED_PAYLOADBLOCK",
    "PARSE_ERROR_EXPECTED_POSDEFPARAMSEQ",
    "PARSE_ERROR_EXPECTED_POSDEFPARAM",
    "PARSE_ERROR_EXPECTED_NAMEDEFPARAMSEQ",
    "PARSE_ERROR_EXPECTED_NAMEDEFPARAM",
    "PARSE_ERROR_EXPECTED_INFOUNITCLASS",
    "PARSE_EOF",
    "PARSE_ERROR_COMMANDCODE",
    "PARSE_ERROR_TID",
    "PARSE_ERROR_AID",
    "PARSE_ERROR_CTAG",
    "PARSE_ERROR_GENERALBLOCK",
    "PARSE_ERROR_PAYLOADBLOCK",
    "PARSE_ERROR_EXPECTED_COLON",
    "PARSE_ERROR_UNKNOWN",
    "PARSE_ERROR_EXPECTED_QUOTATION",
    "PARSE_ERROR_UNEXPECTEDCHARACTERS",
    "PARSE_ERROR_EXPECTED_SYMNAME_PLUS_HASH_PERCENT",
    "PARSE_ERROR_EXPECTED_SYMNAME_DIGIT",
    "PARSE_ERROR_EXPECTED_SYMNAME_LETTER",
    "PARSE_ERROR_EXPECTED_ALPHANUM_DIGIT_LETTER",
    "PARSE_ERROR_EXPECTED_BRACKET_OPEN",
    "PARSE_ERROR_EXPECTED_BRACKET_CLOSE",
    "PARSE_ERROR_EXPECTED_KEYEDDIGIT",
    "PARSE_ERROR_KEYEDNUM_UNEXPECTEDSYMBOL",
    "PARSE_ERROR_EXPECTED_CMPARG_MINUS",
    "PARSE_ERROR_EXPECTED_SEQCHAIN_COMMA_GROUP_COLON_SEMICOLON",
    "PARSE_ERROR_EXPECTED_PARAMVALUE_COMMA_COLON_SEMICOLON",
    "PARSE_ERROR_EXPECTED_EQUAL",
    "PARSE_ERROR_EXPECTED_CTAG_COLON_SEMICOLON",
    "PARSE_ERROR_EXPECTED_PARAMNAME",
    "PARSE_ERROR_EXPECTED_PAYLOAD_COLON_SEMICOLON",
    "PARSE_ERROR_EXPECTED_SEQCHAIN_SIMPLESEQ_COMMA_GROUP_COLON_SEMICOLON",
    "PARSE_ERROR_EXPECTED_SEQCHAIN_CMPCHAIN_COMMA_GROUP_COLON_SEMICOLON",
    "PARSE_ERROR_EXPECTED_PAYLOAD_POSDEF_COLON_SEMICOLON",
    "PARSE_ERROR_EXPECTED_PAYLOAD_NAMEDEF_COLON_SEMICOLON"
};

/******************************************************/
/* Error handling globals                             */
/******************************************************/

void errorStackInit(struct SErrorElement* e) {
    e->error = PARSE_INITIAL;
    e->pos = -1;

    vector_init(&e->suberrors);
}

// Error Element destructor
void free_errorElement(void* t) {
    vector_free(&((struct SErrorElement*)t)->suberrors);
}

void errorStack_freeInit(struct SErrorElement* e) {
    free_errorElement(e);
    errorStackInit(e);
}

void errorStack_GetPositions(struct SErrorElement* errorStack, struct Array_int* error_positions) {
    struct SErrorElement* element = (struct SErrorElement*)vector_first(&errorStack->suberrors);

    if (element == NULL) { // Is it a leaf?
        addSorted_int(error_positions, errorStack->pos);
    }

    while(element) {
        errorStack_GetPositions(element, error_positions);

        element = (struct SErrorElement*)vector_next(&errorStack->suberrors);
    }    
}

struct SErrorElement* allocateErrorElement() {
    struct SErrorElement* element = malloc(sizeof(struct SErrorElement));
    errorStackInit(element);
    return element;
}

void initErrorElement(struct SErrorElement* e, enum EParseStatus error, int pos) {
    e->error = error;
    e->pos = pos;
    vector_init(&e->suberrors);
}

void setErrorElement(struct SErrorElement* e, enum EParseStatus error, int pos) {
    e->error = error;
    e->pos = pos;
}

void addCreate_errorElement(struct SErrorElement* e, enum EParseStatus error, int pos) {
    struct SErrorElement* element = allocateErrorElement();

    initErrorElement(element, error, pos);

    vector_append(&e->suberrors, (void*)element, free_errorElement);
}

void add_errorElement(struct SErrorElement* parent, struct SErrorElement* child) {
    vector_append(&parent->suberrors, (void*)child, free_errorElement);
}

void print_errorStack(struct SErrorElement* e, int level) {
    printf("%*s[%d] %s\n", level, "", e->pos, parseErrorNames[e->error]);

    struct SErrorElement* element = (struct SErrorElement*)vector_first(&e->suberrors);

    while(element) {
        print_errorStack(element, level+2);

        element = (struct SErrorElement*)vector_next(&e->suberrors);
    }
}

/*********************************************************************/
/* Parse and tokenize procedures                                     */
/*********************************************************************/

enum ECheckCharMode { NO_ADVANCE, ADVANCE_IF_OK };

enum EParseStatus _checkChar(struct String *s, int *pos, char c, enum ECheckCharMode mode) {
    if (*pos == s->len)
        return PARSE_EOF;

    if (toUpper(s->str[*pos]) == c) {
        if (mode == ADVANCE_IF_OK) {
            (*pos)++;
        }

        return PARSE_OK;
    } else {
        return PARSE_ERROR_EXPECTED_CHAR;
    }
}

enum EParseStatus _check_sequence(struct String *s, int *pos, char* str, int len, enum ECheckCharMode mode) {
    int j = *pos; // Loop iterator

    if (len == 0) // Nothing to check
        return PARSE_OK;

    if (len > s->len - j) {    // the 'str' string longer then available characters in 's'. Also covers rhe case when the parsed string 's' has no more characters to parse ('*pos' points to the end of 's').
        return PARSE_ERROR_SEQUENCE_NO_MATCH;
    }

    for (int i = 0; i < len; i++, j++) {
        if (s->str[j] != str[i]) {
            return PARSE_ERROR_SEQUENCE_NO_MATCH;
        }
    }
    
    if (mode == ADVANCE_IF_OK) {
        *pos = j;
    }

    return PARSE_OK;
}

enum EUnitClass {
    IDENTIFIER,
    SYM_NAME,
    ALPHANUM,
    DEC_NUM,
    ARITH_EXPR,
    HEXNUM,
    OCTNUM,
    BINNUM,
    KEYEDNUM,
    NUM_STR,
    INTEGER,
    TEXTSTR
};

/***********************/
/****   Tokenizer   ****/
/***********************/

enum ETokens {
    TOKEN_EOF,
    TOKEN_EMPTY,
    TOKEN_WRONG,
    TOKEN_LITERAL,
    TOKEN_RANGE_COMP,       // &&-
    TOKEN_RANGE,            // &&
    TOKEN_GROUP_COMP,       // &-
    TOKEN_GROUP,            // &
    TOKEN_INCREMENT,        // .++.
    TOKEN_COLON,            // :
    TOKEN_SEMICOLON,        // ;
    TOKEN_COMMA,            // ,
    TOKEN_EQUAL,            // =
    TOKEN_STAR,             // *
    TOKEN_HASH,             // #
    TOKEN_PLUS,             // +
    TOKEN_MINUS,            // -
    TOKEN_PERCENT,          // %
    TOKEN_DOT,              // .
    TOKEN_BRACKET_OPEN,     // (
    TOKEN_BRACKET_CLOSE     // )
};

struct Token {
    enum ETokens code;
    struct String str;
    struct String converted_str;
    int pos;
};

struct Token* init_token(enum ETokens token_code, char* s, int len, int pos) {
    struct Token* token = malloc(sizeof(struct Token));

    token->code = token_code;
    token->pos = pos;

    initString(&token->str, 0);
    setCString(&token->str, s, len);

    initString(&token->converted_str, 0);
    setCString(&token->converted_str, s, len);

    return token;
}

// Retrieve the token from the current iterator
enum ETokens getTokenCode(struct Vector* v) {
    if (v == NULL) return TOKEN_EOF;
    if (v->iterator == NULL) return TOKEN_EOF;
    if (v->iterator->element == NULL) return TOKEN_WRONG;

    return ((struct Token*)v->iterator->element)->code;
}

struct String* getTokenString(struct Vector* v) {
    if (v == NULL) return NULL;
    if (v->iterator == NULL) return NULL;
    if (v->iterator->element == NULL) return NULL;

    return &((struct Token*)v->iterator->element)->str;
}

enum ETokens getTokenNextCode(struct Vector* v) {
    if (v == NULL) return TOKEN_EOF;
    if (v->iterator == NULL) return TOKEN_EOF;
    if (v->iterator->next == NULL) return TOKEN_EOF;
    if (v->iterator->next->element == NULL) return TOKEN_WRONG;

    return ((struct Token*)v->iterator->next->element)->code;
}

int getTokenNextPos(struct Vector* v) {
    if (v == NULL) return -1;
    if (v->iterator == NULL) return -1;
    if (v->iterator->next == NULL) return -1;
    if (v->iterator->next->element == NULL) return -1;

    return ((struct Token*)v->iterator->next->element)->pos;
}

// Token destructor in a vector
void free_token(void* t) {
    freeString(&((struct Token*)t)->str);
    freeString(&((struct Token*)t)->converted_str);
    free(t);
}

void print_tokens(struct Vector* tokens) {
    struct Token* t = (struct Token*)vector_first(tokens);

    while(t) {
        switch (t->code) {
            case TOKEN_EMPTY:
                printf("[%d] EMPTY\n", t->pos);
                break;
            case TOKEN_LITERAL:
                printf("[%d] LITERAL: %.*s\n", t->pos, t->str.len, t->str.str);
                break;
            case TOKEN_RANGE_COMP:
                printf("[%d] &&-\n", t->pos);
                break;
            case TOKEN_RANGE:
                printf("[%d] &&\n", t->pos);
                break;
            case TOKEN_GROUP_COMP:
                printf("[%d] &-\n", t->pos);
                break;
            case TOKEN_GROUP:
                printf("[%d] &\n", t->pos);
                break;
            case TOKEN_INCREMENT:
                printf("[%d] .++.\n", t->pos);
                break;
            case TOKEN_COLON:
                printf("[%d] :\n", t->pos);
                break;
            case TOKEN_SEMICOLON:
                printf("[%d] ;\n", t->pos);
                break;
            case TOKEN_COMMA:
                printf("[%d] ,\n", t->pos);
                break;
            case TOKEN_EQUAL:
                printf("[%d] =\n", t->pos);
                break;
            case TOKEN_STAR:
                printf("[%d] *\n", t->pos);
                break;
            case TOKEN_HASH:
                printf("[%d] #\n", t->pos);
                break;
            case TOKEN_PLUS:
                printf("[%d] +\n", t->pos);
                break;
            case TOKEN_MINUS:
                printf("[%d] -\n", t->pos);
                break;
            case TOKEN_PERCENT:
                printf("[%d] %%\n", t->pos);
                break;
            case TOKEN_DOT:
                printf("[%d] .\n", t->pos);
                break;
            case TOKEN_BRACKET_OPEN:
                printf("[%d] (\n", t->pos);
                break;
            case TOKEN_BRACKET_CLOSE:
                printf("[%d] )\n", t->pos);
                break;
            case TOKEN_EOF:
                printf("[%d] EOF\n", t->pos);
                break;
            case TOKEN_WRONG:
                printf("[%d] Wrong\n", t->pos);
                break;
        }

        t = (struct Token*)vector_next(tokens);
    }
}

void putToken(struct Vector* tokens, enum ETokens token_code, int pos) {
    vector_append(tokens, init_token(token_code, NULL, 0, pos), free_token);
}

void putTokenS(struct Vector* tokens, enum ETokens token_code, struct String *s, int pos, int len) {
    if (len == 0)
        vector_append(tokens, init_token(TOKEN_EMPTY, NULL, 0, pos), free_token);
    else 
        vector_append(tokens, init_token(token_code, s->str+pos, len, pos), free_token);
}

enum ETokenState {
    TOKEN_STATE_PLAIN,
    TOKEN_STATE_QUOTATION,
    TOKEN_STATE_ESCAPE_IN_QUOTATION
};

void tokenize(struct String *s, struct Vector* tokens) {
    enum ETokenState state = TOKEN_STATE_PLAIN;
    int start_token = 0;
    int i = 0;

    for(i=0; i<s->len; i++) {
        if (state == TOKEN_STATE_PLAIN) {
            if (_checkChar(s, &i, '"', NO_ADVANCE) == PARSE_OK) {
                state = TOKEN_STATE_QUOTATION;
            } else if (_check_sequence(s, &i, "&&-", 3, NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_RANGE_COMP, i);
                i+=2;   // The 3rd will be added by the 'for' loop
                start_token = i+1;
            } else if (_check_sequence(s, &i, "&&", 2, NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_RANGE, i);
                i++;    // The 2nd will be added by the 'for' loop
                start_token = i+1;
            } else if (_check_sequence(s, &i, "&-", 2, NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_GROUP_COMP, i);
                i++;       // The 2nd will be added by the 'for' loop
                start_token = i+1;
            } else if (_checkChar(s, &i, '&', NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_GROUP, i);
                start_token = i+1;
            } else if (_check_sequence(s, &i, ".++.", 4, NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_INCREMENT, i);
                i+=3;   // The 4TH will be added by the 'for' loop
                start_token = i+1;
            } else if (_checkChar(s, &i, ':', NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_COLON, i);
                start_token = i+1;
            } else if (_checkChar(s, &i, ';', NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_SEMICOLON, i);
                start_token = i+1;
            } else if (_checkChar(s, &i, ',', NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_COMMA, i);
                start_token = i+1;
            } else if (_checkChar(s, &i, '=', NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_EQUAL, i);
                start_token = i+1;
            } else if (_checkChar(s, &i, '*', NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_STAR, i);
                start_token = i+1;
            } else if (_checkChar(s, &i, '#', NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_HASH, i);
                start_token = i+1;
            } else if (_checkChar(s, &i, '+', NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_PLUS, i);
                start_token = i+1;
            } else if (_checkChar(s, &i, '-', NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_MINUS, i);
                start_token = i+1;
            } else if (_checkChar(s, &i, '%', NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_PERCENT, i);
                start_token = i+1;
            } else if (_checkChar(s, &i, '(', NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_BRACKET_OPEN, i);
                start_token = i+1;
            } else if (_checkChar(s, &i, ')', NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_BRACKET_CLOSE, i);
                start_token = i+1;
            } else if (_checkChar(s, &i, '.', NO_ADVANCE) == PARSE_OK) {
                putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
                putToken(tokens, TOKEN_DOT, i);
                start_token = i+1;
            } else {
                ; // The character belongs to the literal. Do nothing.
            }
        } else if (state == TOKEN_STATE_QUOTATION) {
            if (_checkChar(s, &i, '"', NO_ADVANCE) == PARSE_OK) {
                state = TOKEN_STATE_PLAIN;
            } else if (_checkChar(s, &i, '\\', NO_ADVANCE) == PARSE_OK) {
                state = TOKEN_STATE_ESCAPE_IN_QUOTATION;
            } else {
                ; // The character belongs to the quotation
            }
        } else if (state == TOKEN_STATE_ESCAPE_IN_QUOTATION) {
            state = TOKEN_STATE_QUOTATION;
            // This is the escaped character
        }
    } // ~for

    putTokenS(tokens, TOKEN_LITERAL, s, start_token, i-start_token);
    putToken(tokens, TOKEN_EOF, i);
}

/***********************/
/****     Parser    ****/
/***********************/

enum EParseStatus regular_string_char(char c) {
    if (c >= ' ' && c <= '~' && c != '"' && c != '\\')
        return PARSE_OK;
    else
        return PARSE_ERROR_EXPECTED_STRINGCHAR;
}

enum EParseStatus let(char c) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
        return PARSE_OK;
    else
        return PARSE_ERROR_EXPECTED_LETTER;
}

enum EParseStatus dig(char c) {
    if (c >= '0' && c <= '9')
        return PARSE_OK;
    else
        return PARSE_ERROR_EXPECTED_DIGIT;
}

enum EParseStatus hexDigit(char c) {
    if ((c >= '0' && c <= '9')
     || (c >= 'A' && c <= 'F')
     || (c >= 'a' && c <= 'f'))
        return PARSE_OK;
    else
        return PARSE_ERROR_EXPECTED_HEXDIGIT;
}

enum EParseStatus octDigit(char c) {
    if (c >= '0' && c <= '7')
        return PARSE_OK;
    else
        return PARSE_ERROR_EXPECTED_OCTDIGIT;
}

enum EParseStatus binDigit(char c) {
    if (c == '0' || c == '1')
        return PARSE_OK;
    else
        return PARSE_ERROR_EXPECTED_BINDIGIT;
}

enum EParseStatus keyedDigit(char c) {
    if ((c >= '0' && c <= '9')
     || (c >= 'A' && c <= 'D')
     || (c >= 'a' && c <= 'd')
     || c == '*'
     || c == '#')
        return PARSE_OK;
    else
        return PARSE_ERROR_EXPECTED_HEXDIGIT;
}

enum EParseStatus textStr(struct Vector* v, struct SErrorElement* errorStack) {
    int i = 0; // Loop iterator
    struct String* s;
    char c;
    enum EParseStatus ret;

    if (getTokenCode(v) != TOKEN_LITERAL) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_TEXTSTR, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_TEXTSTR;
    }

    s = getTokenString(v);

    if (s == NULL || s->len == 0) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_TEXTSTR, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_TEXTSTR;
    }

    if (_checkChar(s, &i, '"', ADVANCE_IF_OK) != PARSE_OK) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_TEXTSTR, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_TEXTSTR;
    }

    do {
        c = s->str[i];

        ret = regular_string_char(c);

        if (ret != PARSE_OK)
            ret = _check_sequence(s, &i, "\\\\", 2, ADVANCE_IF_OK);

        if (ret != PARSE_OK)
            ret = _check_sequence(s, &i, "\\\"", 2, ADVANCE_IF_OK);

        if (ret == PARSE_OK)
            i++;
    } while (ret == PARSE_OK);

    if (_checkChar(s, &i, '"', ADVANCE_IF_OK) != PARSE_OK) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_QUOTATION, ((struct Token*)v->iterator->element)->pos+i);
        return PARSE_ERROR_EXPECTED_TEXTSTR;
    }

    if (i == s->len)    // Are there any remaining characters
        return PARSE_OK;
    else {
        addCreate_errorElement(errorStack, PARSE_ERROR_UNEXPECTEDCHARACTERS, ((struct Token*)v->iterator->element)->pos+i);
        return PARSE_ERROR_EXPECTED_TEXTSTR;
    }
}

enum EParseStatus identifier(struct Vector* v, struct SErrorElement* errorStack) {
    struct String* s;
    int i = 0; // Loop iterator
    char c;

    if (getTokenCode(v) != TOKEN_LITERAL) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_IDENT, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_IDENT;
    }

    s = getTokenString(v);

    if (s == NULL || s->len == 0) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_IDENT, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_IDENT;
    }

    c = s->str[i];
    if (let(c) == PARSE_OK)
        i++;
    else {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_IDENT, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_IDENT;
    }

    for(; i<s->len; i++) {
        c = s->str[i];

        if (!(let(c) == PARSE_OK || dig(c) == PARSE_OK)) {    // We stop at the first character which is not correct.
            addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_IDENT, ((struct Token*)v->iterator->element)->pos+i);
            return PARSE_ERROR_EXPECTED_IDENT;
        }
    }

    return PARSE_OK;
}

enum EParseStatus cmd_code(struct Vector* v, struct SErrorElement* errorStack) {
    enum EParseStatus ret;
    struct VectorElement* current_token = v->iterator;
    struct SErrorElement* error = allocateErrorElement();

    if ((ret = identifier(v, error)) != PARSE_OK) {
        setErrorElement(error, PARSE_ERROR_EXPECTED_CMDCODE, ((struct Token*)v->iterator->element)->pos);
        add_errorElement(errorStack, error);
        v->iterator = current_token;
        return PARSE_ERROR_EXPECTED_CMDCODE;
    }

    vector_next(v);

    if ( getTokenCode(v) == TOKEN_MINUS ) {
        vector_next(v);

        if ((ret = identifier(v, error)) != PARSE_OK) {
            setErrorElement(error, PARSE_ERROR_EXPECTED_CMDCODE, ((struct Token*)v->iterator->element)->pos);
            add_errorElement(errorStack, error);
            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_CMDCODE;
        }

        enum ETokens token_tmp = getTokenNextCode(v);

        if (token_tmp == TOKEN_MINUS || token_tmp == TOKEN_HASH) {
            vector_next(v); // get '-' or '#'
            vector_next(v);

            if ((ret = identifier(v, error)) != PARSE_OK) {
                setErrorElement(error, PARSE_ERROR_EXPECTED_CMDCODE, ((struct Token*)v->iterator->element)->pos);
                add_errorElement(errorStack, error);
                v->iterator = current_token;
                return PARSE_ERROR_EXPECTED_CMDCODE;
            }
        }
    }

    free_errorElement(error);
    free(error);

    return PARSE_OK;
}

enum EParseStatus tid(struct Vector* v, struct SErrorElement* errorStack) {
    enum EParseStatus ret = PARSE_INITIAL;
    struct VectorElement* current_token = v->iterator;
    struct SErrorElement* error = allocateErrorElement();

    while (identifier(v, error) == PARSE_OK) {
        if(getTokenNextCode(v) == TOKEN_MINUS) {
            vector_next(v); // get '-'
        } else {
            ret = PARSE_OK;
            break;
        }
    }

    if (ret != PARSE_OK) {
        v->iterator = current_token;

        if (textStr(v, error) != PARSE_OK) {
            setErrorElement(error, PARSE_ERROR_EXPECTED_TID, ((struct Token*)v->iterator->element)->pos);
            add_errorElement(errorStack, error);
            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_TID;
        }
    }

    free_errorElement(error);
    free(error);

    return PARSE_OK;
}

enum EPrefix {
    WITH_PREFIX, NO_PREFIX
};

enum EParseStatus dec_num(struct Vector *v, enum EPrefix with_prefix, struct SErrorElement* errorStack) {
    struct VectorElement* current_token = v->iterator;
    struct String *s;
    int i = 0; // Loop iterator

    if (getTokenCode(v) != TOKEN_LITERAL) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_DECNUM, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_DECNUM;
    }

    s = getTokenString(v);

    if (s == NULL) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_DECNUM, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_DECNUM;
    }

    if (with_prefix == WITH_PREFIX)
        _check_sequence(s, &i, "D'", 2, ADVANCE_IF_OK);

    while (i < s->len) {
        char c = s->str[i];
        if (dig(c) != PARSE_OK) {
            addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_DIGIT, ((struct Token*)v->iterator->element)->pos+i);
            return PARSE_ERROR_EXPECTED_DECNUM;
        }

        i++;
    }

    if (getTokenNextCode(v) == TOKEN_DOT) {
        vector_next(v); // Get TOKEN_DOT
        vector_next(v);

        if (getTokenCode(v) != TOKEN_LITERAL) {
            addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_DIGIT, ((struct Token*)v->iterator->element)->pos);
            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_DECNUM;
        }

        s = getTokenString(v);
        if (s == NULL) {
            addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_DIGIT, ((struct Token*)v->iterator->element)->pos);
            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_DECNUM;
        }

        if (s->len == 0) {
            addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_DIGIT, ((struct Token*)v->iterator->element)->pos);
            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_DECNUM;
        }

        for (i=0; i<s->len; i++) {
            char c = s->str[i];

            if (dig(c) != PARSE_OK) {
                addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_DIGIT, ((struct Token*)v->iterator->element)->pos+i);
                v->iterator = current_token;
                return PARSE_ERROR_EXPECTED_DECNUM;
            }
        }
    }

    return PARSE_OK;
}

enum EParseStatus sym_name_literal(struct Vector *v, struct SErrorElement* errorStack) {
    int i = 0;
    struct String* s;

    if (getTokenCode(v) != TOKEN_LITERAL) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_SYM_NAME, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_SYM_NAME;
    }
        
    s = getTokenString(v);

    if (s == NULL || s->len == 0) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_SYM_NAME, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_SYM_NAME;
    }

    while (i < s->len) {
        char c = s->str[i];

        while (dig(c) == PARSE_OK) {
            i++;

            if (i == s->len) {
                addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_SYMNAME_DIGIT, ((struct Token*)v->iterator->element)->pos+1);
                return PARSE_ERROR_EXPECTED_SYM_NAME;
            }

            c = s->str[i];
        }

        if (let(c) == PARSE_OK) {
            i++;
        } else {
            addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_SYMNAME_DIGIT, ((struct Token*)v->iterator->element)->pos+1);
            return PARSE_ERROR_EXPECTED_SYM_NAME; 
        }
        
    }

    return PARSE_OK;
}

enum EParseStatus sym_name(struct Vector *v, struct SErrorElement* errorStack) {
    struct VectorElement* current_token = v->iterator;
    enum ETokens current_token_code;
    enum EParseStatus ret = PARSE_INITIAL;

    do {
        current_token_code = getTokenCode(v);

        if (current_token_code == TOKEN_LITERAL) {
            if (sym_name_literal(v, errorStack) != PARSE_OK) { // in case of error the 'errorStack' is enhanced inside 'sym_name_literal'
                v->iterator = current_token;
                return PARSE_ERROR_EXPECTED_SYM_NAME;
            }
            vector_next(v);
            current_token_code = getTokenCode(v);
        }

        if ( (current_token_code == TOKEN_PLUS)
          || (current_token_code == TOKEN_HASH)
          || (current_token_code == TOKEN_PERCENT) )
        {
            ret = PARSE_OK;
            vector_next(v);
        } else {
            if (ret == PARSE_OK) {
                return PARSE_OK;
            } else {
                addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_SYMNAME_PLUS_HASH_PERCENT, ((struct Token*)v->iterator->element)->pos);
                v->iterator = current_token;
                return PARSE_ERROR_EXPECTED_SYM_NAME;
            }
        }
    } while(1);

    return PARSE_OK;
}

enum EParseStatus alphanum(struct Vector *v, struct SErrorElement* errorStack) {
    struct String* s;

    if (getTokenCode(v) != TOKEN_LITERAL) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_ALPHANUM, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_ALPHANUM;
    }
        
    s = getTokenString(v);

    if (s == NULL || s->len == 0) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_ALPHANUM, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_ALPHANUM;
    }

    for (int i=0; i<s->len; i++) {
        char c = s->str[i];

        if (dig(c) != PARSE_OK && let(c) != PARSE_OK) {
            addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_ALPHANUM_DIGIT_LETTER, ((struct Token*)v->iterator->element)->pos+i);
            return PARSE_ERROR_EXPECTED_ALPHANUM;
        }
    }

    return PARSE_OK;
}

enum EParseStatus arith_expr(struct Vector* v, struct SErrorElement* errorStack) {
    struct VectorElement* current_token = v->iterator;
    enum ETokens current_token_code;

    if (getTokenCode(v) != TOKEN_BRACKET_OPEN) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_BRACKET_OPEN, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_ARITHEXPR;
    }

    vector_next(v);

    current_token_code = getTokenCode(v);
    if (current_token_code == TOKEN_PLUS || current_token_code == TOKEN_MINUS) {
        vector_next(v);
        current_token_code = getTokenCode(v);
    }

    if (dec_num(v, NO_PREFIX, errorStack) != PARSE_OK) { // In case of error 'dec_num' adds to 'errorStack'
        v->iterator = current_token;
        return PARSE_ERROR_EXPECTED_ARITHEXPR;
    }

    vector_next(v);

    if (getTokenCode(v) != TOKEN_BRACKET_CLOSE) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_BRACKET_CLOSE, ((struct Token*)v->iterator->element)->pos);
        v->iterator = current_token;
        return PARSE_ERROR_EXPECTED_ARITHEXPR;
    }

    return PARSE_OK;
}

enum EParseStatus hex_num(struct Vector* v, struct SErrorElement* errorStack) {
    struct String *s;
    int i = 0; // Loop iterator

    if (getTokenCode(v) != TOKEN_LITERAL) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_HEXNUM, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_HEXNUM;
    }

    s = getTokenString(v);

    if (s == NULL) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_HEXNUM, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_HEXNUM;
    }

    _check_sequence(s, &i, "H'", 2, ADVANCE_IF_OK);

    if (i == s->len) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_HEXDIGIT, ((struct Token*)v->iterator->element)->pos+i);
        return PARSE_ERROR_EXPECTED_HEXNUM;
    }

    do {
        char c = s->str[i];
        if (hexDigit(c) != PARSE_OK) {
            addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_HEXDIGIT, ((struct Token*)v->iterator->element)->pos+i);
            return PARSE_ERROR_EXPECTED_HEXNUM;
        }
        i++;
    } while(i < s->len);

    return PARSE_OK;
}

enum EParseStatus oct_num(struct Vector* v, struct SErrorElement* errorStack) {
    struct String *s;
    int i = 0; // Loop iterator

    if (getTokenCode(v) != TOKEN_LITERAL) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_OCTNUM, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_OCTNUM;
    }

    s = getTokenString(v);

    if (s == NULL) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_OCTNUM, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_OCTNUM;
    }

    _check_sequence(s, &i, "O'", 2, ADVANCE_IF_OK);

    if (i == s->len) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_OCTDIGIT, ((struct Token*)v->iterator->element)->pos+i);
        return PARSE_ERROR_EXPECTED_OCTNUM;
    }

    do {
        char c = s->str[i];
        if (octDigit(c) != PARSE_OK) {
            addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_OCTDIGIT, ((struct Token*)v->iterator->element)->pos+i);
            return PARSE_ERROR_EXPECTED_OCTNUM;
        }
        i++;
    } while(i < s->len);

    return PARSE_OK;
}

enum EParseStatus bin_num(struct Vector* v, struct SErrorElement* errorStack) {
    struct String *s;
    int i = 0; // Loop iterator

    if (getTokenCode(v) != TOKEN_LITERAL) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_BINNUM, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_BINNUM;
    }

    s = getTokenString(v);

    if (s == NULL) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_BINNUM, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_BINNUM;
    }

    _check_sequence(s, &i, "B'", 2, ADVANCE_IF_OK);

    if (i == s->len) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_BINDIGIT, ((struct Token*)v->iterator->element)->pos+i);
        return PARSE_ERROR_EXPECTED_BINNUM;
    }

    do {
        char c = s->str[i];
        if (binDigit(c) != PARSE_OK) {
            addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_BINDIGIT, ((struct Token*)v->iterator->element)->pos+i);
            return PARSE_ERROR_EXPECTED_BINNUM;
        }
        i++;
    } while(i < s->len);

    return PARSE_OK;
}

enum EParseStatus keyed_num(struct Vector* v, struct SErrorElement* errorStack) {
    struct VectorElement* current_token = v->iterator;
    struct String *s;
    int i; // Loop iterator
    int is_first = 1;
    int found = 0;
    enum ETokens token_code;

    token_code = getTokenCode(v);
    do {
        switch(token_code) {
            case TOKEN_LITERAL:
                i = 0;
                s = getTokenString(v);

                if (s == NULL) {
                    v->iterator = current_token;
                    if (is_first)
                        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_KEYEDNUM, ((struct Token*)v->iterator->element)->pos);
                    else 
                        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_KEYEDDIGIT, ((struct Token*)v->iterator->element)->pos);

                    return PARSE_ERROR_EXPECTED_KEYEDNUM;
                }

                if (is_first)
                    _check_sequence(s, &i, "K'", 2, ADVANCE_IF_OK);

                while (i < s->len) {
                    char c = s->str[i];
                    if (keyedDigit(c) != PARSE_OK) {
                        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_KEYEDDIGIT, ((struct Token*)v->iterator->element)->pos+i);
                        v->iterator = current_token;
                        return PARSE_ERROR_EXPECTED_KEYEDNUM;
                    }
                    found = 1;
                    i++;
                }
                break;
            case TOKEN_HASH:
            case TOKEN_STAR:
                found = 1;
                break;
            default:
                break;
        } // ~switch

        is_first = 0;

        if (found) {
            token_code = getTokenNextCode(v);

            if (token_code == TOKEN_LITERAL || token_code == TOKEN_HASH || token_code == TOKEN_STAR) {
                vector_next(v);
                token_code = getTokenCode(v);
            } else {
                break; // quit the loop
            }
        } else {
            addCreate_errorElement(errorStack, PARSE_ERROR_KEYEDNUM_UNEXPECTEDSYMBOL, ((struct Token*)v->iterator->element)->pos);
            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_KEYEDNUM;
        }
    } while (1);

    return PARSE_OK;
}

enum EParseStatus num_str(struct Vector* v, struct SErrorElement* errorStack) {
    struct String* s;

    if (getTokenCode(v) == TOKEN_LITERAL) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_DIGIT, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_DIGIT;
    }

    s = getTokenString(v);
    if (s == NULL || s->len == 0) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_DIGIT, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_DIGIT;
    }

    for(int i=0; i<s->len; i++) {
        if (dig(s->str[i]) != PARSE_OK) {
            addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_DIGIT, ((struct Token*)v->iterator->element)->pos);
            return PARSE_ERROR_EXPECTED_DIGIT;
        }
    }

    return PARSE_OK;
}

enum EParseStatus integer(struct Vector* v, struct SErrorElement* errorStack) {
    enum ETokens token_code;
    struct VectorElement* current_token = v->iterator;

    token_code = getTokenCode(v);
    if (token_code == TOKEN_LITERAL) {
        if (num_str(v, errorStack) != PARSE_OK) // In case of error 'num_str' adds to 'errorStack'
            return PARSE_ERROR_EXPECTED_INTEGER;
        else
            return PARSE_OK;
    }

    if (token_code != TOKEN_BRACKET_OPEN) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_INTEGER, ((struct Token*)v->iterator->element)->pos);
        return PARSE_ERROR_EXPECTED_INTEGER;
    }

    vector_next(v);
    token_code = getTokenCode(v);

    if (token_code == TOKEN_PLUS || token_code == TOKEN_MINUS) {
        vector_next(v);
        token_code = getTokenCode(v);
    }

    if (token_code != TOKEN_LITERAL) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_INTEGER, ((struct Token*)v->iterator->element)->pos);
        v->iterator = current_token;
        return PARSE_ERROR_EXPECTED_INTEGER;
    }

    // It's a literal, check if it is a num_str

    if (num_str(v, errorStack) != PARSE_OK) { // In case of error 'num_str' adds to 'errorStack'
        v->iterator = current_token;
        return PARSE_ERROR_EXPECTED_INTEGER;
    }

    vector_next(v);
    token_code = getTokenCode(v);

    if (token_code != TOKEN_BRACKET_CLOSE) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_BRACKET_CLOSE, ((struct Token*)v->iterator->element)->pos);
        v->iterator = current_token;
        return PARSE_ERROR_EXPECTED_INTEGER;
    }

    return PARSE_OK;
}

enum EParseStatus infoUnitClass(struct Vector *v, struct SErrorElement* errorStack) {
    struct SErrorElement* error = allocateErrorElement();

    if ( (identifier(v, error) == PARSE_OK)
      || (sym_name(v, error) == PARSE_OK)
      || (alphanum(v, error) == PARSE_OK)
      || (dec_num(v, WITH_PREFIX, error) == PARSE_OK)
      || (arith_expr(v, error) == PARSE_OK)
      || (hex_num(v, error) == PARSE_OK)
      || (oct_num(v, error) == PARSE_OK)
      || (bin_num(v, error) == PARSE_OK)
      || (keyed_num(v, error) == PARSE_OK)
      || (num_str(v, error) == PARSE_OK)
      || (integer(v, error) == PARSE_OK)
      || (textStr(v, error) == PARSE_OK) ) {
        free_errorElement(error);
        free(error);

        return PARSE_OK;
    }

    setErrorElement(error, PARSE_ERROR_EXPECTED_INFOUNITCLASS, ((struct Token*)v->iterator->element)->pos);
    add_errorElement(errorStack, error);

    return PARSE_ERROR_EXPECTED_INFOUNITCLASS;
}

enum EParseStatus simpleSeq(struct Vector *v, struct SErrorElement* errorStack) {
    struct VectorElement* current_token = v->iterator;
    struct SErrorElement* error = allocateErrorElement();

    if (infoUnitClass(v, error) != PARSE_OK) { // In case of error 'infoUnitClass' adds to 'erroStack'
        setErrorElement(error, PARSE_ERROR_EXPECTED_SIMPLESEQ, ((struct Token*)v->iterator->element)->pos);
        add_errorElement(errorStack, error);

        return PARSE_ERROR_EXPECTED_SIMPLESEQ;
    }

    if (getTokenNextCode(v) == TOKEN_RANGE) {
        vector_next(v); // get '&&'
        vector_next(v);
        if (infoUnitClass(v, error) != PARSE_OK) { // In case of error 'infoUnitClass' adds to 'erroStack'
            setErrorElement(error, PARSE_ERROR_EXPECTED_SIMPLESEQ, ((struct Token*)v->iterator->element)->pos);
            add_errorElement(errorStack, error);

            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_SIMPLESEQ;
        }

        if (getTokenNextCode(v) == TOKEN_INCREMENT) {
            vector_next(v); // get .++.
            vector_next(v);
            if (dec_num(v, NO_PREFIX, error) != PARSE_OK) { // In case of error 'dec_num' adds to 'erroStack'
                setErrorElement(error, PARSE_ERROR_EXPECTED_SIMPLESEQ, ((struct Token*)v->iterator->element)->pos);
                add_errorElement(errorStack, error);

                v->iterator = current_token;
                return PARSE_ERROR_EXPECTED_SIMPLESEQ;
            }
        }
    }

    free_errorElement(error);
    free(error);

    return PARSE_OK;
}

enum EParseStatus linked_seq(struct Vector *v, struct SErrorElement* errorStack) {
    struct VectorElement* current_token = v->iterator;

    if (infoUnitClass(v, errorStack) != PARSE_OK) // In case of error 'infoUnitClass' adds to 'erroStack'
        return PARSE_ERROR_EXPECTED_LINKEDSEQ;

    if (getTokenNextCode(v) == TOKEN_RANGE_COMP) {
        vector_next(v); // get '&&-'
        vector_next(v);
        if (infoUnitClass(v, errorStack) != PARSE_OK) { // In case of error 'infoUnitClass' adds to 'erroStack'
            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_LINKEDSEQ;
        }

        if (getTokenNextCode(v) == TOKEN_INCREMENT) {
            vector_next(v); // get '.++.'
            vector_next(v);
            if (dec_num(v, NO_PREFIX, errorStack) != PARSE_OK) { // In case of error 'dec_num' adds to 'erroStack'
                v->iterator = current_token;
                return PARSE_ERROR_EXPECTED_LINKEDSEQ;
            }
        }
    }

    return PARSE_OK;
}

enum EParseStatus cmp_arg(struct Vector *v, struct SErrorElement* errorStack) {
    struct VectorElement* current_token = v->iterator;
    int is_first = 1;

    if (infoUnitClass(v, errorStack) != PARSE_OK) // In case of error 'infoUnitClass' adds to 'erroStack'
        return PARSE_ERROR_EXPECTED_CMPARG;

    while (getTokenNextCode(v) == TOKEN_MINUS) {
        is_first = 0;
        vector_next(v); // get '-'
        vector_next(v);
        if (infoUnitClass(v, errorStack) != PARSE_OK) { // In case of error 'infoUnitClass' adds to 'erroStack'
            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_CMPARG;
        }
    }

    if (is_first) {
        addCreate_errorElement(errorStack, PARSE_ERROR_EXPECTED_CMPARG_MINUS, getTokenNextPos(v));
        return PARSE_ERROR_EXPECTED_CMPARG;
    } else
        return PARSE_OK;
}

enum EParseStatus cmp_seq(struct Vector *v, struct SErrorElement* errorStack) {
    struct VectorElement* current_token = v->iterator;

    if (cmp_arg(v, errorStack) != PARSE_OK) // In case of error 'cmp_arg' adds to 'erroStack'
        return PARSE_ERROR_EXPECTED_CMPSEQ;

    if (getTokenNextCode(v) == TOKEN_RANGE_COMP) {
        vector_next(v); // get '&&-'
        vector_next(v); // get next token
        if (infoUnitClass(v, errorStack) != PARSE_OK) { // In case of error 'infoUnitClass' adds to 'erroStack'
            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_CMPSEQ;
        }

        if (getTokenNextCode(v) == TOKEN_INCREMENT) {
            vector_next(v); // get '.++.'
            vector_next(v); // get next token
            if (dec_num(v, NO_PREFIX, errorStack) != PARSE_OK) { // In case of error 'dec_num' adds to 'erroStack'
                v->iterator = current_token;
                return PARSE_ERROR_EXPECTED_CMPSEQ;
            }
        }
    }

    return PARSE_OK;
}

enum EParseStatus cmp_chain(struct Vector *v, struct SErrorElement* errorStack) {
    struct VectorElement* current_token = v->iterator;
    struct SErrorElement* error = allocateErrorElement();

    if (cmp_seq(v, error) != PARSE_OK) { // In case of error 'cmp_seq' adds to 'erroStack'
        setErrorElement(error, PARSE_ERROR_EXPECTED_COMPCHAIN, ((struct Token*)v->iterator->element)->pos);
        add_errorElement(errorStack, error);

        return PARSE_ERROR_EXPECTED_COMPCHAIN;
    }

    while(getTokenNextCode(v) == TOKEN_GROUP_COMP) {
        vector_next(v); // get '&-'
        vector_next(v);
        if(linked_seq(v, error) != PARSE_OK) { // In case of error 'linked_seq' adds to 'erroStack'
            setErrorElement(error, PARSE_ERROR_EXPECTED_COMPCHAIN, ((struct Token*)v->iterator->element)->pos);
            add_errorElement(errorStack, error);

            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_COMPCHAIN;
        }
    }

    free_errorElement(error);
    free(error);

    return PARSE_OK;
}

enum EParseStatus seq_chain(struct Vector *v, struct SErrorElement* errorStack) {
    int __checkNext() {
        enum ETokens next_token;

        next_token = getTokenNextCode(v);

        return (next_token == TOKEN_COMMA
             || next_token == TOKEN_GROUP
             || next_token == TOKEN_COLON
             || next_token == TOKEN_SEMICOLON
             || next_token == TOKEN_EOF);
    }

    struct VectorElement* current_token = v->iterator;
    struct SErrorElement* error = allocateErrorElement();

    if (simpleSeq(v, error) == PARSE_OK) {
        if (__checkNext()) {
            free_errorElement(error);
            free(error);

            return PARSE_OK;
        } else {
            addCreate_errorElement(error, PARSE_ERROR_EXPECTED_SEQCHAIN_SIMPLESEQ_COMMA_GROUP_COLON_SEMICOLON, getTokenNextPos(v));
        }
    }

    if (cmp_chain(v, error) == PARSE_OK) {
        if (__checkNext()) {
            free_errorElement(error);
            free(error);

            return PARSE_OK;
        } else {
            addCreate_errorElement(error, PARSE_ERROR_EXPECTED_SEQCHAIN_CMPCHAIN_COMMA_GROUP_COLON_SEMICOLON, getTokenNextPos(v));
        }
    }

    v->iterator = current_token;
    setErrorElement(error, PARSE_ERROR_EXPECTED_SEQCHAIN, ((struct Token*)v->iterator->element)->pos);
    add_errorElement(errorStack, error);

    return PARSE_ERROR_EXPECTED_SEQCHAIN;
}

enum EParseStatus paramValueComplex(struct Vector *v, struct SErrorElement* errorStack) {
    enum EParseStatus ret = PARSE_INITIAL;
    struct VectorElement* current_token = v->iterator;

    while(seq_chain(v, errorStack) == PARSE_OK) { // In case of error 'seq_chain' adds to 'erroStack'
        if(getTokenNextCode(v) == TOKEN_GROUP) {
            vector_next(v); // get '&'
            vector_next(v); // get next token
        } else {
            ret = PARSE_OK;
            break;
        }
    }

    if (ret != PARSE_OK) {
        v->iterator = current_token;
        return PARSE_ERROR_EXPECTED_PARAMVALUECOMPLEX;
    } else {
        return PARSE_OK;
    }
}

enum EParseStatus param_value(struct Vector *v, struct SErrorElement* errorStack) {
    int __checkNext() {
        enum ETokens next_token;

        next_token = getTokenNextCode(v);

        return (next_token == TOKEN_COMMA
             || next_token == TOKEN_COLON
             || next_token == TOKEN_SEMICOLON
             || next_token == TOKEN_EOF);
    }

    struct VectorElement* current_token = v->iterator;
    struct SErrorElement* error = allocateErrorElement();

    if (paramValueComplex(v, error) == PARSE_OK) {
        if (__checkNext()) {
            free_errorElement(error);
            free(error);

            return PARSE_OK;
        }
    }

    v->iterator = current_token;
    setErrorElement(error, PARSE_ERROR_EXPECTED_PARAMVALUE_COMMA_COLON_SEMICOLON, ((struct Token*)v->iterator->element)->pos);
    add_errorElement(errorStack, error);

    return PARSE_ERROR_EXPECTED_PARAMVALUE;
}

enum EParseStatus aid_unit(struct Vector *v, struct SErrorElement* errorStack) {
    struct VectorElement* current_token = v->iterator;
    struct SErrorElement* error = allocateErrorElement();

    if (getTokenNextCode(v) == TOKEN_EQUAL) {
        if (identifier(v, error) != PARSE_OK) {
            setErrorElement(error, PARSE_ERROR_EXPECTED_AIDNAME, ((struct Token*)v->iterator->element)->pos);
            add_errorElement(errorStack, error);

            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_AIDNAME;
        }

        vector_next(v);
        if (getTokenCode(v) != TOKEN_EQUAL) {
            setErrorElement(error, PARSE_ERROR_EXPECTED_EQUAL, ((struct Token*)v->iterator->element)->pos);
            add_errorElement(errorStack, error);

            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_AIDNAME;
        }

        vector_next(v);
        if (param_value(v, error) != PARSE_OK) {
            setErrorElement(error, PARSE_ERROR_EXPECTED_AIDVALUE, ((struct Token*)v->iterator->element)->pos);
            add_errorElement(errorStack, error);

            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_AIDNAME;
        }
    } else {
        if (getTokenCode(v) == TOKEN_EMPTY) {
            free_errorElement(error);
            free(error);
            return PARSE_OK;
        }

        if (param_value(v, error) != PARSE_OK) {
            setErrorElement(error, PARSE_ERROR_EXPECTED_AIDVALUE, ((struct Token*)v->iterator->element)->pos);
            add_errorElement(errorStack, error);

            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_AIDNAME;
        }
    }

    free_errorElement(error);
    free(error);

    return PARSE_OK;
}

enum EParseStatus aid(struct Vector *v, struct SErrorElement* errorStack) {
    enum EParseStatus ret = PARSE_INITIAL;
    struct VectorElement* current_token = v->iterator;

    while(aid_unit(v, errorStack) == PARSE_OK) { // In case of error 'aid_unit' adds to 'erroStack'
        if(getTokenNextCode(v) == TOKEN_COMMA) {
            vector_next(v); // get ','
            vector_next(v);
        } else {
            ret = PARSE_OK;
            break;
        }
    }

    if (ret != PARSE_OK) {
        v->iterator = current_token;
        return PARSE_ERROR_EXPECTED_AID;
    } else {
        return PARSE_OK;
    }
}

enum EParseStatus ctag(struct Vector *v, struct SErrorElement* errorStack) {
    int __checkNext() {
        enum ETokens next_token;

        next_token = getTokenNextCode(v);

        return (next_token == TOKEN_COLON
             || next_token == TOKEN_SEMICOLON
             || next_token == TOKEN_EOF);
    }

    struct VectorElement* current_token = v->iterator;
    struct SErrorElement* error = allocateErrorElement();
    
    if (identifier(v, error) == PARSE_OK) { // In case of error 'identifier' adds to 'erroStack'
        if (__checkNext()) {
            free_errorElement(error);
            free(error);

            return PARSE_OK;
        }
    }

    if (dec_num(v, WITH_PREFIX, error) == PARSE_OK) { // In case of error 'dec_num' adds to 'erroStack'
        if (__checkNext()) {
            free_errorElement(error);
            free(error);

            return PARSE_OK;
        }
    }

    v->iterator = current_token;
    setErrorElement(error, PARSE_ERROR_EXPECTED_CTAG_COLON_SEMICOLON, ((struct Token*)v->iterator->element)->pos);
    add_errorElement(errorStack, error);

    return PARSE_ERROR_EXPECTED_CTAG;
}

enum EParseStatus general_block(struct Vector *v, struct SErrorElement* errorStack) {
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    return PARSE_OK;
}

enum EParseStatus pos_def_param(struct Vector *v, struct SErrorElement* errorStack) {
    struct VectorElement* current_token = v->iterator;
    struct SErrorElement* error = allocateErrorElement();

    if (getTokenNextCode(v) == TOKEN_EQUAL) {
        if (identifier(v, error) != PARSE_OK) { // In case of error 'identifier' adds to 'erroStack'
            setErrorElement(error, PARSE_ERROR_EXPECTED_PARAMNAME, ((struct Token*)v->iterator->element)->pos);
            add_errorElement(errorStack, error);

            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_POSDEFPARAM;
        }

        vector_next(v);
        if (getTokenCode(v) != TOKEN_EQUAL) {
            setErrorElement(error, PARSE_ERROR_EXPECTED_EQUAL, ((struct Token*)v->iterator->element)->pos);
            add_errorElement(errorStack, error);

            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_POSDEFPARAM;
        }

        vector_next(v);
        if (param_value(v, error) != PARSE_OK) {
            setErrorElement(error, PARSE_ERROR_EXPECTED_PARAMVALUE, ((struct Token*)v->iterator->element)->pos);
            add_errorElement(errorStack, error);

            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_POSDEFPARAM;
        }
    } else {
        if (getTokenCode(v) == TOKEN_EMPTY) {
            free_errorElement(error);
            free(error);

            return PARSE_OK;
        }

        if (param_value(v, error) != PARSE_OK) {
            setErrorElement(error, PARSE_ERROR_EXPECTED_PARAMVALUE, ((struct Token*)v->iterator->element)->pos);
            add_errorElement(errorStack, error);

            v->iterator = current_token;
            return PARSE_ERROR_EXPECTED_POSDEFPARAM;
        }
    }

    free_errorElement(error);
    free(error);

    return PARSE_OK;
}

enum EParseStatus pos_def_param_seq(struct Vector *v, struct SErrorElement* errorStack) {
    enum EParseStatus ret = PARSE_INITIAL;
    struct VectorElement* current_token = v->iterator;
    struct SErrorElement* error = allocateErrorElement();

    while (pos_def_param(v, error) == PARSE_OK) { // In case of error 'pos_def_param' adds to 'erroStack'
        if(getTokenNextCode(v) == TOKEN_COMMA) {
            vector_next(v); // get ','
            vector_next(v);
        } else {
            ret = PARSE_OK;
            break;
        }
    }

    if (ret != PARSE_OK) {
        setErrorElement(error, PARSE_ERROR_EXPECTED_POSDEFPARAMSEQ, ((struct Token*)v->iterator->element)->pos);
        add_errorElement(errorStack, error);

        v->iterator = current_token;
        return PARSE_ERROR_EXPECTED_POSDEFPARAMSEQ;
    } else {
        free_errorElement(error);
        free(error);

        return PARSE_OK;
    }
}

enum EParseStatus name_def_param(struct Vector *v, struct SErrorElement* errorStack) {
    struct VectorElement* current_token = v->iterator;
    struct SErrorElement* error = allocateErrorElement();

    if (identifier(v, error) != PARSE_OK) {
        setErrorElement(error, PARSE_ERROR_EXPECTED_PARAMNAME, ((struct Token*)v->iterator->element)->pos);
        add_errorElement(errorStack, error);

        return PARSE_ERROR_EXPECTED_NAMEDEFPARAM;
    }

    vector_next(v);

    if (getTokenCode(v) != TOKEN_EQUAL) {
        setErrorElement(error, PARSE_ERROR_EXPECTED_EQUAL, ((struct Token*)v->iterator->element)->pos);
        add_errorElement(errorStack, error);

        v->iterator = current_token;
        return PARSE_ERROR_EXPECTED_NAMEDEFPARAM;
    }

    vector_next(v);

    if (param_value(v, error) != PARSE_OK) {
        setErrorElement(error, PARSE_ERROR_EXPECTED_PARAMVALUE, ((struct Token*)v->iterator->element)->pos);
        add_errorElement(errorStack, error);

        v->iterator = current_token;
        return PARSE_ERROR_EXPECTED_NAMEDEFPARAM;
    }

    free_errorElement(error);
    free(error);

    return PARSE_OK;
}

enum EParseStatus name_def_param_seq(struct Vector *v, struct SErrorElement* errorStack) {
    enum EParseStatus ret = PARSE_INITIAL;
    struct VectorElement* current_token = v->iterator;
    struct SErrorElement* error = allocateErrorElement();

    while (name_def_param(v, error) == PARSE_OK) { // In case of error 'name_def_param' adds to 'erroStack'
        if(getTokenNextCode(v) == TOKEN_COMMA) {
            vector_next(v); // get ','
            vector_next(v);
        } else {
            ret = PARSE_OK;
            break;
        }
    }

    if (ret != PARSE_OK) {
        setErrorElement(error, PARSE_ERROR_EXPECTED_NAMEDEFPARAMSEQ, ((struct Token*)v->iterator->element)->pos);
        add_errorElement(errorStack, error);

        v->iterator = current_token;
        return PARSE_ERROR_EXPECTED_NAMEDEFPARAMSEQ;
    } else {
        free_errorElement(error);
        free(error);

        return PARSE_OK;
    }
}

enum EParseStatus payload_block(struct Vector *v, struct SErrorElement* errorStack) {
    int __checkNext() {
        enum ETokens next_token;

        next_token = getTokenNextCode(v);

        return (next_token == TOKEN_COLON
                || next_token == TOKEN_SEMICOLON
                || next_token == TOKEN_EOF);
    }

    struct VectorElement* current_token = v->iterator;
    struct SErrorElement* error = allocateErrorElement();

    if (pos_def_param_seq(v, error) == PARSE_OK) {
        if (__checkNext()) {
            free_errorElement(error);
            free(error);

            return PARSE_OK;
        } else {
            addCreate_errorElement(error, PARSE_ERROR_EXPECTED_PAYLOAD_POSDEF_COLON_SEMICOLON, ((struct Token*)v->iterator->element)->pos);
        }
    }

    if (name_def_param_seq(v, error) == PARSE_OK) {
        if (__checkNext()) {
            free_errorElement(error);
            free(error);

            return PARSE_OK;
        } else {
            addCreate_errorElement(error, PARSE_ERROR_EXPECTED_PAYLOAD_NAMEDEF_COLON_SEMICOLON, ((struct Token*)v->iterator->element)->pos);
        }
    }

    v->iterator = current_token;
    setErrorElement(error, PARSE_ERROR_EXPECTED_PAYLOADBLOCK, ((struct Token*)v->iterator->element)->pos);
    add_errorElement(errorStack, error);

    return PARSE_ERROR_EXPECTED_PAYLOADBLOCK;
}

enum EParseStatus parse(struct Vector* tokens, struct SErrorElement* errorStack) {
    enum ETokens token_code;

    vector_first(tokens); // cmd code
    enum EParseStatus ret;

    // cmd-code
    ret = cmd_code(tokens, errorStack);

    if (ret != PARSE_OK) {
        setErrorElement(errorStack, PARSE_ERROR_COMMANDCODE, ((struct Token*)vector_get(tokens))->pos);
        return PARSE_ERROR_EXPECTED_INPUTMESSAGE;
    }

    if (getTokenNextCode(tokens) != TOKEN_COLON) {
        setErrorElement(errorStack, PARSE_ERROR_EXPECTED_COLON, ((struct Token*)vector_get(tokens))->pos);
        return PARSE_ERROR_EXPECTED_INPUTMESSAGE;
    }

    vector_next(tokens); // ':'
    vector_next(tokens); // tid

    // [tid]
    if (getTokenCode(tokens) != TOKEN_EMPTY) {
        ret = tid(tokens, errorStack);

        if (ret != PARSE_OK) {
            setErrorElement(errorStack, PARSE_ERROR_TID, ((struct Token*)vector_get(tokens))->pos);
            return PARSE_ERROR_EXPECTED_INPUTMESSAGE;
        }
    }

    if (getTokenNextCode(tokens) != TOKEN_COLON) {
        setErrorElement(errorStack, PARSE_ERROR_EXPECTED_COLON, ((struct Token*)vector_get(tokens))->pos);
        return PARSE_ERROR_EXPECTED_INPUTMESSAGE;
    }

    vector_next(tokens); // ':'
    vector_next(tokens); // aid

    // [aid]
    ret = aid(tokens, errorStack);

    if (ret != PARSE_OK) {
        setErrorElement(errorStack, PARSE_ERROR_AID, ((struct Token*)vector_get(tokens))->pos);
        return PARSE_ERROR_EXPECTED_INPUTMESSAGE;
    }

    if (getTokenNextCode(tokens) != TOKEN_COLON) {
        setErrorElement(errorStack, PARSE_ERROR_EXPECTED_COLON, ((struct Token*)vector_get(tokens))->pos);
        return PARSE_ERROR_EXPECTED_INPUTMESSAGE;
    }

    vector_next(tokens); // ':'
    vector_next(tokens); // ctag

    // ctag
    ret = ctag(tokens, errorStack);

    if (ret != PARSE_OK) {
        setErrorElement(errorStack, PARSE_ERROR_CTAG, ((struct Token*)vector_get(tokens))->pos);
        return PARSE_ERROR_EXPECTED_INPUTMESSAGE;
    }

    token_code = getTokenNextCode(tokens);

    if (token_code == TOKEN_SEMICOLON) {
        setErrorElement(errorStack, PARSE_OK, ((struct Token*)vector_get(tokens))->pos);
        return PARSE_OK;
    }

    if (token_code != TOKEN_COLON) {
        setErrorElement(errorStack, PARSE_ERROR_EXPECTED_COLON, ((struct Token*)vector_get(tokens))->pos);
        return PARSE_ERROR_EXPECTED_INPUTMESSAGE;
    }

    vector_next(tokens); // ':' or ';' or EOF
    vector_next(tokens); // general block

    // [general block]
    if (getTokenCode(tokens) != TOKEN_EMPTY) {
        ret = general_block(tokens, errorStack);

        if (ret != PARSE_OK) {
            setErrorElement(errorStack, PARSE_ERROR_GENERALBLOCK, ((struct Token*)vector_get(tokens))->pos);
            return PARSE_ERROR_EXPECTED_INPUTMESSAGE;
        }
    }

    // [payload]
    do {
        token_code = getTokenNextCode(tokens);

        if (token_code == TOKEN_SEMICOLON) {
            setErrorElement(errorStack, PARSE_OK, ((struct Token*)vector_get(tokens))->pos);
            return PARSE_OK;
        }

        if (token_code != TOKEN_COLON) {
            setErrorElement(errorStack, PARSE_ERROR_EXPECTED_COLON, ((struct Token*)vector_get(tokens))->pos);
            return PARSE_ERROR_EXPECTED_INPUTMESSAGE;
        }

        vector_next(tokens); // ':' or ';' or EOF
        vector_next(tokens); // payload

        ret = payload_block(tokens, errorStack);

        if (ret != PARSE_OK) {
            setErrorElement(errorStack, PARSE_ERROR_PAYLOADBLOCK, ((struct Token*)vector_get(tokens))->pos);
            return PARSE_ERROR_EXPECTED_INPUTMESSAGE;
        }
    } while (1);

    // In theory we should come to this place only with ret == PARSE_OK
    setErrorElement(errorStack, ret, ((struct Token*)vector_get(tokens))->pos);
    return ret;
}

enum EParseStatus TL1CommandParse(struct String* str_command, struct SErrorElement* errorStack) {
    enum EParseStatus ret;
    struct Vector tokens;

    vector_init(&tokens);

    if (str_command == NULL) {
        initErrorElement(errorStack, PARSE_ERROR_EXPECTED_INPUTMESSAGE, 0);
        return PARSE_ERROR_EXPECTED_INPUTMESSAGE;
    }

    tokenize(str_command, &tokens);
    print_tokens(&tokens);

    ret = parse(&tokens, errorStack);

    printf("===================\nError Stack\n");
    print_errorStack(errorStack, 0);

    vector_free(&tokens);

    return ret;
}

void initCommandDefinitions