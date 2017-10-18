%{

/*
 * This is a "format preserving" lexer. This means for example that if we have
 * white chars before and after the '=' operator we are not ignoring those but
 * we store them together with the token.
 * 
 * This is a feature we use when the internal representation is changed and we 
 * generate the language script from it: the '=' operator will appear with the
 * exact same white characters the original input file had.
 */

    /*
{HASHMARK}{WORD}{EQUAL}[^\n\r#]+/[\n\r#] {
    S9S_DEBUG("Commented: %s", yytext);
    S9sString name, value;
    S9sString expression = yytext;

    expression.regMatch(
            "[\\ \t]*#[\\ \t]*"
            "([-a-zA-Z0-9_\\.]+)"
            "[\t\\ ]*=[\t\\ ]*"
            "([^\n\r]+)",
            name, value);
        
    S9sConfigAstNode *node1, *node2;
    node1 = new S9sConfigAstNode(
                S9sConfigAstNode::Variable, STR(name));

    node2 = new S9sConfigAstNode(
                S9sConfigAstNode::Literal, STR(value));

    yylval->nval = new S9sConfigAstNode(
                S9sConfigAstNode::Commented, " = ");
        
    yylval->nval->setChildren(node1, node2);

    // stupid *hack* but if a 'Comment' follows 'Commented'
    // we must keep a whitespace then...
    if (isspace (yytext[strlen(yytext)-1])) {
        unput (yytext[strlen(yytext)-1]);
    }

    return CONFIG_COMMENTED_ASSIGNMENT;
}*/



#define YY_EXTRA_TYPE S9sClusterConfigParseContext * 

#include "S9sConfigFile"
#include "config_parser.h"
#include <unistd.h>

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

EQUAL           [\t\ ]*=[\t\ ]*
HASHMARK        [\ \t]*#[\t\ ]*
WORD            [a-zA-Z_][-a-zA-Z0-9_/\.;]*
STRING          [-a-zA-Z0-9_\.\*\+/:,@%]+
ABSOLUTEPATH    \/[-0-9a-zA-Z\/_\.]*
RELATIVEPATH    \.[-0-9a-zA-Z\/_\.]+
IPADDRESS       [0-9]+\.[0-9]+\.[0-9]+\.[0-9]+
SUBSTITUTION    @{WORD}       
DIGIT           [0-9]
INTEGER         [-+]?{DIGIT}+[MK]?
DOUBLE          [-+]?{DIGIT}*\.{DIGIT}+
DOUBLEWITHEXP   ({DOUBLE}|{INTEGER})[eE][-+]?{DIGIT}+

%option yylineno
%option reentrant
%option bison-bridge

%x C_STRING
%x SECTION_NAME
%x RIGHT_SIDE
%x CONFIG_SET

%%

    if (false)
        yyunput(0, NULL, NULL);

{HASHMARK}[^\n\r#]* {
    S9S_DEBUG("comment: %s", yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Comment, yytext);
    return CONFIG_COMMENT;
}

!include {
    S9S_DEBUG("CONFIG_KEYWORD_INCLUDE: %s", yytext);
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Keyword, yytext);
    return CONFIG_KEYWORD_INCLUDE;
}

!includedir {
    S9S_DEBUG("CONFIG_KEYWORD_INCLUDEDIR: %s", yytext);
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Keyword, yytext);
    return CONFIG_KEYWORD_INCLUDEDIR;
}

"set-variable" {
    S9S_DEBUG("CONFIG_KEYWORD_SET: %s", yytext);
    yyextra->tokenFound(yytext);
    //yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Keyword, yytext);
    BEGIN(CONFIG_SET);
    return CONFIG_KEYWORD_SET;
}

{IPADDRESS} {
    S9S_DEBUG("CONFIG_IPADDRESS: %s", yytext);
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Literal, yytext);
    return CONFIG_IPADDRESS;
}

{WORD} {
    S9S_DEBUG("CONFIG_WORD: %s", yytext);
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Literal, yytext);
    return CONFIG_WORD;
}

{ABSOLUTEPATH} {
    S9S_DEBUG("CONFIG_ABSOLUTEPATH: %s", yytext);
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Literal, yytext);
    return CONFIG_ABSOLUTEPATH;
}

{RELATIVEPATH} {
    S9S_DEBUG("CONFIG_RELATIVEPATH: %s", yytext);
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Literal, yytext);
    return CONFIG_RELATIVEPATH;
}

