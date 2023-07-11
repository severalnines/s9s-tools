%{

#include "s9sglobal.h"
#include "s9svariant.h"
#include "s9sconfigfile.h"

#include "config_parser.h"
#include "config_lexer.h"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC visibility push(hidden)
#endif

int config_error(S9sClusterConfigParseContext &context, const char *c);

#define scanner context.m_flex_scanner
%}

%pure-parser
%parse-param {S9sClusterConfigParseContext &context}
%lex-param {void * scanner}


%union {
    S9sConfigAstNode *nval;
}

%token <nval> CONFIG_KEYWORD_INCLUDE
%token <nval> CONFIG_KEYWORD_INCLUDEDIR
%token <nval> CONFIG_WORD

%token <nval> CONFIG_ASSIGN
%token <nval> CONFIG_COMMENT
%token <nval> CONFIG_COMMENTED_ASSIGNMENT
%token <nval> CONFIG_INTEGER
%token <nval> CONFIG_STRING
%token <nval> CONFIG_DOUBLE
%token <nval> CONFIG_IPADDRESS
%token <nval> CONFIG_SUBSTITUTION
%token <nval> CONFIG_ABSOLUTEPATH CONFIG_RELATIVEPATH
%token <nval> CONFIG_NEWLINE
%token        CONFIG_KEYWORD_SET

%type  <nval> comment
%type  <nval> literal literal_list
%type  <nval> section_header;
%type  <nval> assignment;
%type  <nval> config_item;

%destructor { delete $$; } <nval>

%%
config_file
    :
    | config_item_list
    ;

config_item_list
    : config_item                   { context.append($1); }
    | config_item_list config_item  { context.append($2); }
    ;

config_item
    : CONFIG_KEYWORD_INCLUDE CONFIG_WORD {
            $$ = new S9sConfigAstNode(S9sConfigAstNode::Include, $1, $2);
        }
    | CONFIG_KEYWORD_INCLUDE CONFIG_ABSOLUTEPATH {
            $$ = new S9sConfigAstNode(S9sConfigAstNode::Include, $1, $2);
        }
    | CONFIG_KEYWORD_INCLUDEDIR CONFIG_WORD {
            $$ = new S9sConfigAstNode(S9sConfigAstNode::IncludeDir, $1, $2);
        }
    | CONFIG_KEYWORD_INCLUDEDIR CONFIG_ABSOLUTEPATH {
            $$ = new S9sConfigAstNode(S9sConfigAstNode::IncludeDir, $1, $2);
        }
    | section_header
    | assignment
    | comment
    | CONFIG_NEWLINE
    ;

comment
    : CONFIG_COMMENT
    | CONFIG_COMMENTED_ASSIGNMENT
    ;

section_header
    : '[' CONFIG_WORD ']' { 
            $2->setType(S9sConfigAstNode::Section); 
            $$ = $2;
        }
    ;

assignment
    : CONFIG_WORD CONFIG_ASSIGN literal_list {
            $$ = $2;
            $$->setChildren($1, $3);
        }
    | CONFIG_KEYWORD_SET CONFIG_ASSIGN CONFIG_WORD CONFIG_ASSIGN literal_list {
            delete $2;
            $$ = $4;
            $$->setChildren($3, $5);
        }
    | CONFIG_WORD CONFIG_ASSIGN {
            // Assignment without an actual value
            // dkedves: I've changed this, hopefully will not cause issues
            $$ = $2;
            $$->setChildren($1, new S9sConfigAstNode(S9sConfigAstNode::Literal,""));
        }
    | CONFIG_WORD
    ;

literal_list
    : literal                   { $$ = $1; }
    | literal_list ',' literal  { $$ = $1; S9S_UNUSED($3); /* Loosing values here. */ }
    | literal_list ';' literal  { $$ = $1; S9S_UNUSED($3); /* Loosing values here. */ }
    ;

literal
    : CONFIG_STRING
    | CONFIG_INTEGER 
    | CONFIG_DOUBLE
    | CONFIG_IPADDRESS
    | CONFIG_SUBSTITUTION
    | CONFIG_WORD
    | CONFIG_ABSOLUTEPATH
    | CONFIG_RELATIVEPATH
    ;
%%

extern int config_lineno;
int
config_error(S9sClusterConfigParseContext &context, const char *c)
{
    context.errorFound(c);
    return 1;
}

extern "C" 
int 
config_wrap(void *my_scanner)
{
	return 1;
}

