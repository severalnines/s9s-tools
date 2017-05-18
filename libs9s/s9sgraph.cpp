/* 
 * Copyright (C) 2011-2017 severalnines.com
 */
#include "s9sgraph.h"

#define DEBUG
#define WARNING
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
        int newWidth)
{
    S9sVariantList transformed;
    int            origIndex;
    double         origPercent;
    double         newPercent;

    S9S_DEBUG("");
    S9S_DEBUG("            width : %d", newWidth);
    S9S_DEBUG(" m_rawData.size() : %u", m_rawData.size());
    
    origIndex = 0;
    while (true)
    {
        origPercent = 
            origIndex == 0 ? 0.0 :
            (double) origIndex / (double) m_rawData.size();

        newPercent  = 
            transformed.size() == 0u ? 0.0 :
            (double) transformed.size() / (double) newWidth;

        S9S_DEBUG("orig: %.6f new: %.6f (%3u, %3u)", 
                origPercent, newPercent,
                origIndex, transformed.size());

        if (origPercent < newPercent)
        {
            ++origIndex;
        } else {
            transformed << 0.0;
        }

        if (transformed.size() == (uint) newWidth)
            break;
    }
}
