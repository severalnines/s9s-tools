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
S9sGraph::transform(
        int newWidth)
{
    S9sVariantList transformed;
    S9sVariantList tmp;
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

        if (newPercent < origPercent)
        {
            transformed << tmp.average();
            tmp.clear();
        } else {
            if (origIndex < (int) m_rawData.size())
            {
                tmp << m_rawData[origIndex];
            }

            ++origIndex;
        }

        if (transformed.size() == (uint) newWidth)
            break;
    }

    S9sVariant biggest = transformed.max();

    S9S_WARNING("");
    S9S_WARNING("  biggest : %g", biggest.toDouble());
    for (uint idx = 0; idx < transformed.size(); ++idx)
    {
        S9S_WARNING("%5u %f", idx, transformed[idx].toDouble());
    }
}
