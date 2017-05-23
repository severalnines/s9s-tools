/* 
 * Copyright (C) 2011-2017 severalnines.com
 */
#include "s9scmongraph.h"

#define DEBUG
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
S9sCmonGraph::setNode(
        const S9sNode &node)
{
    m_node = node;
}

void 
S9sCmonGraph::appendValue(
        S9sVariant value)
{
    m_values << value;
}

void
S9sCmonGraph::realize()
{
    S9sCmonGraph::clearValues();

    switch (m_graphType)
    {
        case Unknown:
            S9S_WARNING("Unknown graph type.");
            break;

        case LoadAverage:
            setAggregateType(S9sGraph::Average);
            break;

        case CpuGhz:
            setAggregateType(S9sGraph::Max);
            break;
    }

    /*
     * Calculating the values that we actually show.
     */
    for (uint idx = 0u; idx < m_values.size(); ++idx)
    {
        S9sVariant value = m_values[idx];
    
        switch (m_graphType)
        {
            case Unknown:
                S9S_WARNING("Unknown graph type.");
                break;

            case LoadAverage:
                if (value["hostid"].toInt() != m_node.id())
                    continue;

                if (value["cpuid"].toInt() != 0)
                    continue;

                S9sGraph::appendValue(value["loadavg1"]);
                break;

            case CpuGhz:
                if (value["hostid"].toInt() != m_node.id())
                    continue;

                if (value["cpuid"].toInt() != 0)
                    continue;
                
                S9sGraph::appendValue(value["cpumhz"].toDouble() / 1000.0);
                break;

        }
    }

    /*
     * Linking up to the parent class.
     */
    S9sGraph::realize();
}
