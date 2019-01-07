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
#include "s9seditor.h"

#include "S9sDisplay"
#include "S9sUser"
#include "S9sCluster"
#include "S9sContainer"
#include "S9sNode"
#include "S9sServer"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sEditor::S9sEditor() :
    S9sWidget(),
    m_readOnly(false),
    m_saveRequested(false),
    m_objectSetTime(0),
    m_lineOffset(0),
    m_cursorX(0),
    m_cursorY(0)
{
}

S9sEditor::~S9sEditor()
{
}

S9sString
S9sEditor::path() const
{
    return m_objectPath;
}

S9sString 
S9sEditor::content() const
{
    S9sString retval;

    for (uint idx = 0u; idx < m_lines.size(); ++idx)
    {
        retval += m_lines[idx].toString();
        retval += "\n";
    }

    return retval;
}


void
S9sEditor::setIsReadOnly(
        bool value)
{
    m_readOnly = value;
}

bool
S9sEditor::isReadonly() const
{
    return m_readOnly;
}

bool
S9sEditor::isSaveRequested() const
{
    return m_saveRequested;
}

void
S9sEditor::setSaveRequested(
        bool value) 
{
    m_saveRequested = value;
}

void 
S9sEditor::processKey(
        int key)
{
    bool      doInsert = false;
    S9sString thisLine;
    S9sString subString;

    s9s_log("S9sEditor::processKey()");

    // FIXME: We should still be able to scroll.
    if (isReadonly())
        return;

    switch (key)
    {
        case S9S_KEY_DOWN:
            ++m_cursorY;
            if (m_cursorY >= 0 && m_cursorY > (int)m_lines.size() - 1)
                m_cursorY = m_lines.size() - 1;
            
            if (m_cursorX > (int) lineAt(m_cursorY).length())
                m_cursorX = lineAt(m_cursorY).length();
            break;

        case S9S_KEY_UP:
            --m_cursorY;
            if (m_cursorY < 0)
                m_cursorY = 0;
            
            if (m_cursorX > (int) lineAt(m_cursorY).length())
                m_cursorX = lineAt(m_cursorY).length();

            break;

        case S9S_KEY_LEFT:
            --m_cursorX;

            if (m_cursorX < 0)
                m_cursorX = 0;

            break;

        case S9S_KEY_RIGHT:
            ++m_cursorX;

            if (m_cursorX > (int) lineAt(m_cursorY).length())
                m_cursorX = lineAt(m_cursorY).length();

            break;

        case S9S_KEY_HOME:
            m_cursorX = 0;
            return;

        case S9S_KEY_END:
            m_cursorX = lineAt(m_cursorY).length();
            break;

        case S9S_KEY_ENTER:
            thisLine = lineAt(m_cursorY);

            subString = thisLine.substr(m_cursorX);
            m_lines[m_cursorY] = thisLine.erase(m_cursorX);

            ++m_cursorY;
            if ((int)m_lines.size() <= m_cursorY)
            {
                m_lines << subString;
            } else {
                m_lines.insert(
                        m_lines.begin() + m_cursorY, subString);
            }

            m_cursorX = 0;
            break;

        case S9S_KEY_DELETE:
            thisLine = lineAt(m_cursorY);
            
            if ((int)thisLine.length() >= m_cursorX)
            {
                thisLine.erase(m_cursorX, 1);
                m_lines[m_cursorY] = thisLine;

                if (m_cursorX > (int) thisLine.length())
                    m_cursorX = thisLine.length();
            }

            return;


        case S9S_KEY_BACKSPACE:
            if (m_cursorX > 0)
            {
                thisLine = lineAt(m_cursorY);
                s9s_log("  thisLine: '%s'", STR(thisLine));
                s9s_log(" m_cursorX: %d", m_cursorX);

                if (m_cursorX - 1 < (int) thisLine.length())
                {
                    thisLine.erase(m_cursorX - 1, 1);
                    m_cursorX--;
                    m_lines[m_cursorY] = thisLine;
                }
            } else if (m_cursorY > 0) 
            {
                m_cursorY--;
                m_cursorX = lineAt(m_cursorY).length();
                m_lines[m_cursorY] = lineAt(m_cursorY) + lineAt(m_cursorY + 1);
                m_lines.erase(m_lines.begin() + (m_cursorY + 1));
            }

            return;

        case S9S_KEY_PGDN:
            break;

        case S9S_KEY_PGUP:
            break;

        case S9S_KEY_F2:
            s9s_log("Save requested.");
            m_saveRequested = true;
            break;
    }

    if (key >= 'a' && key <= 'z')
        doInsert = true;
    else if (key >= 'A' && key <= 'Z')
        doInsert = true;
    else if (key >= '0' && key <= '9')
        doInsert = true;
    else if (key == ' ' || key == '/' || key == '*')
        doInsert = true;
    else if (key == ' ' || key == '+' || key == '-')
        doInsert = true;
    else if (key == '(' || key == ')' || key == '[' || key == ']')
        doInsert = true;
    else if (key == '!' || key == '&' || key == '[' || key == '|')
        doInsert = true;
    else if (key == '#' || key == ':' || key == ';' || key == '=')
        doInsert = true;
    else if (key == '.' || key == ',')
        doInsert = true;
    else if (key == '\'' || key == '"')
        doInsert = true;
    else if (key == '_' || key == '{' || key == '}')
        doInsert = true;

    #if 0
    if (m_lineOffset < 0)
        m_lineOffset = 0;

    if (m_lineOffset > (int) m_lines.size() - height() + 8)
        m_lineOffset = (int) m_lines.size() - height() + 8;
    #endif

    if (doInsert)
    {
        S9sString line;

        while (m_lines.size() <= (uint) m_cursorY)
            m_lines << "";

        line = m_lines[m_cursorY].toString();
        
        if (m_cursorX > (int) line.size())
            line += key;
        else
            line.insert((size_t) m_cursorX, (size_t) 1, key);

        s9s_log("line: '%s'", STR(line));
        m_lines[m_cursorY] = line;

        ++m_cursorX;
    }
}

