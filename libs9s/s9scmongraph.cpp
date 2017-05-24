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

/**
 * This method is very similar to the realize() method of widgets in GUI
 * libraries: it will calculate and create various data sets needed to show the
 * graph. This method can be called multiple times to refresh the data.
 */
void
S9sCmonGraph::realize()
{
    time_t start = 0;
    time_t end   = 0;

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

        case CpuTemp:
            setAggregateType(S9sGraph::Max);
            setTitle("Cpu temperature on %s (℃ )", STR(m_node.hostName()));
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

        case SqlConnections:
            setAggregateType(S9sGraph::Max);
            setTitle("SQL connections on %s", STR(m_node.hostName()));
            break;
        
        case MemFree:
            setAggregateType(S9sGraph::Min);
            setTitle("Free memory on %s (GBytes)", STR(m_node.hostName()));
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

            case CpuTemp:
                if (value["hostid"].toInt() != m_node.id())
                    continue;

                if (value["cpuid"].toInt() != 0)
                    continue;

                S9sGraph::appendValue(value["cputemp"]);
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
                break;

            case SqlConnections:
                if (value["hostid"].toInt() != m_node.id())
                    continue;
               
                if (value.contains("CONNECTIONS"))
                    S9sGraph::appendValue(value["CONNECTIONS"].toDouble());
                else
                    S9sGraph::appendValue(value["connections"].toDouble());

                break;

            case MemFree:
                if (value["hostid"].toInt() != m_node.id())
                    continue;

                S9sGraph::appendValue(
                        value["ramfree"].toDouble() / 
                        (1024.0 * 1024.0 * 1024.0));
                break;
        }

        /*
         *
         */
        if (value.contains("created"))
        {
            time_t created = value["created"].toTimeT();
            time_t ended   = created + (value["interval"].toInt() / 1000);

            if (start == 0)
                start = created;

            if (end == 0)
                end = ended;

            if (start > created)
                start = created;

            if (end < ended)
                end = ended;
        }
    }

    /*
     * Setting the start time and end time for the graph so that the user can
     * have an idea what time interval is shown.
     */
    if (start == 0 && end == 0)
    {
        // If we had no timestamp that's because we had no statistics. No
        // matter, we set the last 1 minute showing some labels anyway.
        end = time(NULL);
        start = end - 60;
    }

    setInterval(start, end);

    /*
     * Linking up to the parent class.
     */
    S9sGraph::realize();
}
