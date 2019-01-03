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
    m_objectSetTime(0),
    m_previewLineOffset(0),
    m_cursorX(0),
    m_cursorY(0)
{
}

S9sEditor::~S9sEditor()
{
}

void 
S9sEditor::processKey(
        int key)
{
    bool      doInsert = false;
    S9sString thisLine;

    s9s_log("S9sEditor::processKey()");
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
            ++m_cursorY;
            
            if ((int)m_lines.size() <= m_cursorY)
            {
                m_lines << "";
            } else {
                m_lines.insert(
                        m_lines.begin() + m_cursorY, "");
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
            }

            return;

        case S9S_KEY_PGDN:
            break;

        case S9S_KEY_PGUP:
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

    #if 0
    if (m_previewLineOffset < 0)
        m_previewLineOffset = 0;

    if (m_previewLineOffset > (int) m_lines.size() - height() + 8)
        m_previewLineOffset = (int) m_lines.size() - height() + 8;
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

void
S9sEditor::setInfoController(
        const S9sString &hostName,
        const int        port,
        const bool       useTls)
{
    m_hostName = hostName;
    m_port     = port;
    m_useTls   = useTls;
}

void
S9sEditor::setInfoRequestName(
        const S9sString  requestName)
{
    m_requestName = requestName;
}

void
S9sEditor::setInfoObject(
        const S9sString     &path,
        const S9sVariantMap &theMap)
{
    m_objectPath    = path;
    m_object        = theMap;
    m_objectSetTime = time(NULL);
    m_previewLineOffset = 0;

    m_lines.clear();
}

S9sString
S9sEditor::objectPath() const
{
    return m_objectPath;
}

S9sString
S9sEditor::controllerUrl() const
{
    S9sString retval;

    retval.sprintf(
            "%s://%s:%d",
            m_useTls ? "https" : "http",
            STR(m_hostName),
            m_port);

    return retval;
}

void
S9sEditor::setInfoLastReply(
        const S9sRpcReply &reply)
{
    m_lastReply = reply;
}

void
S9sEditor::setInfoNode(
        const S9sTreeNode &node)
{
    m_node = node;
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
        lineIndex += m_previewLineOffset;
            
        if (lineIndex >= 0 && lineIndex < (int)m_lines.size())
        {
            printString(m_lines[lineIndex].toString());
        }

        printChar(" ", width() - 1);
        printChar("║");
    }
}


void
S9sEditor::printLinePreviewReply(
        int lineIndex)
{
    if (lineIndex == 0)
    {
        printChar("║");
        printChar(" ");

        if (m_object.contains("error_string"))
            printString(m_object.at("error_string").toString());

        printChar(" ", width() - 1);
        printChar("║");
    } else {
        printChar("║");

        lineIndex += m_previewLineOffset;
            
        if (lineIndex >= 0 && lineIndex < (int)m_lines.size())
            printString(m_lines[lineIndex].toString());

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
S9sEditor::printNameValue(
        const S9sString &name,
        const S9sString &value)
{
    const char *normal = "\033[48;5;19m" "\033[1m\033[38;5;33m";
    const char *header = "\033[48;5;19m" "\033[1m\033[38;5;11m";
    S9sString   tmp;

    tmp.sprintf("%11s: ", STR(name));
    ::printf("%s", STR(tmp));
    m_nChars += tmp.length();
   
    ::printf("%s", header);
    ::printf("%s", STR(value));
    ::printf("%s", normal);
    m_nChars += value.length();
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

    if (!hasFocus())
        return;

    sequence.sprintf("\033[%d;%dH", row, col);
    ::printf("%s", STR(sequence));
    ::printf("%s", TERM_CURSOR_ON);

    fflush(stdout);
}

