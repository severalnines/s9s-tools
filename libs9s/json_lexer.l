%{

#define YY_EXTRA_TYPE S9sJsonParseContext * 

#include "s9sjsonparsecontext.h"
#include "json_parser.h"
#include <unistd.h>
#include <cmath>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wunused-parameter"
//#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC visibility push(hidden)
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#endif

#define YY_INPUT(buffer, returnvalue, maxsize) \
	(returnvalue = yyextra->yyinput(buffer, maxsize))

%}

DIGIT           [0-9]
INTEGER         [-+]?{DIGIT}+
DOUBLE          [-+]?{DIGIT}*\.{DIGIT}+
DOUBLEWITHEXP   ({DOUBLE}|{INTEGER})[eE][-+]?{DIGIT}+
NAN             [-+]?[Nn][Aa][Nn]
INF             [-+]?[Ii][Nn][Ff]([Ii][Nn][Ii][Tt][Yy])?
DOUBLESPECIAL   ({NAN}|{INF})

%option yylineno
%option reentrant
%option bison-bridge

%x C_COMMENT
%x C_STRING
%x S_STRING

%%

    if (false)
        yyunput(0, NULL, NULL);

<C_COMMENT>"*/" {
    S9S_DEBUG("BEGIN(INITIAL)");
    BEGIN(INITIAL);   
}

<C_COMMENT>. {
    S9S_DEBUG("comment: %c", yytext[0]);
    /*nothing*/       
}

"/*" { 
    S9S_DEBUG("BEGIN(C_COMMENT)");
    BEGIN(C_COMMENT); 
}

"//"[^\n]* {
    /* Comment, doing nothing */
}

{INTEGER} {
    S9S_DEBUG("JSON_INTEGER: %s", yytext);
    S9sString theString(yytext);
    if (theString.looksULongLong())
        yylval->vval = new S9sVariant(theString.toULongLong());
    else if (theString.looksInteger())
        yylval->vval = new S9sVariant(theString.toInt());
    else
        yylval->vval = new S9sVariant(theString.toDouble());

    return JSON_INTEGER;
}

{NAN} {
    S9S_DEBUG("JSON_DOUBLE: \"%s\", nan", yytext);
    yylval->vval = new S9sVariant(NAN);
    return JSON_DOUBLE;
}

{INF} {
    S9S_DEBUG("JSON_DOUBLE: \"%s\", %cinf", yytext, *yytext);
    if (*yytext == '-') {
        yylval->vval = new S9sVariant(-INFINITY);
    } else {
        yylval->vval = new S9sVariant(INFINITY);
    }
    return JSON_DOUBLE;
}

{DOUBLE}|{DOUBLEWITHEXP} {
    S9S_WARNING("JSON_DOUBLE: \"%s\"/%f", yytext, S9sString(yytext).toDouble());
    yylval->vval = new S9sVariant(S9sString(yytext).toDouble());
    return JSON_DOUBLE;
}

\" {
    S9S_DEBUG("START DBL QUOTE : %s", yytext);
    yymore();
    BEGIN(C_STRING);
}

<C_STRING>\" {
    S9S_DEBUG("END DBL QTE     : '%s'", yytext);
    S9sString theString = yytext;

    theString = theString.unQuote();
    theString = theString.unEscape();

    yylval->vval = new S9sVariant(theString);
    BEGIN(INITIAL);

    return JSON_STRING;
}

<C_STRING>\\. {
    S9S_DEBUG("ESCAPED CHAR    : '%s'", yytext);
    yymore();
}

<C_STRING>[^\\\n\"]* {
    S9S_DEBUG("JSON_STRING1    : '%s'", yytext);
    yymore();
}

\' {
    S9S_DEBUG("START SNGL QUOTE : %s", yytext);
    yymore();
    BEGIN(S_STRING);
}

<S_STRING>\' {
    S9S_DEBUG("END SNGL QTE     : '%s'", yytext);
    S9sString theString = yytext;

    theString = theString.unQuote();
    theString = theString.unEscape();

    yylval->vval = new S9sVariant(theString);
    BEGIN(INITIAL);

    return JSON_STRING;
}

<S_STRING>\\. {
    S9S_DEBUG("ESCAPED CHAR    : '%s'", yytext);
    yymore();
}

<S_STRING>[^\\\n\']* {
    S9S_DEBUG("JSON_STRING1    : '%s'", yytext);
    yymore();
}

"true" {
    S9S_DEBUG("JSON_BOOLEAN: %s", yytext);
    yylval->vval = new S9sVariant(true);
    return JSON_BOOLEAN;
}

"false" {
    S9S_DEBUG("JSON_BOOLEAN: %s", yytext);
    yylval->vval = new S9sVariant(false);
    return JSON_BOOLEAN;
}

"null" {
    S9S_DEBUG("JSON_NULL: %s", yytext);
    yylval->vval = new S9sVariant;
    return JSON_NULL;
}

[a-zA-Z_]+ {
    S9S_DEBUG("JSON_STRING2   : '%s'", yytext);
    yylval->vval = new S9sVariant(yytext);
    return JSON_STRING;
}


[\t\r\n ] {
    /* Nothing to do. */
}

. {
    S9S_DEBUG("char '%c'", yytext[0]);
    return yytext[0];
}

