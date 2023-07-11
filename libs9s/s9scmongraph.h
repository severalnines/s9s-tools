/* 
 * Copyright (C) 2011-2017 severalnines.com
 */
#pragma once

#include "s9sgraph.h"
#include "s9snode.h"

/**
 * A graph that understands Cmon Statistical data.
 */
class S9sCmonGraph : public S9sGraph
{
    public:
        enum GraphTemplate
        {
            Unknown,
            LoadAverage,
            CpuGhz,
            CpuTemp,
            CpuSys,
            CpuIdle,
            CpuUser,
            CpuIoWait,
            SqlStatements,
            SqlConnections,
            SqlCommits,
            SqlReplicationLag,
            SqlQueries,
            SqlSlowQueries,
            SqlOpenTables,
            MemUtil,
            MemFree,
            SwapFree,
            DiskFree,
            DiskReadSpeed,
            DiskWriteSpeed,
            DiskReadWriteSpeed,
            DiskUtilization,
            NetReceivedSpeed,
            NetSentSpeed,
            NetReceiveErrors,
            NetTransmitErrors,
            NetErrors,
            NetSpeed,
        };

        S9sCmonGraph();
        virtual ~S9sCmonGraph();
        
        bool setGraphType(S9sCmonGraph::GraphTemplate type);
        bool setGraphType(const S9sString &graphType);

        void setFilter(
                const S9sString  &filterName,
                const S9sVariant &filterValue);

        void setNode(const S9sNode &node);

        virtual void appendValue(S9sVariant value);
        virtual void realize();
       
        static S9sCmonGraph::GraphTemplate 
            stringToGraphTemplate(
                    const S9sString &theString);

        static S9sString
            statName(
                    const S9sCmonGraph::GraphTemplate graphTemplate);

    private:
        static S9sVariantMap sm_templateNames;
        
    private:
        GraphTemplate  m_graphType;
        S9sVariantList m_values;
        S9sNode        m_node;
        S9sString      m_filterName;
        S9sVariant     m_filterValue;
};