{SUBSTITUTION} {
    S9S_DEBUG("CONFIG_SUBSTITUTION: %s", yytext);
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Literal, yytext);
    return CONFIG_SUBSTITUTION;
}

{INTEGER} {
    S9S_DEBUG("CONFIG_INTEGER: %s", yytext);
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Literal, yytext);
    return CONFIG_INTEGER;
}

{DOUBLE}|{DOUBLEWITHEXP} {
    S9S_DEBUG("CONFIG_DOUBLE: \"%s\"/%f", yytext, S9sString(yytext).toDouble());
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Literal, yytext);
    return CONFIG_DOUBLE;
}

\" {
    S9S_DEBUG("START DBL QUOTE : %s", yytext);
    yymore();
    BEGIN(C_STRING);
}

<RIGHT_SIDE>\" {
    S9S_DEBUG("START DBL QUOTE : %s", yytext);
    yymore();
    BEGIN(C_STRING);
}

"[" {
    S9S_DEBUG("START SECTION     : %s", yytext);
    BEGIN(SECTION_NAME);
    return '[';
}

[-a-zA-Z0-9\.:]+ {
    S9S_DEBUG("CONFIG_STRING: %s", yytext);
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Literal, yytext);
    return CONFIG_STRING;
}

'[^']*' {
    S9S_DEBUG("CONFIG_STRING: %s", yytext);
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Literal, yytext);
    return CONFIG_STRING;
}

<RIGHT_SIDE>'[^']*' {
    S9S_DEBUG("CONFIG_STRING: %s", yytext);
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Literal, yytext);
    BEGIN(INITIAL);
    return CONFIG_STRING;
}

{EQUAL} {
    S9S_DEBUG("CONFIG_ASSIGN     : '%s'", yytext);
    yyextra->tokenFound("=");
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Assignment, yytext);
    BEGIN(RIGHT_SIDE);

    return CONFIG_ASSIGN;
}

<CONFIG_SET>{EQUAL} {
    S9S_DEBUG("CONFIG_ASSIGN     : '%s'", yytext);
    yyextra->tokenFound("=");
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Assignment, yytext);
    BEGIN(INITIAL);
    return CONFIG_ASSIGN;
}

<C_STRING>\" {
    S9S_DEBUG("END DBL QTE     : '%s'", yytext);
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Literal, yytext);
    BEGIN(INITIAL);

    return CONFIG_STRING;
}

<C_STRING>\\. {
    S9S_DEBUG("ESCAPED CHAR    : '%s'", yytext);
    yymore();
}

<C_STRING>[^\\\n\r\"]* {
    S9S_DEBUG("CONFIG_STRING     : '%s'", yytext);
    yymore();
}

<SECTION_NAME>[^\]]*/"]" {
    S9S_DEBUG("END SECTION_NAME: '%s'", yytext);
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Literal, yytext);
    BEGIN(INITIAL);

    return CONFIG_WORD;
}

<SECTION_NAME>[^\]]* {
    S9S_DEBUG("SECTION_NAME      : '%s'", yytext);
    yymore();
}

<RIGHT_SIDE>(\\.|[^\n\r\t#\"])*(\\.|[^\n\r\t\ #\"])+ {
    // to handle the "string containing space" situation: the string is not
    // double quoted, it might have spaces and escaped characters and the 
    // triling white chars are not included. Leading white chars are included,
    // but those are already accepted by the '=' operator.
    S9S_DEBUG("END RIGHT_SIDE  : '%s'", yytext);
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Literal, yytext);
    BEGIN(INITIAL);

    return CONFIG_WORD;
}

<RIGHT_SIDE>"\r\n"|"\n\r"|"\n" {
    S9S_DEBUG("new-line (empty right-side)");
    yyextra->tokenFound("new-line");
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::NewLine, yytext);
    BEGIN(INITIAL);
    return CONFIG_NEWLINE;
}

{STRING} {
    S9S_DEBUG("STRING          : '%s'", yytext);
    yyextra->tokenFound(yytext);
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::Literal, yytext);

    return CONFIG_STRING;
}


<*>[\t\ ] {
    // Nothing to do
}

<*>"\r\n"|"\n\r"|"\n" {
    S9S_DEBUG("new-line");
    yyextra->tokenFound("new-line");
    yylval->nval = new S9sConfigAstNode(S9sConfigAstNode::NewLine, yytext);
    return CONFIG_NEWLINE;
}

<*>"\r" {
    // lets eat alone carriage return chars
}

<*>. {
    yyextra->tokenFound(yytext);
    S9S_DEBUG("char '%c'", yytext[0]);
    return yytext[0];
}

