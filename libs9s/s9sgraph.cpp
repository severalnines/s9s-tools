/* 
 * Copyright (C) 2011-2017 severalnines.com
 */
#include "s9sgraph.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sGraph::S9sGraph()
{
}

S9sGraph::~S9sGraph()
{
}

void
S9sGraph::appendValue(
        const S9sVariant &value)
{
    m_rawData << value;
}

void
S9sGraph::transformWidth(
        int width)
{
}
