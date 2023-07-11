/*
 * Severalnines Tools
 * Copyright (C) 2016  Severalnines AB
 *
 * This file is part of s9s-tools.
 *
 * s9s-tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * S9sTools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with S9sTools. If not, see <http://www.gnu.org/licenses/>.
 */
#include "s9sformat.h"

#include <stdio.h>
#include "s9soptions.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

#define KILO (1024.0)
#define MEGA (1024.0 * 1024.0)
#define GIGA (1024.0 * 1024.0 * 1024.0)
#define TERA (1024.0 * 1024.0 * 1024.0 * 1024.0)
#define PETA (1024.0 * 1024.0 * 1024.0 * 1024.0 * 1024)

S9sFormat::S9sFormat() :
    m_unit(UnitUnknown),
    m_humanreadable(false),
    m_width(0),
    m_withFieldSeparator(true),
    m_colorStart(0),
    m_colorEnd(0),
    m_alignment(AlignLeft),
    m_ellipsize(false)
{
}

S9sFormat::S9sFormat(
        const char *colorStart,
        const char *colorEnd) :
    m_unit(UnitUnknown),
    m_humanreadable(false),
    m_width(0),
    m_withFieldSeparator(true),
    m_colorStart(colorStart),
    m_colorEnd(colorEnd),
    m_alignment(AlignLeft),
    m_ellipsize(false)
{
}

S9sFormat 
S9sFormat::operator+(
        const S9sFormat &rhs)
{
    S9sFormat retval;

    retval.m_width = m_width + rhs.m_width;

    if (m_withFieldSeparator)
        retval.m_width += 1;

    return retval;
}

/**
 * Sets the justification/alignment for the format.
 */
void
S9sFormat::setRightJustify()
{
    m_alignment = AlignRight;
}

/**
 * Sets the justification/alignment for the format.
 */
void
S9sFormat::setCenterJustify()
{
    m_alignment = AlignCenter;
}

/**
 * FIXME: We don't really use this.
 */
void
S9sFormat::setColor(
        const char *colorStart,
        const char *colorEnd)
{
    m_colorStart = colorStart;
    m_colorEnd   = colorEnd;
}

void
S9sFormat::setUnit(
        const S9sFormat::Unit unit)
{
    m_unit = unit;
}

void
S9sFormat::setHumanReadable(
        const bool value)
{
    m_humanreadable = value;
}

/**
 * \returns How wide the format is with the field separator counted as one
 *   space.
 */
int
S9sFormat::realWidth() const
{
    int retval = m_width;

    if (m_withFieldSeparator)
        retval += 1;

    return retval;
}

void
S9sFormat::setWidth(
        int width)
{
    m_width = width;
    m_withFieldSeparator = false;
}

void
S9sFormat::setEllipsize(
        bool ellipsize)
{
    m_ellipsize = ellipsize;
}

/**
 * If necessary makes the format wider to accomodate the given value.
 */
void
S9sFormat::widen(
        const S9sString &value)
{
    if ((int)value.terminalLength() > m_width)
        m_width = (int) value.terminalLength();
}

/**
 * If necessary makes the format wider to accomodate the given value.
 */
void
S9sFormat::widen(
        const int value)
{
    S9sString tmp;

    tmp.sprintf("%d", value);
    widen(tmp);
}

/**
 * If necessary makes the format wider to accomodate the given value.
 */
void
S9sFormat::widen(
        const ulonglong value)
{
    S9sString tmp;

    tmp.sprintf("%llu", value);
    widen(tmp);
}

void
S9sFormat::widen(
        const double value)
{
    S9sString tmp = toString(value);
    
    if ((int)tmp.terminalLength() > m_width)
        m_width = (int) tmp.terminalLength();
}

/**
 * \param value The value to convert to a string.
 * \returns The formatted string without alignment, field separator and colors.
 *
 * Converts the double to string. Does not consider the width and the color of
 * the column, but considers the unit and the human readable flag.
 */
S9sString
S9sFormat::toString(
        const double value) const
{
    S9sString retval;

    switch (m_unit)
    {
        case UnitUnknown:
            retval.sprintf("%g", value);
            break;

        case UnitMs:
            if (!m_humanreadable)
            {
                retval.sprintf("%.0f", value);
            } else {
                if (value > 10000.0)
                {
                    retval.sprintf("%.0fs", value / 1000.0);
                } else if (value > 1000.0)
                {
                    retval.sprintf("%.2fs", value / 1000.0);
                } else if (value > 100.0)
                {
                    retval.sprintf("%.0fms", value);
                } else if (value < 1.0)
                {
                    retval.sprintf("%.0fus", value * 1000.0);
                } else {
                    retval.sprintf("%.2fms", value);
                }
            }
            break;
        
        case UnitBytes:
            retval.sprintf("%.0f", value);
            break;
    }

    return retval;
}


