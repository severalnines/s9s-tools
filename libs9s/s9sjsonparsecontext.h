/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

#include "s9svariantmap.h"
#include "s9sparsecontext.h"

class S9sJsonParseContext :
    public S9sVariantMap,
    public S9sParseContext
{
    public:
        S9sJsonParseContext(const char *input);
        void setValues(S9sVariantMap *values);
};

extern int json_parse(S9sJsonParseContext &context);

