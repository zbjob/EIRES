#pragma once
#define YY_NO_UNISTD_H 1

enum yytokentype
{
	AND = 258,
	OR = 259,

	LSS,
	LEQ,
	GRT,
	GEQ,
	EQ,
	NEQ,
    ADD,
    SUB,

	KW_QUERY,
	KW_EVENT,
	KW_SEQ,
	KW_WHERE,
	KW_WITHIN,
	KW_TYPE,
	KW_RETURN,

	CONST_INT = 277,
	ID = 281

};

struct yylval_t
{
	char* string;
	int intValue;
};

#ifdef FLEX_SCANNER
struct yylval_t yylval;
#else
extern "C" struct yylval_t yylval;
#endif