/**
 * Prints the value to the standard output, then prints the field separator.
 */
void
S9sFormat::printf(
        const int value) const
{
    S9sString formatString;

    if (m_width > 0)
    {
        formatString.sprintf("%%%dd", m_width);
    } else {
        formatString.sprintf("%%d", m_width);
    }

    if (m_withFieldSeparator)
        formatString += " ";

    ::printf(STR(formatString), value);
}



/**
 * Prints the value to the standard output, then prints the field separator.
 */
void
S9sFormat::printf(
        const ulonglong value) const
{
    S9sString formatString;

    if (m_width > 0)
        formatString.sprintf("%%%dllu", m_width);
    else
        formatString.sprintf("%%llu");

    if (m_withFieldSeparator)
        formatString += " ";

    ::printf(STR(formatString), value);
}

void
S9sFormat::printf(
        const double value,
        bool         color) const
{
    S9sString myValue = toString(value);
    S9sString formatString;

    if (m_width > 0)
        formatString.sprintf("%%%ds", m_width);
    else
        formatString.sprintf("%%s");

    if (m_withFieldSeparator)
        formatString += " ";
    
    if (color && m_colorStart != NULL)
        ::printf("%s", m_colorStart);

    ::printf(STR(formatString), STR(myValue));

    if (color && m_colorEnd != NULL)
        ::printf("%s", m_colorEnd);
}

/**
 * Prints the value to the standard output, then prints the field separator.
 */
void
S9sFormat::printf(
        const S9sString &value,
        bool             color) const
{
    S9sString formatString;
    S9sString myValue = value;

    if (m_width > 0)
    {
        if (m_ellipsize)
        {
            if ((int) myValue.length() > m_width)
            {
                myValue.resize(m_width - 1);
                myValue += "…";
            }
        }

        switch (m_alignment)
        {
            case AlignRight:
                formatString.sprintf("%%%ds", m_width);
                break;
            
            case AlignLeft:
                formatString.sprintf("%%-%ds", m_width);
                break;
            
            case AlignCenter:
                {
                    S9sString alignString;

                    if (m_width > (int) myValue.terminalLength())
                    {
                        alignString = 
                            S9sString(" ") * 
                            ((m_width - myValue.terminalLength()) / 2);
                    }
               
                    myValue = alignString + myValue;

                    formatString.sprintf("%%-%ds", m_width);
                }
                break;
        }
    } else {
        formatString = "%s";
    }

    if (m_withFieldSeparator)
        formatString += " ";

    if (color && m_colorStart != NULL)
        ::printf("%s", m_colorStart);

    ::printf(STR(formatString), STR(myValue));

    if (color && m_colorEnd != NULL)
        ::printf("%s", m_colorEnd);
}

void
S9sFormat::printHeader(
        const S9sString &value) 
{
    // The first thing that we print is the header, so it is not too late to
    // widen the column here.
    widen(value);
    printf(STR(value), false);
}

/**
 * Takes a value and converts it as a byte measuring value. If the 
 * -h, --human-readable command line option is not provided simply the number is
 *  is printed into the string. If the option is provided a short, human
 *  readable version will be printed (e.g. 100K or 1.1G).
 */
S9sString
S9sFormat::toSizeString(
        const ulonglong value)
{
    S9sOptions *options = S9sOptions::instance();
    bool        humanReadable = options->humanReadable();
    S9sString   retval;

    //
    if (!humanReadable)
    {
        retval.sprintf("%llu", value);
    } else {
        double dValue = value;

        if (dValue < 1024.0)
        {
            retval.sprintf("%.0f", dValue);

            return retval;
        } else if (dValue < MEGA)
        {
            retval.sprintf("%.1fK", dValue / KILO);

            if (retval.length() > 4)
                retval.sprintf("%.0fK", dValue / KILO);

            return retval;
        } else if (dValue < GIGA)
        {
            retval.sprintf("%.1fM", dValue / MEGA);

            if (retval.length() > 4)
                retval.sprintf("%.0fM", dValue / MEGA);

            return retval;
        } else if (dValue < TERA)
        {
            retval.sprintf("%.1fG", dValue / GIGA);

            if (retval.length() > 4)
                retval.sprintf("%.0fG", dValue / GIGA);

            return retval;
        } else if (dValue < PETA)
        {
            retval.sprintf("%.1fT", dValue / TERA);

            if (retval.length() > 4)
                retval.sprintf("%.0fT", dValue / TERA);

            return retval; 
        } else {
            retval.sprintf("%.1fP", dValue / PETA);

            if (retval.length() > 4)
                retval.sprintf("%.0fP", dValue / PETA);

            return retval;
        }
    }

    return retval;
}
