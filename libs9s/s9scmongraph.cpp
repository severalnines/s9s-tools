/* 
 * Copyright (C) 2011-2017 severalnines.com
 */
#include "s9scmongraph.h"

#include "S9sOptions"

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
        
/**
 * \param type The type to set.
 *
 * Sets the type of the graph. The type controls how the data will be
 * interpreted, what the title will show and how the graph will look like.
 */
bool
S9sCmonGraph::setGraphType(
        S9sCmonGraph::GraphTemplate type)
{
    m_graphType = type;
    return true;
}

/**
 * \param graphType The string representing of the graph type as it is described
 *   in the manual page.
 *
 * Overloaded version that accepts UI strings to set the graph type.
 */
bool
S9sCmonGraph::setGraphType(
        const S9sString &graphType)
{
    if (graphType == "cpuclock" || graphType == "cpughz")
    {
        return setGraphType(S9sCmonGraph::CpuGhz);
    } else if (graphType == "cpuload" || graphType == "load") 
    {
        return setGraphType(S9sCmonGraph::LoadAverage);
    } else if (graphType == "cputemp") 
    {
        return setGraphType(S9sCmonGraph::CpuTemp);
    } else if (graphType == "sqlcommands" || graphType == "sqlstatements")
    {
        return setGraphType(S9sCmonGraph::SqlStatements);
    } else if (graphType == "sqlconnections")
    {
        return setGraphType(S9sCmonGraph::SqlConnections);
    } else if (graphType == "sqlconnections")
    {
        return setGraphType(S9sCmonGraph::SqlConnections);
    } else if (graphType == "memutil")
    {
        return setGraphType(S9sCmonGraph::MemUtil);
    } else if (graphType == "memfree" || graphType == "ramfree")
    {
        return setGraphType(S9sCmonGraph::MemFree);
    } else if (graphType == "swapfree")
    {
        return setGraphType(S9sCmonGraph::SwapFree);
    }    

    return false;
}

/**
 * \param node The node to set.
 *
 * Sets the node for the graph. When the node shows data for one specific node
 * this is going to be the node to be used to filter the dataset.
 */
void
S9sCmonGraph::setNode(
        const S9sNode &node)
{
    m_node = node;
}

/**
 * \param value The data to be appended to the data in the graph.
 *
 * This is the function where the data received from the Cmon Controller should
 * be added to the graph. The value should be a map with a statistical sample 
 * in it as it is received from the controller.
 */
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
    S9sOptions *options    = S9sOptions::instance();
    S9sString   nodeFormat = "%N";
    S9sString   hostName;
    time_t      start = 0;
    time_t      end   = 0;

    /*
     * Finding out how the user would like to see the node names.
     */
    if (options->hasNodeFormat())
        nodeFormat = options->nodeFormat();

    hostName = m_node.toString(false, nodeFormat);
    
    /*
     *
     */
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
            setTitle("Load on %s", STR(hostName));
            break;

        case CpuTemp:
            setAggregateType(S9sGraph::Max);
            setTitle("Cpu temperature on %s (℃ )", STR(hostName));
            break;

        case CpuGhz:
            setAggregateType(S9sGraph::Max);
            setTitle("CPU clock of %s (GHz)", STR(hostName));
            break;
        
        case SqlStatements:
            setAggregateType(S9sGraph::Max);
            if (!m_values.empty())
            {
                if (m_values[0].contains("COM_SELECT") || 
                        m_values[0].contains("COM_INSERT"))
                {
                    setTitle("SQL statements on %s (1/sec)", STR(hostName));
                } else if (m_values[0].contains("rows-inserted"))
                {
                    setTitle("SQL activity on %s (rows/sec)", STR(hostName));
                }
            }
            break;

        case SqlConnections:
            setAggregateType(S9sGraph::Max);
            setTitle("SQL connections on %s", STR(hostName));
            break;

        case MemUtil:
            setAggregateType(S9sGraph::Max);
            setTitle("Memory utilization on %s (%%)", STR(hostName));
            break;
        
        case MemFree:
            setAggregateType(S9sGraph::Min);
            setTitle("Free memory on %s (GBytes)", STR(hostName));
            break;
        
        case SwapFree:
            setAggregateType(S9sGraph::Min);
            setTitle("Free swap on %s (GBytes)", STR(hostName));
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

                if (value.contains("COM_SELECT") || 
                        value.contains("COM_INSERT"))
                {
                    double dval;

                    dval = 
                        value["COM_DELETE"].toDouble() +
                        value["COM_INSERT"].toDouble() + 
                        value["COM_REPLACE"].toDouble() + 
                        value["COM_SELECT"].toDouble() + 
                        value["COM_UPDATE"].toDouble();
               
                    dval /= value["interval"].toDouble() / 1000.0;
                    
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
            
            case MemUtil:
                if (value["hostid"].toInt() != m_node.id())
                    continue;

                S9sGraph::appendValue(
                        value["memoryutilization"].toDouble() * 100.0);
                break;

            case MemFree:
                if (value["hostid"].toInt() != m_node.id())
                    continue;

                S9sGraph::appendValue(
                        value["ramfree"].toDouble() / 
                        (1024.0 * 1024.0 * 1024.0));
                break;
            
            case SwapFree:
                if (value["hostid"].toInt() != m_node.id())
                    continue;

                S9sGraph::appendValue(
                        value["swapfree"].toDouble() / 
                        (1024.0 * 1024.0 * 1024.0));
                break;
        }

        /*
         * Finding the timestamps of the first and last samples.
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
