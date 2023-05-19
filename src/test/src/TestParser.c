#include <stdio.h>
#include <string.h>
#include "..\cutest-1.5\CuTest.h"
#include "..\..\include\tl_string.h"
#include "..\..\include\TL1_Command.h"

// void testIdent(CuTest *tc) {
// 	struct String input_command;
// 	enum EParseStatus ret = PARSE_INITIAL;
// 	int pos;
	
// 	printf("Test identifier ...\n");

// 	initString(&input_command, 0);
	
// 	printf("Step 1\n");
// 	setCString(&input_command, "", 0);
// 	pos = 0;
// 	ret = identifier(&input_command, &pos);
// 	CuAssertIntEquals(tc, PARSE_ERROR_EXPECTED_IDENT, ret);
// 	CuAssertIntEquals(tc, 0, pos);
	
// 	printf("Step 2\n");
// 	setCString(&input_command, "a", 1);
// 	pos = 0;
// 	ret = identifier(&input_command, &pos);
// 	CuAssertIntEquals(tc, PARSE_OK, ret);
// 	CuAssertIntEquals(tc, 1, pos);
	
// 	printf("Step 3\n");
// 	setCString(&input_command, "get", 3);
// 	pos = 0;
// 	ret = identifier(&input_command, &pos);
// 	CuAssertIntEquals(tc, PARSE_OK, ret);
// 	CuAssertIntEquals(tc, 3, pos);
	
// 	printf("Step 4\n");
// 	setCString(&input_command, "1", 1);
// 	pos = 0;
// 	ret = identifier(&input_command, &pos);
// 	CuAssertIntEquals(tc, PARSE_ERROR_EXPECTED_IDENT, ret);
// 	CuAssertIntEquals(tc, 0, pos);
	
// 	printf("Step 5\n");
// 	setCString(&input_command, "1get", 4);
// 	pos = 0;
// 	ret = identifier(&input_command, &pos);
// 	CuAssertIntEquals(tc, PARSE_ERROR_EXPECTED_IDENT, ret);
// 	CuAssertIntEquals(tc, 0, pos);
	
// 	printf("Step 6\n");
// 	setCString(&input_command, "get1", 4);
// 	pos = 0;
// 	ret = identifier(&input_command, &pos);
// 	CuAssertIntEquals(tc, PARSE_OK, ret);
// 	CuAssertIntEquals(tc, 4, pos);
	
// 	printf("Step 7\n");
// 	setCString(&input_command, "!", 1);
// 	pos = 0;
// 	ret = identifier(&input_command, &pos);
// 	CuAssertIntEquals(tc, PARSE_ERROR_EXPECTED_IDENT, ret);
// 	CuAssertIntEquals(tc, 0, pos);
	
// 	printf("Step 8\n");
// 	setCString(&input_command, "!get", 4);
// 	pos = 0;
// 	ret = identifier(&input_command, &pos);
// 	CuAssertIntEquals(tc, PARSE_ERROR_EXPECTED_IDENT, ret);
// 	CuAssertIntEquals(tc, 0, pos);
	
// 	printf("Step 9\n");
// 	setCString(&input_command, "get!", 4);
// 	pos = 0;
// 	ret = identifier(&input_command, &pos);
// 	CuAssertIntEquals(tc, PARSE_OK, ret);
// 	CuAssertIntEquals(tc, 3, pos);
	
// 	printf("Step 10\n");
// 	setCString(&input_command, "get1set", 7);
// 	pos = 0;
// 	ret = identifier(&input_command, &pos);
// 	CuAssertIntEquals(tc, PARSE_OK, ret);
// 	CuAssertIntEquals(tc, 7, pos);
	
// 	printf("Step 11\n");
// 	setCString(&input_command, "get12set13", 10);
// 	pos = 0;
// 	ret = identifier(&input_command, &pos);
// 	CuAssertIntEquals(tc, PARSE_OK, ret);
// 	CuAssertIntEquals(tc, 10, pos);
	
