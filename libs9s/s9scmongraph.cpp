/* 
 * Copyright (C) 2011-2017 severalnines.com
 */
#include "s9scmongraph.h"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sCmonGraph::S9sCmonGraph() :
    S9sGraph(),
    m_graphType(Unknown)
{
}

S9sCmonGraph::~S9sCmonGraph()
{
}
        
void 
S9sCmonGraph::setGraphType(
        S9sCmonGraph::GraphTemplate type)
{
    m_graphType = type;
}
       
void 
S9sCmonGraph::appendValue(
        S9sVariant value)
{
    if (!value.isVariantMap())
    {
        S9sGraph::appendValue(value);
        return;
    }

    switch (m_graphType)
    {
        case Unknown:
            S9S_WARNING("Unknown graph type.");
            break;

        case LoadAverage:
            S9sGraph::appendValue(value["loadavg1"]);
            break;
    }
}
