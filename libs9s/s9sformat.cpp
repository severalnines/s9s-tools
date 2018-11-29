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
#include "S9sOptions"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

#define KILO (1024.0)
#define MEGA (1024.0 * 1024.0)
#define GIGA (1024.0 * 1024.0 * 1024.0)
#define TERA (1024.0 * 1024.0 * 1024.0 * 1024.0)
#define PETA (1024.0 * 1024.0 * 1024.0 * 1024.0 * 1024)

S9sFormat::S9sFormat() :
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
        formatString.sprintf("%%llu", m_width);

    if (m_withFieldSeparator)
        formatString += " ";

    ::printf(STR(formatString), value);
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