// 	printf("Step 12\n");
// 	setCString(&input_command, "get123set!", 10);
// 	pos = 0;
// 	ret = identifier(&input_command, &pos);
// 	CuAssertIntEquals(tc, PARSE_OK, ret);
// 	CuAssertIntEquals(tc, 9, pos);
	
// 	freeString(&input_command);

// 	printf("Test identifier done.\n");
// }

/******************************************/
struct Smock {
	enum EParseStatus ret;
	int expected_pos;
	char* s;
};

#define MOCK_SIZE(c) sizeof(c)/sizeof(struct Smock)

struct Smock sym_name_mock[] = {
	{PARSE_OK, 1, "a"},
	{PARSE_OK, 3, "get"},
	{PARSE_OK, 1, "+"},
	{PARSE_OK, 1, "#"},
	{PARSE_OK, 1, "%"},
	{PARSE_OK, 2, "++"},
	{PARSE_OK, 2, "##"},
	{PARSE_OK, 2, "%%"},
	{PARSE_OK, 3, "+#%"},
	{PARSE_OK, 4, "a+#%"},
	{PARSE_OK, 4, "+#%a"},
	{PARSE_OK, 5, "a+#%a"},
	{PARSE_OK, 10, "a+#%aa+#%a"},
	{PARSE_OK, 8, "+#%aa+#%"},
	{PARSE_OK, 2, "1a"},
	{PARSE_OK, 4, "11aa"},
	{PARSE_OK, 5, "11+a+"},
	{PARSE_OK, 3, "1a1"},
	{PARSE_OK, 5, "11aa1"},
	{PARSE_OK, 6, "11aa11"},
	{PARSE_OK, 6, "+a1111"},
	{PARSE_OK, 8, "+a1111+a"},
	{PARSE_OK, 11, "+a1111+a222"},
	{PARSE_ERROR_EXPECTED_SYM_NAME, 0, ""},
	{PARSE_ERROR_EXPECTED_SYM_NAME, 0, "!"},
	{PARSE_ERROR_EXPECTED_SYM_NAME, 0, "!a"},
	{PARSE_OK, 1, "a!"},
	{PARSE_ERROR_EXPECTED_SYM_NAME, 0, "111"},
	{PARSE_OK, 1, "g!et"},
	{PARSE_OK, 1, "+!"},
	{PARSE_OK, 1, "#!"},
	{PARSE_OK, 1, "%!"},
	{PARSE_OK, 2, "++!"},
	{PARSE_OK, 1, "#!#"},
	{PARSE_ERROR_EXPECTED_SYM_NAME, 0, "!%%"},
	{PARSE_OK, 11, "+a1111+a222!"}
};

struct Smock dec_num_mock[] = {
	{PARSE_ERROR_EXPECTED_DECNUM, 0, ""},
	{PARSE_OK, 1, "0"},
	{PARSE_OK, 5, "01234"},
	{PARSE_ERROR_EXPECTED_DECNUM, 0, "."},
	{PARSE_OK, 2, ".0"},
	{PARSE_OK, 5, ".0123"},
	{PARSE_ERROR_EXPECTED_DECNUM, 0, "0."},
	{PARSE_OK, 3, "0.0"},
	{PARSE_OK, 6, "0.0123"},
	{PARSE_OK, 7, "012.012"},

	{PARSE_ERROR_EXPECTED_DECNUM, 0, "D'"},
	{PARSE_ERROR_EXPECTED_DECNUM, 0, "D'ala"},
	{PARSE_ERROR_EXPECTED_DECNUM, 0, "Dala"},
	{PARSE_OK, 3, "D'0"},
	{PARSE_OK, 7, "D'01234"},
	{PARSE_ERROR_EXPECTED_DECNUM, 0, "D'."},
	{PARSE_OK, 4, "D'.0"},
	{PARSE_OK, 7, "D'.0123"},
	{PARSE_ERROR_EXPECTED_DECNUM, 0, "D'0."},
	{PARSE_OK, 5, "D'0.0"},
	{PARSE_OK, 8, "D'0.0123"},
	{PARSE_OK, 9, "D'012.012"},

