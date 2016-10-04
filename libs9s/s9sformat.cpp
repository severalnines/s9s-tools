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

S9sFormat::S9sFormat() :
    m_width(0),
    m_withFieldSeparator(true)
{
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

void
S9sFormat::printf(
        const int value) const
{
    S9sString formatString;

    formatString.sprintf("%%%dd", m_width);

    if (m_withFieldSeparator)
        formatString += " ";

    ::printf(STR(formatString), value);
}

