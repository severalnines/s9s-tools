/* 
 * Copyright (C) 2011-2017 severalnines.com
 */
#pragma once

#include "S9sGraph"

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
        };

        S9sCmonGraph();
        virtual ~S9sCmonGraph();
        
        void setGraphType(S9sCmonGraph::GraphTemplate type);

        virtual void appendValue(S9sVariant value);
        virtual void realize();
        
    private:
        GraphTemplate  m_graphType;
        S9sVariantList m_values;
};