	{PARSE_ERROR_EXPECTED_DECNUM, 0, "D01.01"},
	{PARSE_OK, 3, "D'1,45"},
	{PARSE_ERROR_EXPECTED_DECNUM, 0, "A'ala"},
	{PARSE_ERROR_EXPECTED_DECNUM, 0, "A'123"},
	{PARSE_OK, 4, "1872ala"},
	{PARSE_OK, 5, "12.12aa"},
	{PARSE_ERROR_EXPECTED_DECNUM, 0, "12.ala12"}
};

struct Smock arith_expr_mock[] = {
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, ""},
    {PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "()"},
    {PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "("},
    {PARSE_ERROR_EXPECTED_ARITHEXPR, 0, ")"},
	{PARSE_OK, 3, "(0)"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(0"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "0)"},
	{PARSE_OK, 7, "(01234)"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(.)"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(."},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, ".)"},
	{PARSE_OK, 4, "(.0)"},
	{PARSE_OK, 7, "(.0123)"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(0.)"},
	{PARSE_OK, 5, "(0.0)"},
	{PARSE_OK, 8, "(0.0123)"},
	{PARSE_OK, 9, "(012.012)"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(ala)"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(ala"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "D(01.01)"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(1,45)"},
	{PARSE_OK, 3, "(1)aa"},
	{PARSE_OK, 9, "(123.123)aa"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(123.12ala)"},

	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "+"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "-"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "+-"},
    {PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(+)"},
    {PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(-)"},
    {PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(+-)"},
    {PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(+"},
    {PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(-"},
    {PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(+-"},
    {PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "+)"},
	{PARSE_OK, 4, "(+0)"},
	{PARSE_OK, 4, "(-0)"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(+-0)"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(+0"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "+0)"},
	{PARSE_OK, 8, "(+01234)"},
	{PARSE_OK, 8, "(-01234)"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(+.)"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(+."},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "+.)"},
	{PARSE_OK, 5, "(+.0)"},
	{PARSE_OK, 8, "(+.0123)"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(+0.)"},
	{PARSE_OK, 6, "(+0.0)"},
	{PARSE_OK, 9, "(+0.0123)"},
	{PARSE_OK, 10, "(+012.012)"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(+ala)"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(+ala"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(a01.01)"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(+1,45)"},
	{PARSE_OK, 4, "(+1)aa"},
	{PARSE_OK, 10, "(+123.123)aa"},
	{PARSE_ERROR_EXPECTED_ARITHEXPR, 0, "(+123.12ala)"}
};

struct Smock hex_num_mock[] = {
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, ""},
    {PARSE_OK, 1, "0"},
    {PARSE_OK, 2, "01"},
    {PARSE_OK, 3, "012"},
	{PARSE_OK, 1, "A"},
	{PARSE_OK, 2, "AA"},
	{PARSE_OK, 3, "ABF"},
	{PARSE_OK, 2, "0A"},
	{PARSE_OK, 3, "0AB"},
	{PARSE_OK, 3, "00A"},
	{PARSE_OK, 4, "00AB"},
	{PARSE_OK, 2, "A0"},
	{PARSE_OK, 3, "AB0"},
	{PARSE_OK, 3, "A01"},
	{PARSE_OK, 4, "AB01"},
	{PARSE_OK, 6, "AB01AA"},
	{PARSE_OK, 7, "AB01AA1"},
	{PARSE_OK, 7, "01AB01A"},
	{PARSE_OK, 7, "01aB01a"},
	{PARSE_OK, 7, "01aB01aX"},

	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "H'"},
    {PARSE_OK, 3, "H'0"},
    {PARSE_OK, 4, "H'01"},
    {PARSE_OK, 5, "H'012"},
	{PARSE_OK, 3, "H'A"},
	{PARSE_OK, 4, "H'AA"},
	{PARSE_OK, 5, "H'ABF"},
	{PARSE_OK, 4, "H'0A"},
	{PARSE_OK, 5, "H'0AB"},
	{PARSE_OK, 5, "H'00A"},
	{PARSE_OK, 6, "H'00AB"},
	{PARSE_OK, 4, "H'A0"},
	{PARSE_OK, 5, "H'AB0"},
	{PARSE_OK, 5, "H'A01"},
	{PARSE_OK, 6, "H'AB01"},
	{PARSE_OK, 8, "H'AB01AA"},
	{PARSE_OK, 9, "H'AB01AA1"},
	{PARSE_OK, 9, "H'01AB01A"},
	{PARSE_OK, 9, "H'01aB01a"},
	{PARSE_OK, 9, "H'01aB01aX"},

	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "H"},
    {PARSE_ERROR_EXPECTED_HEXNUM, 0, "H0"},
    {PARSE_ERROR_EXPECTED_HEXNUM, 0, "H01"},
    {PARSE_ERROR_EXPECTED_HEXNUM, 0, "H012"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HA"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HAA"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HABF"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "H0A"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "H0AB"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "H00A"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "H00AB"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HA0"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HAB0"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HA01"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HAB01"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HAB01AA"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HAB01AA1"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "H01AB01A"},

	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "Hx"},
    {PARSE_ERROR_EXPECTED_HEXNUM, 0, "Hx0"},
    {PARSE_ERROR_EXPECTED_HEXNUM, 0, "Hx01"},
    {PARSE_ERROR_EXPECTED_HEXNUM, 0, "Hx012"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HxA"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HxAA"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HxABF"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "Hx0A"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "Hx0AB"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "Hx00A"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "Hx00AB"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HxA0"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HxAB0"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HxA01"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HxAB01"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HxAB01AA"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "HxAB01AA1"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "Hx01AB01A"},

	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "x"},
    {PARSE_ERROR_EXPECTED_HEXNUM, 0, "x0"},
    {PARSE_ERROR_EXPECTED_HEXNUM, 0, "x01"},
    {PARSE_ERROR_EXPECTED_HEXNUM, 0, "x012"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "xA"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "xAA"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "xABF"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "x0A"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "x0AB"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "x00A"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "x00AB"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "xA0"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "xAB0"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "xA01"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "xAB01"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "xAB01AA"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "xAB01AA1"},
	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "x01AB01A"},

	{PARSE_ERROR_EXPECTED_HEXNUM, 0, "H'X"},
    {PARSE_OK, 3, "H'0X"},
    {PARSE_OK, 4, "H'01X"},
    {PARSE_OK, 5, "H'012X"},
	{PARSE_OK, 3, "H'AX"},
	{PARSE_OK, 4, "H'AAX"},
	{PARSE_OK, 5, "H'ABFX"},
	{PARSE_OK, 4, "H'0AX"},
	{PARSE_OK, 5, "H'0ABX"},
	{PARSE_OK, 5, "H'00AX"},
	{PARSE_OK, 6, "H'00ABX"},
	{PARSE_OK, 4, "H'A0X"},
	{PARSE_OK, 5, "H'AB0X"},
	{PARSE_OK, 5, "H'A01X"},
	{PARSE_OK, 6, "H'AB01X"},
	{PARSE_OK, 8, "H'AB01AAX"},
	{PARSE_OK, 9, "H'AB01AA1X"},
	{PARSE_OK, 9, "H'01AB01AX"},
	{PARSE_OK, 9, "H'01aB01aX"}
};

struct Smock oct_num_mock[] = {
	{PARSE_ERROR_EXPECTED_OCTNUM, 0, ""},
    {PARSE_OK, 1, "0"},
    {PARSE_OK, 2, "01"},
    {PARSE_OK, 4, "0127"},
    {PARSE_OK, 4, "01278"},

	{PARSE_ERROR_EXPECTED_OCTNUM, 0, "O'"},
    {PARSE_OK, 3, "O'0"},
    {PARSE_OK, 4, "O'01"},
    {PARSE_OK, 6, "O'0127"},
    {PARSE_OK, 6, "O'01278"},

	{PARSE_ERROR_EXPECTED_OCTNUM, 0, "O"},
    {PARSE_ERROR_EXPECTED_OCTNUM, 0, "O0"},
    {PARSE_ERROR_EXPECTED_OCTNUM, 0, "O01"},
	{PARSE_ERROR_EXPECTED_OCTNUM, 0, "Ox"},
    {PARSE_ERROR_EXPECTED_OCTNUM, 0, "Ox0"},
    {PARSE_ERROR_EXPECTED_OCTNUM, 0, "Ox01"},
	{PARSE_ERROR_EXPECTED_OCTNUM, 0, "x"},
    {PARSE_ERROR_EXPECTED_OCTNUM, 0, "x0"},
    {PARSE_ERROR_EXPECTED_OCTNUM, 0, "x01"},
    {PARSE_ERROR_EXPECTED_OCTNUM, 0, "x012"},
	{PARSE_ERROR_EXPECTED_OCTNUM, 0, "O'X"}
};

struct Smock bin_num_mock[] = {
	{PARSE_ERROR_EXPECTED_BINNUM, 0, ""},
    {PARSE_OK, 1, "0"},
    {PARSE_OK, 2, "01"},
    {PARSE_OK, 2, "0127"},

	{PARSE_ERROR_EXPECTED_BINNUM, 0, "B'"},
    {PARSE_OK, 3, "B'0"},
    {PARSE_OK, 4, "B'01"},
    {PARSE_OK, 4, "B'0127"},

	{PARSE_ERROR_EXPECTED_BINNUM, 0, "B"},
    {PARSE_ERROR_EXPECTED_BINNUM, 0, "B0"},
    {PARSE_ERROR_EXPECTED_BINNUM, 0, "B01"},
	{PARSE_ERROR_EXPECTED_BINNUM, 0, "Bx"},
    {PARSE_ERROR_EXPECTED_BINNUM, 0, "Bx0"},
    {PARSE_ERROR_EXPECTED_BINNUM, 0, "Bx01"},
	{PARSE_ERROR_EXPECTED_BINNUM, 0, "x"},
    {PARSE_ERROR_EXPECTED_BINNUM, 0, "x0"},
    {PARSE_ERROR_EXPECTED_BINNUM, 0, "x01"},
    {PARSE_ERROR_EXPECTED_BINNUM, 0, "x012"},
	{PARSE_ERROR_EXPECTED_BINNUM, 0, "B'X"}
};

struct Smock keyed_num_mock[] = {
	{PARSE_ERROR_EXPECTED_KEYEDNUM, 0, ""},
    {PARSE_OK, 1, "0"},
    {PARSE_OK, 2, "01"},
    {PARSE_OK, 4, "0127"},
    {PARSE_OK, 1, "A"},
    {PARSE_OK, 1, "D"},
    {PARSE_OK, 4, "ABCD"},
    {PARSE_OK, 8, "0127ABCD"},
    {PARSE_OK, 8, "ABCD1234"},
    {PARSE_OK, 12, "0127ABCD1234"},
    {PARSE_OK, 1, "*"},
    {PARSE_OK, 2, "**"},
    {PARSE_OK, 1, "#"},
    {PARSE_OK, 2, "##"},
    {PARSE_OK, 3, "1#D"},
    {PARSE_OK, 9, "*#123DA#*"},
    {PARSE_OK, 9, "*#123DA#*WWW"},

	{PARSE_ERROR_EXPECTED_KEYEDNUM, 0, "K'"},
    {PARSE_OK, 3, "K'0"},
    {PARSE_OK, 4, "K'01"},
    {PARSE_OK, 6, "K'0127"},
    {PARSE_OK, 3, "K'A"},
    {PARSE_OK, 3, "K'D"},
    {PARSE_OK, 6, "K'ABCD"},
    {PARSE_OK, 10, "K'0127ABCD"},
    {PARSE_OK, 10, "K'ABCD1234"},
    {PARSE_OK, 14, "K'0127ABCD1234"},
    {PARSE_OK, 3, "K'*"},
    {PARSE_OK, 4, "K'**"},
    {PARSE_OK, 3, "K'#"},
    {PARSE_OK, 4, "K'##"},
    {PARSE_OK, 5, "K'1#D"},
    {PARSE_OK, 11, "K'*#123DA#*"},
    {PARSE_OK, 11, "K'*#123DA#*WWW"},

	{PARSE_ERROR_EXPECTED_KEYEDNUM, 0, "K"},
    {PARSE_ERROR_EXPECTED_KEYEDNUM, 0, "K0"},
    {PARSE_ERROR_EXPECTED_KEYEDNUM, 0, "K01"},
	{PARSE_ERROR_EXPECTED_KEYEDNUM, 0, "K*#1"},
    {PARSE_ERROR_EXPECTED_KEYEDNUM, 0, "K'W"},
	{PARSE_ERROR_EXPECTED_KEYEDNUM, 0, "x"},
    {PARSE_ERROR_EXPECTED_KEYEDNUM, 0, "x0"}
};

struct Smock integer_mock[] = {
	{PARSE_ERROR_EXPECTED_INTEGER, 0, ""},
    {PARSE_OK, 1, "0"},
    {PARSE_OK, 2, "01"},
    {PARSE_OK, 4, "0127"},
    {PARSE_OK, 3, "(0)"},
    {PARSE_OK, 4, "(01)"},
    {PARSE_OK, 5, "(012)"},
    {PARSE_OK, 7, "(+0123)"},
    {PARSE_OK, 7, "(-0123)"},

    {PARSE_OK, 1, "0a"},
    {PARSE_OK, 2, "01a"},
    {PARSE_OK, 4, "0127a"},
    {PARSE_OK, 3, "(0)a"},
    {PARSE_OK, 4, "(01)a"},
    {PARSE_OK, 5, "(012)a"},
    {PARSE_OK, 7, "(+0123)a"},
    {PARSE_OK, 7, "(-0123)a"},

    {PARSE_ERROR_EXPECTED_INTEGER, 0, "a"},
    {PARSE_ERROR_EXPECTED_INTEGER, 0, "a0"},
    {PARSE_ERROR_EXPECTED_INTEGER, 0, "a(0)"},

    {PARSE_ERROR_EXPECTED_INTEGER, 0, "(0"},
    {PARSE_ERROR_EXPECTED_INTEGER, 0, "(01"},
    {PARSE_ERROR_EXPECTED_INTEGER, 0, "(012"},
    {PARSE_ERROR_EXPECTED_INTEGER, 0, "(+0123"},
    {PARSE_ERROR_EXPECTED_INTEGER, 0, "(-0123"},

    {PARSE_OK, 1, "0)"},
    {PARSE_OK, 2, "01)"},
    {PARSE_OK, 3, "012)"},
    {PARSE_ERROR_EXPECTED_INTEGER, 0, "+0123)"},
    {PARSE_ERROR_EXPECTED_INTEGER, 0, "-0123)"},

    {PARSE_ERROR_EXPECTED_INTEGER, 0, "+0123"},
    {PARSE_ERROR_EXPECTED_INTEGER, 0, "-0123"}
};

struct Smock text_str_mock[] = {
	{PARSE_ERROR_EXPECTED_TEXTSTR, 0, ""},
	{PARSE_ERROR_EXPECTED_TEXTSTR, 0, "aaa"},
    {PARSE_OK, 2, "\"\""},
    {PARSE_OK, 3, "\"a\""},
    {PARSE_OK, 5, "\"ala\""},
    {PARSE_OK, 10, "\"!@#$ssss\""},
    {PARSE_OK, 8, "\"ss\\\\dd\""},
    {PARSE_OK, 8, "\"ss\\\"dd\""},
    {PARSE_ERROR_EXPECTED_TEXTSTR, 0, "\"ss\\gdd\""},

    {PARSE_OK, 2, "\"\"xxx"},
    {PARSE_OK, 3, "\"a\"xxx"},
    {PARSE_OK, 5, "\"ala\"xxx"},
    {PARSE_OK, 10, "\"!@#$ssss\"xxx"},
    {PARSE_OK, 8, "\"ss\\\\dd\"xxx"},
    {PARSE_OK, 8, "\"ss\\\"dd\"xxx"},
    {PARSE_ERROR_EXPECTED_TEXTSTR, 0, "\"ss\\gdd\"xxx"}
};

void _testMocks(CuTest *tc, struct Smock mock[], int mock_size, char* msg, enum EParseStatus(*fun)(struct String*, int*) ) {
	struct String input_command;
	enum EParseStatus ret = PARSE_INITIAL;
	int pos;
	
    printf("-----------------------------------\n");
	printf("Test start: %s\n", msg);

	initString(&input_command, 0);
	
	for (int i=0; i<mock_size; i++){
		printf("Step %d: '%s', len=%ld\n", i, mock[i].s, strlen(mock[i].s));
		setCString(&input_command, mock[i].s, strlen(mock[i].s));
		pos = 0;
		ret = fun(&input_command, &pos);
		printf("\texpected ret=%d %s, actual ret=%d %s\n", mock[i].ret, parseErrorNames[mock[i].ret], ret, parseErrorNames[ret]);
		printf("\texpected pos=%d, actual pos=%d\n", mock[i].expected_pos, pos);
		CuAssertIntEquals(tc, mock[i].ret, ret);
		CuAssertIntEquals(tc, mock[i].expected_pos, pos);
	}
	
	freeString(&input_command);

	printf("Test done %s.\n", msg);
}

// void testSymName(CuTest *tc) {
//     _testMocks(tc, sym_name_mock, MOCK_SIZE(sym_name_mock), "Symbolic Name", sym_name);
// }

// void testDecNum(CuTest *tc) {
//     _testMocks(tc, dec_num_mock, MOCK_SIZE(dec_num_mock), "Dec Num", dec_num);
// }

// void testArithExpr(CuTest *tc) {
//     _testMocks(tc, arith_expr_mock, MOCK_SIZE(arith_expr_mock), "Arith Expr", arith_expr);
// }

// void testHexNum(CuTest *tc) {
//     _testMocks(tc, hex_num_mock, MOCK_SIZE(hex_num_mock), "Hex Num", hexNum);
// }

// void testOctNum(CuTest *tc) {
//     _testMocks(tc, oct_num_mock, MOCK_SIZE(oct_num_mock), "Oct Num", octNum);
// }

// void testBinNum(CuTest *tc) {
//     _testMocks(tc, bin_num_mock, MOCK_SIZE(bin_num_mock), "Bin Num", binNum);
// }

// void testKeyedNum(CuTest *tc) {
//     _testMocks(tc, keyed_num_mock, MOCK_SIZE(keyed_num_mock), "Keyed Num", keyedNum);
// }

// void testInteger(CuTest *tc) {
//     _testMocks(tc, integer_mock, MOCK_SIZE(integer_mock), "Integer", integer);
// }

// void testTextStr(CuTest *tc) {
//     _testMocks(tc, text_str_mock, MOCK_SIZE(text_str_mock), "TextStr", textStr);
// }

/************************************************************************************/

struct SCommandMock {
    char* command;
    enum EParseStatus expected_status;
};

struct SCommandMock commands_mock[] = {
    {"aaa-aa:xx:,,xx-1:ctag::,z1=10;", PARSE_OK},
    {"aaa-bb:bbv:\"ala\",aa-1&bb-2&&-4&bb-1:ctag::vv-1&-2&&-3:D'.001::kk=10,jj,\"ll\",gg=D'.01:m=10&&16.++.2;", PARSE_OK}, 
    {"aaa-bb;", PARSE_ERROR_EXPECTED_INPUTMESSAGE},
    {"aaa-bb:;", PARSE_ERROR_EXPECTED_INPUTMESSAGE},
    {"aaa-bb:bbv;", PARSE_ERROR_EXPECTED_INPUTMESSAGE},
    {"aaa-bb:bbv:\"ala\",aa-1&bb-2&&-4&::kk=10,jj,\"ll\",gg=D'.01:m=10&&16.++.2:;", PARSE_ERROR_EXPECTED_INPUTMESSAGE},
    {"aaa!bb:bbv:\"ala\",aa-1&bb-2&&-4&::kk=10,jj,\"ll\",gg=D'.01:m=10&&16.++.2:;", PARSE_ERROR_EXPECTED_INPUTMESSAGE},
    {"aaa-bb:bbv:\"ala\",aa-1&bb-2&&-4&bb-1:ctag::vv-1&-2&&-3:D'.001::kk=10,jj,\"ll\",gg=D'.01:m=10&&16.++.!2;", PARSE_ERROR_EXPECTED_INPUTMESSAGE}, 
    {"aaa-bb:bbv", PARSE_ERROR_EXPECTED_INPUTMESSAGE}, 
    {"aaa-bb:bbv:", PARSE_ERROR_EXPECTED_INPUTMESSAGE}, 
    {"aaa-bb:bbv:\"ala\",aa-1&bb-2&&-4&bb-1:ctag::vv-1&-2&&-3:D'.001::kk=10,jj,\"ll\",gg=D'.01:m=10-&&16.++.!2;", PARSE_ERROR_EXPECTED_INPUTMESSAGE}, 
    {"aaa-bb:bbv:\"ala\",aa-1&bb-2&&-4&bb-1:ctag::vv-1&-2&&-3:D'.001::kk=10,jj,\"ll\",gg=D'.01:m=10-1*2&&16.++.!2;", PARSE_ERROR_EXPECTED_INPUTMESSAGE}, 
    {"aaa-bb:bbv:\"ala\",aa-1&bb-2&&-4&bb-1:ctag::vv-1&-2&&-3:D'.001::kk=10,jj,\"ll\",gg=D'.01:m=10-K'1*2&&16.++.!2;", PARSE_ERROR_EXPECTED_INPUTMESSAGE}, 
};

#define COMMAND_MOCK_SIZE(c) sizeof(c)/sizeof(struct SCommandMock)


void _testMockedCommands(CuTest *tc, struct SCommandMock mock[], int mock_size) {
	struct String input_command;
	
    printf("-----------------------------------\n");
	printf("Test Tokenizer start\n");

	initString(&input_command, 0);
	
	for (int i=0; i<mock_size; i++){
		printf("Step %d: '%s'\n", i, mock[i].command);
		setCString(&input_command, mock[i].command, strlen(mock[i].command));
		enum EParseStatus ret = TL1CommandParse(&input_command);
        printf("Expected: %s (%d); Actual: %s (%d)\n", parseErrorNames[mock[i].expected_status], mock[i].expected_status, parseErrorNames[ret], ret);
        CuAssertIntEquals(tc, mock[i].expected_status, ret);

	}
	
	freeString(&input_command);

	printf("Test Tokenizer done.\n");
}

void testParse(CuTest *tc) {
    _testMockedCommands(tc, commands_mock, COMMAND_MOCK_SIZE(commands_mock));
}

CuSuite* TestIdentSuite() {
	CuSuite* suite = CuSuiteNew();
	// SUITE_ADD_TEST(suite, testIdent);
	// SUITE_ADD_TEST(suite, testSymName);
	// SUITE_ADD_TEST(suite, testDecNum);
	// SUITE_ADD_TEST(suite, testArithExpr);
	// SUITE_ADD_TEST(suite, testHexNum);
	// SUITE_ADD_TEST(suite, testOctNum);
	// SUITE_ADD_TEST(suite, testBinNum);
	// SUITE_ADD_TEST(suite, testKeyedNum);
	// SUITE_ADD_TEST(suite, testInteger);
	// SUITE_ADD_TEST(suite, testTextStr);

	// SUITE_ADD_TEST(suite, testTokenize);
	SUITE_ADD_TEST(suite, testParse);

	return suite;
}
