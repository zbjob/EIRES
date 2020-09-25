%option noyywrap
%option yylineno
%option nounput
%option noinput

WHITESPACE     [ \t\n\r]
INTEGER        [[:digit:]]+

%x COMMENT

%{
#include "eql.h"
%}

%%

{WHITESPACE}+ {                 /* ignore whitespaces */ }
"//".*\n      {                 /* C++ Comment */ }
"/*"          { BEGIN(COMMENT); /* C   Comment */ }
<COMMENT>"*/" { BEGIN(INITIAL); }
<COMMENT>.|\n {}

QUERY		{ return KW_QUERY; }
EVENT		{ return KW_EVENT; }
SEQ			{ return KW_SEQ; }
WHERE		{ return KW_WHERE; }
WITHIN		{ return KW_WITHIN; }
TYPE		{ return KW_TYPE; }
RETURN		{ return KW_RETURN; }

&&          { return AND; }
"||"        { return OR;  }
==          { return EQ;  }
!=          { return NEQ; }
\<=         { return LEQ; }
>=          { return GEQ; }
\<          { return LSS; }
>           { return GRT; }

{INTEGER}   { yylval.intValue = atoi(yytext); return CONST_INT; }
([[:alpha:]]|_)([[:alnum:]]|_)* { yylval.string = yytext; return ID; }

.           { return yytext[0]; }

%%

