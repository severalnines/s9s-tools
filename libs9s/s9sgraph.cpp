/* 
 * Copyright (C) 2011-2017 severalnines.com
 */
#include "s9sgraph.h"
#include "stdio.h"

#include "math.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

#define IS_DIVISIBLE_BY(a,b) ((int) (a) == ((int) (a) / (b)) * (b))

S9sGraph::S9sGraph() :
    m_aggregateType(Average),
    m_width(40),
    m_height(10),
    m_color(true),
    m_warningLevel(0.0),
    m_errorLevel(0.0)
{
}

S9sGraph::~S9sGraph()
{
}

void
S9sGraph::setAggregateType(
        S9sGraph::AggregateType type)
{
    m_aggregateType = type;
}

void
S9sGraph::setColor(
        const bool useColor)
{
    m_color = useColor;
}

void
S9sGraph::setWarningLevel(
        double level)
{
    m_warningLevel = level;
}

void
S9sGraph::setErrorLevel(
        double level)
{
    m_errorLevel = level;
}

int 
S9sGraph::nColumns() const
{
    return m_transformed.size() + 6;
}

int 
S9sGraph::nRows() const
{
    return (int) m_lines.size();
}

S9sString
S9sGraph::line(
        const int idx)
{
    if (idx >= 0 && idx < (int) m_lines.size())
        return m_lines[idx].toString();

    return S9sString();
}

/**
 * \returns The number of the original data.
 */
int 
S9sGraph::nValues() const
{ 
    return (int) m_rawData.size(); 
}

/**
 * \returns The maximum value of the original data.
 */
S9sVariant
S9sGraph::max() const
{ 
    return m_rawData.max(); 
}

void
S9sGraph::appendValue(
        S9sVariant value)
{
    m_rawData << value;
}

void
S9sGraph::realize()
{
    transform(m_width, m_height);
    createLines(m_width, m_height);
}


void
S9sGraph::setTitle(
        const char *formatString,
        ...)        
{
    va_list arguments;
        
    va_start(arguments, formatString);
    m_title.vsprintf(formatString, arguments);
    va_end(arguments);
}

void
S9sGraph::print() const
{
    for (uint idx = 0u; idx < m_lines.size(); ++idx)
    {
        printf("%s\n", STR(m_lines[idx].toString()));
    }
}

void
S9sGraph::clearValues()
{
    m_rawData.clear();
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

        while (newPercent <= origPercent && 
                (int) m_transformed.size() < newWidth) 
        { 
            m_transformed << aggregate(tmp);
            
            origPercent = 
                origIndex == 0 ? 0.0 :
                ((double) origIndex) / ((double) m_rawData.size());

            newPercent  = 
                m_transformed.size() == 0u ? 0.0 :
                (double) (m_transformed.size()) / (double) newWidth;
        }
        
        tmp.clear();
    }
}

void
S9sGraph::createLines(
        int newWidth,
        int newHeight)
{
    S9sString  line;
    S9sVariant biggest, smallest;
    double     mult;
   
    m_lines.clear();

    /*
     * Indenting and adding the title.
     */
    if (!m_title.empty())
    {
        int       extraSpaces = m_width - m_title.length();
        S9sString indent;

        // Y labels.
        indent = S9sString(" ") * 6;

        // 
        if (extraSpaces > 0)
            indent += S9sString(" ") * (extraSpaces / 2);

        if (m_color)
        {
            m_lines << indent + TERM_BOLD + m_title + TERM_NORMAL;
        } else {
            m_lines << indent + m_title;
        }
    }

    /*
     *
     */
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
        {
            line += yLabel(baseLine);
        } else {
            line += "      ";
        }

        for (int x = 0; x < (int) m_transformed.size(); ++x)
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
                if (y != 0.0)
                {
                    if (IS_DIVISIBLE_BY(y, 5))
                    {
                        if (m_color)
                            c = XTERM_COLOR_DARK_GRAY "." TERM_NORMAL;
                        else
                            c = "." ;
                    } else {
                        c = " ";
                    }
                } else {
                    c = "▁";
                }
            }

            if (m_color && m_errorLevel > 0.0)
            {
                if (baseLine > m_errorLevel)
                {
                    line += XTERM_COLOR_RED;
                    line += c;
                    line += TERM_NORMAL;
                } else if (baseLine > m_warningLevel)
                {
                    line += XTERM_COLOR_ORANGE;
                    line += c;
                    line += TERM_NORMAL;
                } else {
                    line += XTERM_COLOR_GREEN;
                    line += c;
                    line += TERM_NORMAL;
                }
            } else {
                line += c;
            }
        }

        //printf(" %6g - %6g", baseLine, topLine);
        //printf("\n");
        m_lines << line;
        line.clear();
    }
}

S9sString
S9sGraph::yLabel(
        double baseLine) const
{
    double     maxValue = m_transformed.max().toDouble();
    S9sString  retval;

    if (maxValue < 10.0)
    {
        baseLine = roundMultiple(baseLine, 0.05);
        retval.sprintf("%5.2f ", baseLine);
    } else {
        baseLine = roundMultiple(baseLine, 0.5);
        retval.sprintf("%5.1f ", baseLine);
    }

    return retval;
}

S9sVariant 
S9sGraph::aggregate(
        const S9sVariantList &data) const
{
    S9sVariant retval;

    switch (m_aggregateType)
    {
        case Max:
            retval = data.max();
            break;

        case Min:
            retval = data.min();
            break;

        case Average:
            retval = data.average();
            break;
    }

    return retval;
}
