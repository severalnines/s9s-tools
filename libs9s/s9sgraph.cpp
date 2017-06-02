/* 
 * Copyright (C) 2011-2017 severalnines.com
 */
#include "s9sgraph.h"

#include "S9sDateTime"
#include "S9sOptions"

#include "stdio.h"
#include "math.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

#define IS_DIVISIBLE_BY(a,b) ((int) (a) == ((int) (a) / (b)) * (b))

S9sGraph::S9sGraph() :
    m_showDensityFunction(false),
    m_aggregateType(Average),
    m_width(40),
    m_height(10),
    m_color(true),
    m_warningLevel(0.0),
    m_errorLevel(0.0),
    m_started(0),
    m_ended(0)
{
}

S9sGraph::~S9sGraph()
{
}

/**
 * \param type The aggragation type to be set.
 *
 * When there are more values in the graph than we have room for the data points
 * must be aggregated. This function controls how this will be done.
 */
void
S9sGraph::setAggregateType(
        S9sGraph::AggregateType type)
{
    m_aggregateType = type;
}

/**
 * \param start The timestamp showing where the first data point starts.
 * \param end The timestamp showing where the last data point ends in time.
 *
 * Sets the interval for the X axis.
 */
void
S9sGraph::setInterval(
        const time_t start,
        const time_t end)
{
    m_started = start;
    m_ended   = end;
}

/**
 * \param useColor True if the graph should use colors, false for monochrome.
 *
 * Sets if the graph should be color or not.
 */
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

/**
 * \returns How wide the graph will be measured in characters.
 */
int 
S9sGraph::nColumns() const
{
    return m_normalized.size() + 6;
}

/**
 * \returns The height of the graph measured as the number of lines.
 */
int 
S9sGraph::nRows() const
{
    return (int) m_lines.size();
}

/**
 * \param idx The index of the line, 0 for the first line, 1 for the second, and
 *   so forth.
 */
S9sString
S9sGraph::line(
        const int idx)
{
    if (idx >= 0 && idx < (int) m_lines.size())
        return m_lines[idx].toString();

    return S9sString();
}

/**
 * \returns The number of the original data points.
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

/**
 * This method is very similar to the realize() method of widgets in GUI
 * libraries: it will calculate and create various data sets needed to show the
 * graph. This method can be called multiple times to refresh the data.
 */
void
S9sGraph::realize()
{
    normalize(m_rawData, m_normalized, m_width);
    createLines(m_width, m_height);
}

/**
 * Sets the title of the graph.
 */
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
S9sGraph::normalize(
        S9sVariantList &original,
        S9sVariantList &normalized,
        int             newWidth)
{
    S9sVariantList tmp;
    double         origPercent;
    double         newPercent;

    S9S_DEBUG("");
    S9S_DEBUG("            width : %d", newWidth);
    S9S_DEBUG(" original.size() : %u",  original.size());
    normalized.clear();

    if (original.empty())
    {
        for (int x = 0; x < newWidth; ++x)
            normalized << 0.0;

        return;
    }
    
    for (uint origIndex = 0u; origIndex < original.size(); /*++origIndex*/)
    {    
        bool added = false;

        tmp << original[origIndex++];

        origPercent = 
            origIndex == 0 ? 0.0 :
            ((double) origIndex) / ((double) original.size());

        newPercent  = 
            normalized.size() == 0u ? 0.0 :
            (double) (normalized.size()) / (double) newWidth;

        while (newPercent <= origPercent && 
                (int) normalized.size() < newWidth) 
        { 
            normalized << aggregate(tmp);
            
            origPercent = 
                origIndex == 0 ? 0.0 :
                ((double) origIndex) / ((double) original.size());

            newPercent  = 
                normalized.size() == 0u ? 0.0 :
                (double) (normalized.size()) / (double) newWidth;

            added = true;
        }
        
        if (added)
            tmp.clear();
    }
}

