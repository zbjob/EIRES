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

	KW_QUERY,
	KW_EVENT,
	KW_SEQ,
	KW_WHERE,
	KW_WITHIN,
	KW_TYPE,
	KW_RETURN,

	KW_INTERSECT,
	
	CONST_INT = 277,
	PLUS,
	MINUS,
	MULTIPLY,
	DIVIDE,
	ID = 281,
	CONST_DOUBLE
};

struct yylval_t
{
	char* string;
	int intValue;
	double doubleValue;
	char* text;
};

#ifdef FLEX_SCANNER
struct yylval_t yylval;
#else
extern "C" struct yylval_t yylval;
#endif