/**
 * \param path The CDT path where the object is found.
 * \param object The serialized version of the object that is set.
 *
 * This method sets the object that is either viewed (read-only mode) or 
 * edited in the widget.
 */
void
S9sEditor::setObject(
        const S9sString     &path,
        const S9sVariantMap &object)
{
    m_objectPath    = path;
    m_object        = object;
    m_objectSetTime = time(NULL);
    m_lineOffset    = 0;

    m_lines.clear();

    if (m_object.contains("content"))
    {
        S9sString text = m_object["content"].toString();
        
        m_lines = text.split("\n", true);
        m_cursorX = 0;
        m_cursorY = 0;
        m_lineOffset = 0;
    }
}

S9sString
S9sEditor::objectPath() const
{
    return m_objectPath;
}

time_t
S9sEditor::objectSetTime() const
{
    return m_objectSetTime;
}

void
S9sEditor::printLine(
        int lineIndex)
{
    const char *normal = "\033[48;5;19m" "\033[1m\033[38;5;33m";
    //const char *header = "\033[48;5;19m" "\033[1m\033[38;5;11m";
    //const char *selection = "\033[1m\033[48;5;51m" "\033[2m\033[38;5;237m";

    m_nChars = 0;
    ::printf("%s", normal);
    if (lineIndex == 0)
    {
        // The top frame line.
        printChar("╔");
        printChar("═", width() - 1);
        printChar("╗");
    } else if (lineIndex == height() - 1)
    {
        // Last line, frame.
        printChar("╚");
        printChar("═", width() - 1);
        printChar("╝");
    } else {
        printChar("║");

        lineIndex -= 1;
        lineIndex += m_lineOffset;
            
        if (lineIndex >= 0 && lineIndex < (int)m_lines.size())
        {
            printString(m_lines[lineIndex].toString());
        }

        printChar(" ", width() - 1);
        printChar("║");
    }
}

void
S9sEditor::printString(
        const S9sString &theString)
{
    const char *normal = TERM_NORMAL "\033[48;5;19m" "\033[1m\033[38;5;33m";
    const char *bold = "\033[48;5;19m" "\033[1m\033[38;5;11m";
    S9sString   asciiString = theString;
    S9sString   colorString = theString;
    int         availableChars = width() - m_nChars - 1;
    
    if (availableChars <= 0)
        return;

    asciiString.replace("<b>",  "");
    asciiString.replace("</b>", "");
    
    colorString.replace("<b>",  bold);
    colorString.replace("</b>", normal);

    if ((int)asciiString.length() > availableChars)
    {
        asciiString.resize(availableChars);
        ::printf("%s", STR(asciiString));
    } else {
        ::printf("%s", STR(colorString));
        ::printf("%s", normal);
    }

    m_nChars += asciiString.length();
}

void
S9sEditor::printChar(
        int c)
{
    ::printf("%c", c);
    ++m_nChars;
}

void
S9sEditor::printChar(
        const char *c)
{
    ::printf("%s", c);
    ++m_nChars;
}

void
S9sEditor::printChar(
        const char *c,
        const int   lastColumn)
{
    while (m_nChars < lastColumn)
    {
        ::printf("%s", c);
        ++m_nChars;
    }
}

S9sString
S9sEditor::lineAt(
        int index)
{
    if (index >= 0 && index < (int) m_lines.size())
        return m_lines[index].toString();

    return S9sString("");
}

void
S9sEditor::showCursor()
{
    int   col = x() + m_cursorX + 2;
    int   row = y() + m_cursorY + 1;
    S9sString sequence;

    if (!hasFocus() || m_readOnly)
        return;

    sequence.sprintf("\033[%d;%dH", row, col);
    ::printf("%s", STR(sequence));
    ::printf("%s", TERM_CURSOR_ON);

    fflush(stdout);
}