void
S9sGraph::createLines(
        int newWidth,
        int newHeight)
{
    S9sOptions *options = S9sOptions::instance();
    bool        ascii = options->onlyAscii();
    S9sString   line;
    S9sVariant  biggest, smallest;
    double      mult;
   
    m_lines.clear();

    /*
     * Indenting and adding the title.
     */
    if (!m_title.empty())
    {
        int       extraSpaces;
        S9sString indent;
        S9sString titleLine;

        if (m_width >= (int) m_title.length())
        {
            // We can align the title into the middle of the graphics.
            extraSpaces = m_width - m_title.length();
            indent = S9sString(" ") * 6;
        } else {
            // We need to indent into the middle of the entire graph including
            // the left labels.
            extraSpaces = m_width + 6 - m_title.length();
        }

        // 
        if (extraSpaces > 0)
            indent += S9sString(" ") * (extraSpaces / 2);

        titleLine = indent + m_title;

        if ((int)titleLine.length() < nColumns())
            titleLine += S9sString(" ") * (nColumns() - titleLine.length());

        if (m_color)
            titleLine = TERM_BOLD + titleLine + TERM_NORMAL;
        
        m_lines << titleLine;
    }

    /*
     * The Y labels and the body of the graph.
     */
    biggest  = m_normalized.max();
    smallest = m_normalized.min();

    if (biggest.toDouble() < 0.1)
        biggest = 0.1;
    
    mult     = (newHeight / biggest.toDouble());

    S9S_DEBUG("   biggest : %g", biggest.toDouble());
    S9S_DEBUG("  smallest : %g", smallest.toDouble());
    S9S_DEBUG("      mult : %g", mult);
    S9S_DEBUG("   x range : 0 - %u", m_normalized.size() - 1);

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

        for (int x = 0; x < m_width; ++x)
        {
            double value;
            const char *c;

            if (x < (int) m_normalized.size())
                value = m_normalized[x].toDouble();
            else 
                value = 0.0;

            if (value >= topLine)
            {
                // The shown value is above this character position.
                c = ascii ? "#" : "█";
            } else if (value > baseLine && value < topLine)
            {
                // The shown value is at this position.
                const char *bg = " ";
                double      remainder;
                int         fraction;

                if (IS_DIVISIBLE_BY(y, 5))
                {
                    if (m_color)
                    {
                        bg = y == 0 ? 
                            "_" : 
                            XTERM_COLOR_DARK_GRAY "-" TERM_NORMAL;
                    } else {
                        bg = y == 0 ? "_" : "-";
                    }
                }

                remainder  = value - baseLine;
                remainder  = remainder / (topLine - baseLine);
                remainder *= 10;
                fraction   = remainder;

                switch (fraction)
                {
                    case 0:
                        if (remainder == 0.0)
                        {
                            c = bg;
                            break;
                        }

                    case 1:
                    case 2:
                        c = ascii ? bg : "▁";
                        break;

                    case 3:
                        c = ascii ? bg : "▂";
                        break;

                    case 4:
                        c = ascii ? bg : "▃";
                        break;

                    case 5:
                        c = ascii ? bg : "▄";
                        break;

                    case 6:
                        c = ascii ? bg : "▅";
                        break;
                    
                    case 7:
                        c = ascii ? bg : "▆";
                        break;
                    
                    case 8:
                    case 9:
                        c = ascii ? "." : "▇";
                        break;

                    default:
                        c = " ";
                }
            } else {
                // The shown value is under or at this baseline.
                if (y != 0)
                {
                    if (IS_DIVISIBLE_BY(y, 5))
                    {
                        if (m_color)
                            c = XTERM_COLOR_DARK_GRAY "-" TERM_NORMAL;
                        else
                            c = "-" ;
                    } else {
                        c = " ";
                    }
                } else {
                    c = ascii ? "_" : "▁";
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
            } else if (m_color) 
            {
                line += XTERM_COLOR_GREEN;
                line += c;
                line += TERM_NORMAL;
            } else {
                line += c;
            }
        }

        //printf(" %6g - %6g", baseLine, topLine);
        //printf("\n");
        m_lines << line;
        line.clear();
    }

    /*
     * The X labels.
     */
    if (m_started != 0 && m_ended != 0)
    {
        S9sDateTime start(m_started);
        S9sDateTime end(m_ended);
        S9sString   startString = start.toString(S9sDateTime::CompactFormat);
        S9sString   endString = end.toString(S9sDateTime::CompactFormat);
        S9sString   indentString;
        int         indent;

        indent = m_width - startString.length() - endString.length();
    
        if (indent > 0)
            indentString = S9sString(" ") * indent;

        m_lines << 
            "      " + startString + indentString + endString;
    }

    /*
     * The "no data" label.
     */
    if (m_rawData.empty() && m_lines.size() / 1)
    {
        S9sString labelString  = "NO DATA FOUND";
        uint      lineIndex    = m_lines.size() / 2 - 1;
        S9sString line         = m_lines[lineIndex].toString();
        int       leftIndent, rightIndent;

        leftIndent = 6 + (m_width - labelString.length()) / 2;

        line = S9sString(" ") * leftIndent + labelString;

        rightIndent = nColumns() - line.length();

        line += S9sString(" ") * rightIndent;

        m_lines[lineIndex] = line;
    }
}

/**
 * \param baseLine The value the label shows.
 *
 * This function will create a text intended to be shown on the Y axis.
 */
S9sString
S9sGraph::yLabel(
        double baseLine) const
{
    double     maxValue = m_normalized.max().toDouble();
    S9sString  retval;

    if (maxValue < 10.0)
    {
        baseLine = roundMultiple(baseLine, 0.05);
        retval.sprintf("%5.2f ", baseLine);
    } else if (maxValue >= 1000.0) 
    {
        baseLine = roundMultiple(baseLine, 10.0);
        retval.sprintf("%5.0f ", baseLine);
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

/**
 * \param graphs The graphs to print.
 * \param columnSeparator The string that will be printed between the graphs.
 *
 * Static function that prints one or more graphs horizontally. The graphs will
 * be shown side by side.
 */
void 
S9sGraph::printRow(
        S9sVector<S9sGraph *> graphs,
        S9sString             columnSeparator)
{
    for (uint lineIdx = 0; ; ++lineIdx)
    {
        bool  hadLine = false;

        for (uint idx = 0u; idx < graphs.size(); ++idx)
        {
            S9sGraph *graph = graphs[idx];

            if (lineIdx < graph->m_lines.size())
            {
                if (hadLine)
                    printf("%s", STR(columnSeparator));

                printf("%s", STR(graph->m_lines[lineIdx].toString()));
                hadLine = true;
            }
        }

        if (hadLine)
            printf("\n");
        else
            break;
    }
}
