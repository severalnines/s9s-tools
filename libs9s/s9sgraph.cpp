/* 
 * Copyright (C) 2011-2017 severalnines.com
 */
#include "s9sgraph.h"
#include "stdio.h"

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
        int newWidth,
        int newHeight)
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

    S9sVariant biggest;
    double     mult;

    
    biggest = transformed.max();
    mult    = (newHeight / biggest.toDouble());

    S9S_DEBUG("  biggest : %g", biggest.toDouble());
    S9S_DEBUG("  mult    : %g", mult);

    for (int y = newHeight; y >= 0; --y)
    {
        double baseLine = y / mult;

        if (y % 5 == 0)
            printf("%4.2f ", baseLine);
        else
            printf("     ");

        for (int x = 0; x < newWidth; ++x)
        {
            double value = transformed[x].toDouble() * mult;
            const char *c = " ";

            if (value >= y)
                c = "█";

            printf("%s", c);
        }

        printf("\n");
    }
}
