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
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar. If not, see <http://www.gnu.org/licenses/>.
 */
#include "s9sformat.h"

#include <stdio.h>
#include "S9sOptions"

#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sFormat::S9sFormat() :
    m_width(0),
    m_withFieldSeparator(true),
    m_colorStart(0),
    m_colorEnd(0),
    m_rightJustify(true)
{
}

void
S9sFormat::setRightJustify(
        const bool value)
{
    m_rightJustify = value;
}

void
S9sFormat::setColor(
        const char *colorStart,
        const char *colorEnd)
{
    m_colorStart = colorStart;
    m_colorEnd   = colorEnd;
}

int
S9sFormat::realWidth() const
{
    int retval = m_width;

    if (m_withFieldSeparator)
        retval += 1;

    return retval;
}

/**
 * If necessary makes the format wider to accomodate the given value.
 */
void
S9sFormat::widen(
        const S9sString &value)
{
    if ((int)value.length() > m_width)
        m_width = (int) value.length();
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
S9sFormat::printf(
        const int value) const
{
    S9sString formatString;

    if (m_width > 0)
        formatString.sprintf("%%%dd", m_width);
    else
        formatString.sprintf("%%d", m_width);

    if (m_withFieldSeparator)
        formatString += " ";

    ::printf(STR(formatString), value);
}

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

void
S9sFormat::printf(
        const S9sString &value) const
{
    S9sString formatString;

    if (m_width > 0)
    {
        if (m_rightJustify)
            formatString.sprintf("%%%ds", m_width);
        else
            formatString.sprintf("%%-%ds", m_width);
    } else {
        formatString = "%s";
    }

    if (m_withFieldSeparator)
        formatString += " ";

    if (m_colorStart != NULL)
        ::printf("%s", m_colorStart);

    ::printf(STR(formatString), STR(value));

    if (m_colorEnd != NULL)
        ::printf("%s", m_colorEnd);
}

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
        }

        retval.sprintf("%.1fK", dValue / 1024.0);
        return retval;
    }

    return retval;
}
