/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#include "s9sjsonparsecontext.h"

#define DEBUG
#include "s9sdebug.h"

S9sJsonParseContext::S9sJsonParseContext(
        const char *input) :
    S9sParseContext(input)
{
}

void
S9sJsonParseContext::setValues(
        S9sVariantMap *values)
{
    clear();

    std::vector<S9sString> newKeys = values->keys();

    for (uint idx = 0; idx < newKeys.size(); ++idx)
        (*this)[newKeys[idx]] = (*values)[newKeys[idx]];
}

