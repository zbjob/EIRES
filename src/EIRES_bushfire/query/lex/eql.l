%option noyywrap
%option yylineno
%option nounput
%option noinput

WHITESPACE     [ \t\n\r]
INTEGER        [[:digit:]]+
DOUBLE         [-+]?[0-9]*[.][0-9]+$

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

INTERSECT   { return KW_INTERSECT; }

&&          { return AND; }
"||"        { return OR;  }
==          { return EQ;  }
!=          { return NEQ; }
\<=         { return LEQ; }
\>=         { return GEQ; }
\<           { return LSS; }
>           { return GRT; }
\/\\        { return EQ;  }

"+"         {yylval.text = yytext; return PLUS;}
"-"         {yylval.text = yytext; return MINUS;}
"*"         {yylval.text = yytext; return MULTIPLY;}
"/"         {yylval.text = yytext; return DIVIDE;}


{INTEGER}   { yylval.intValue = atoi(yytext);yylval.text = yytext; return CONST_INT; }
([[:alpha:]]|_)([[:alnum:]]|_)* { yylval.string = yytext; yylval.text = yytext; return ID; }

{DOUBLE} {yylval.doubleValue = atof(yytext);yylval.text = yytext; return CONST_DOUBLE;}

.           { yylval.text = yytext; return yytext[0]; }

%%

