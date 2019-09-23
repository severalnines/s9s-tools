%{

#include "S9sGlobal"
#include "S9sVariant"
#include "S9sJsonParseContext"

#include "json_parser.h"
#include "json_lexer.h"

//#define DEBUG
#include "s9sdebug.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC visibility push(hidden)
#endif

int json_error(S9sJsonParseContext &context, const char *c);

#define scanner context.m_flex_scanner
%}

%pure-parser
%parse-param {S9sJsonParseContext &context}
%lex-param {void * scanner}


%union {
    S9sVariant     *vval;
    S9sVariantMap  *mval;
    S9sVariantList *lval;
}

%token <vval> JSON_INTEGER
%token <vval> JSON_STRING
%token <vval> JSON_NULL
%token <vval> JSON_DOUBLE
%token <vval> JSON_BOOLEAN

%type <vval> literal
%type <mval> json_value_list
%type <mval> json_opt_value_list
%type <mval> json_map
%type <mval> json_object
%type <lval> json_literal_list

%destructor { delete $$; } <vval>
%destructor { delete $$; } <mval>
%destructor { delete $$; } <lval>

%%
json_string
    : json_object { 
            context.setValues($1);
            delete $1;
        }
    ;

json_object
    : json_map { $$ = $1; }
    ;

json_map
    : '{' json_opt_value_list '}' {
            $$ = $2;
        }
    ;

json_opt_value_list
    :                  { $$ = new S9sVariantMap; }
    | json_value_list  { $$ = $1; }
    ;

json_value_list
    : JSON_STRING ':' literal {
            $$ = new S9sVariantMap;
            (*$$)[$1->toString()] = *$3;
            delete $1;
            delete $3;
        }
    | json_value_list ',' JSON_STRING ':' literal {
            $$ = $1;
            (*$$)[$3->toString()] = *$5;
            delete $3;
            delete $5;
        }
    ;

literal
    : JSON_STRING
    | JSON_NULL
    | JSON_INTEGER
    | JSON_BOOLEAN
    | JSON_DOUBLE
    | json_map {
            $$ = new S9sVariant((S9sVariantMap &) *$1);
            delete $1;
        }
    | '[' json_literal_list ']' {
            $$ = new S9sVariant((S9sVariantList &) *$2);
            delete $2;
        }
    | '[' ']' {
            $$ = new S9sVariant(S9sVariantList());
        }
    ;

json_literal_list
    : literal { $$ = new S9sVariantList; $$->push_back(*$1); delete $1; }
    | json_literal_list ',' literal {$$ = $1; $$->push_back(*$3); delete $3; }
    ;
%%

extern int json_lineno;
int
json_error(S9sJsonParseContext &context, const char *c)
{
    S9S_UNUSED(c);
    return 1;
}

extern "C" 
int 
json_wrap(void *my_scanner)
{
    S9S_UNUSED(my_scanner);
	return 1;
}

