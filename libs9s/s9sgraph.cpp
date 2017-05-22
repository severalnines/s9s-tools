/* 
 * Copyright (C) 2011-2017 severalnines.com
 */
#include "s9sgraph.h"
#include "stdio.h"

#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sGraph::S9sGraph() :
    m_width(40),
    m_height(10)
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
S9sGraph::realize()
{
    transform(m_width, m_height);
    print(m_width, m_height);
}

void
S9sGraph::transform(
        int newWidth,
        int newHeight)
{
    S9sVariantList tmp;
    double         origPercent;
    double         newPercent;

    S9S_DEBUG("");
    S9S_DEBUG("            width : %d", newWidth);
    S9S_DEBUG(" m_rawData.size() : %u", m_rawData.size());
    m_transformed.clear();
    
    for (uint origIndex = 0u; origIndex < m_rawData.size(); /*++origIndex*/)
    {     
        tmp << m_rawData[origIndex++];

        origPercent = 
            origIndex == 0 ? 0.0 :
            ((double) origIndex) / ((double) m_rawData.size());

        newPercent  = 
            m_transformed.size() == 0u ? 0.0 :
            (double) (m_transformed.size()) / (double) newWidth;

            #if 0
            S9S_WARNING("%3d (%2.0f%%) %3d (%2.0f%%) %g",
                    (int)m_transformed.size(), newPercent * 100.0, 
                    (int)origIndex, origPercent * 100.0, 
                    tmp.max().toDouble());
            #endif
        while (newPercent <= origPercent && 
                (int) m_transformed.size() < newWidth) 
        { 
            #if 0   
            S9S_WARNING("%3d (%2.0f%%) %3d (%2.0f%%) %g",
                    (int)m_transformed.size(), newPercent * 100.0, 
                    (int)origIndex, origPercent * 100.0, 
                    tmp.max().toDouble());
            #endif
            m_transformed << tmp.max();
            //S9S_WARNING("<< %g", tmp.max().toDouble());
            
            origPercent = 
                origIndex == 0 ? 0.0 :
                ((double) origIndex) / ((double) m_rawData.size());

            newPercent  = 
                m_transformed.size() == 0u ? 0.0 :
                (double) (m_transformed.size()) / (double) newWidth;
            
            #if 0
            S9S_WARNING("%3d (%2.0f%%) %3d (%2.0f%%) %g",
                    (int)m_transformed.size(), newPercent * 100.0, 
                    (int)origIndex, origPercent * 100.0, 
                    tmp.max().toDouble());
            #endif
        }
        
        tmp.clear();
    }
}

void
S9sGraph::print(
        int newWidth,
        int newHeight)
{
    S9sVariant biggest, smallest;
    double     mult;

    
    biggest  = m_transformed.max();
    smallest = m_transformed.min();
    mult     = (newHeight / biggest.toDouble());

    S9S_DEBUG("   biggest : %g", biggest.toDouble());
    S9S_DEBUG("  smallest : %g", smallest.toDouble());
    S9S_DEBUG("      mult : %g", mult);
    S9S_DEBUG("   x range : 0 - %u", m_transformed.size() - 1);

    for (int y = newHeight; y >= 0; --y)
    {
        double baseLine = y / mult;
        double topLine  = (y + 1) / mult;

        if (y % 5 == 0)
            printf(yLabelFormat(), baseLine);
        else
            printf("      ");

        for (int x = 0; x < newWidth; ++x)
        {
            double value;
            const char *c;

            value  = m_transformed[x].toDouble();
            //value *= mult;

            if (value >= topLine)
            {
                c = "█";
            } else if (value > baseLine && value < topLine)
            {
                double remainder;
                int    fraction;

                remainder  = value - baseLine;
                remainder  = remainder / (topLine - baseLine);
                remainder *= 10;
                fraction   = remainder;

                //printf("-> %g / %d\n", remainder, fraction);
                switch (fraction)
                {
                    case 0:
                        if (remainder == 0.0)
                        {
                            c = " ";
                            break;
                        }

                    case 1:
                    case 2:
                        c = "▁";
                        break;

                    case 3:
                        c = "▂";
                        break;

                    case 4:
                        c = "▃";
                        break;

                    case 5:
                        c = "▄";
                        break;

                    case 6:
                        c = "▅";
                        break;
                    
                    case 7:
                        c = "▆";
                        break;
                    
                    case 8:
                    case 9:
                        c = "▇";
                        break;

                    default:
                        c = " ";
                }
            } else {
                c = " ";
            }

            printf("%s", c);
        }

        //printf(" %6g - %6g", baseLine, topLine);
        printf("\n");
    }
}

const char *
S9sGraph::yLabelFormat() const
{
    double maxValue = m_transformed.max().toDouble();

    if (maxValue < 10.0)
    {
        return "%5.2f ";
    } else {
        return "%5.1f ";
    }
}
