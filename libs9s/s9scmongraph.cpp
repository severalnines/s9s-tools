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

    /*
     * Setting up the graph to look like the type suggests.
     */
    switch (m_graphType)
    {
        case Unknown:
            S9S_WARNING("Unknown graph type.");
            break;

        case LoadAverage:
            setAggregateType(S9sGraph::Max);
            setWarningLevel(5.0);
            setErrorLevel(10.0);
            setTitle("Load on %s", STR(m_node.hostName()));
            break;

        case CpuGhz:
            setAggregateType(S9sGraph::Max);
            setTitle("CPU clock of %s (GHz)", STR(m_node.hostName()));
            break;
        
        case SqlStatements:
            setAggregateType(S9sGraph::Max);
            if (!m_values.empty())
            {
                if (m_values[0].contains("COM_INSERT"))
                {
                    setTitle("SQL statements on %s (1/sec)", 
                            STR(m_node.hostName()));
                } else if (m_values[0].contains("rows-inserted"))
                {
                    setTitle("SQL activity on %s (rows/sec)", 
                            STR(m_node.hostName()));
                }
            }
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

            case SqlStatements:
                if (value["hostid"].toInt() != m_node.id())
                    continue;

                //S9S_WARNING("[%3u] interval : %g", 
                //        idx, value["interval"].toDouble());

                if (value.contains("COM_INSERT"))
                {
                    double dval;

                    dval = 
                        value["COM_DELETE"].toDouble() +
                        value["COM_INSERT"].toDouble() + 
                        value["COM_REPLACE"].toDouble() + 
                        value["COM_SELECT"].toDouble() + 
                        value["COM_UPDATE"].toDouble();
               
                    #if 0
                    S9S_WARNING("");
                    S9S_WARNING("[%3u]     dval : %g", idx, dval);
                    S9S_WARNING("[%3u] interval : %g", 
                            idx, value["interval"].toDouble());
                    #endif

                    dval /= value["interval"].toDouble() / 1000.0;
                    #if 0
                    S9S_WARNING("[%3u]    speed : %6.4f", idx, dval);
                    #endif
                    
                    S9sGraph::appendValue(dval);
                } else if (value.contains("rows-inserted"))
                {
                    double dval;

                    dval = 
                        value["rows-deleted"].toDouble() +
                        value["rows-fetched"].toDouble() + 
                        value["rows-inserted"].toDouble() + 
                        value["rows-updated"].toDouble();

                    dval /= value["interval"].toDouble() / 1000.0;
                    
                    S9sGraph::appendValue(dval);
                } else {
                    S9sGraph::appendValue(0.0);
                }
        }
    }

    /*
     * Linking up to the parent class.
     */
    S9sGraph::realize();
}
