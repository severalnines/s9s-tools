/*
 * Severalnines Tools
 * Copyright (C) 2016-2018 Severalnines AB
 *
 * This file is part of s9s-tools.
 *
 * s9s-tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * s9s-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with s9s-tools. If not, see <http://www.gnu.org/licenses/>.
 */
#include "s9sdisplayentry.h"

#include "S9sDisplay"

#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sDisplayEntry::S9sDisplayEntry() :
    S9sWidget(),
    m_cursorPosition(0)
{
}

S9sDisplayEntry::~S9sDisplayEntry()
{
}

S9sString
S9sDisplayEntry::text() const
{
    return m_content;
}

void
S9sDisplayEntry::setText(
        const S9sString text) 
{
    m_content = text;
    m_cursorPosition = m_content.length();
}

void
S9sDisplayEntry::processKey(
        int key)
{
    if (!isActive())
        return;

    switch (key)
    {
        case S9S_KEY_LEFT:
            if (m_cursorPosition > 0)
                m_cursorPosition--;

            return;

        case S9S_KEY_RIGHT:
            if (m_cursorPosition < (int) m_content.length())
                m_cursorPosition++;

            return;

        case S9S_KEY_BACKSPACE:
            if (m_cursorPosition > 0)
            {
                m_content.erase(m_cursorPosition - 1, 1);
                m_cursorPosition--;
            }

            return;

        case S9S_KEY_DELETE:
            m_content.erase(m_cursorPosition, 1);
            if (m_cursorPosition > (int) m_content.length())
                m_cursorPosition = m_content.length();

            return;

        case S9S_KEY_HOME:
            m_cursorPosition = 0;
            return;

        case S9S_KEY_END:
            m_cursorPosition = m_content.length();
            return;

    }

    bool doInsert = false;

    if (key >= 'a' && key <= 'z')
        doInsert = true;
    else if (key >= 'A' && key <= 'Z')
        doInsert = true;
    else if (key >= '0' && key <= '9')
        doInsert = true;
    else if (key == ' ' || key == '/' || key == '*')
        doInsert = true;
    else if (key == '(' || key == ')' || key == '[' || key == ']')
        doInsert = true;
    else if (key == '!' || key == '&' || key == '[' || key == '|')
        doInsert = true;
    else if (key == '#' || key == ':' || key == ';')
        doInsert = true;

    if (doInsert)
    {
        m_content.insert((size_t) m_cursorPosition, (size_t) 1, key);
        ++m_cursorPosition;
    }
}

void
S9sDisplayEntry::print() const
{
    printf("%s", STR(m_content));
}

void
S9sDisplayEntry::showCursor()
{
    int   col = x() + m_cursorPosition;
    int   row = y();
    S9sString sequence;

    if (!isActive())
        return;

    sequence.sprintf("\033[%d;%dH", row, col);
    ::printf("%s", STR(sequence));
    ::printf("%s", TERM_CURSOR_ON);

    fflush(stdout);
}

