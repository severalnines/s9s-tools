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
#include "s9sdialog.h"

#include "S9sDisplay"

#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sDialog::S9sDialog(
        S9sDisplay *display) :
    S9sWidget(),
    m_display(display)
{
}

S9sDialog::~S9sDialog()
{
}

void
S9sDialog::processKey(
        int key)
{
    m_entry.processKey(key);
}

void
S9sDialog::refreshScreen()
{
    #if 0
    s9s_log("S9sDialog::refreshScreen()");
    s9s_log("***      y(): %d", y());
    s9s_log("*** height(): %d", height());
    #endif

    alignCenter();
    m_entry.setLocation(x() + 1, y() + 2);
    m_entry.setSize(width() - 2, 1);

    for (int row = y(); row < y() + height(); ++row)
    {
        S9sDisplay::gotoXy(x(), row);
        printLine(row - y());
    }

    m_entry.setHasFocus(true);
    m_entry.showCursor();
    fflush(stdout);
}

void
S9sDialog::printLine(
        int lineIndex)
{
    const char *normal     = TERM_NORMAL "\033[47m" "\033[90m";

    m_nChars = 0;
    ::printf("%s", normal);

    if (lineIndex == 0)
    {
        S9sString title = "Create Folder";

        if (title.empty())
        {
            printChar("╔");
            printChar("═", width() - 1);
            printChar("╗");
        } else {
            int titleStart;
        
            printChar("╔");

            title = " " + title + " ";
            titleStart = (width() - 2 - title.length()) / 2;
            if (titleStart >= 0)
            {
                printChar("═", titleStart);
            }
            
            printString(title);
            printChar("═", width() - 1);
            printChar("╗");
        }
    } else if (lineIndex == 1)
    {
        printChar("║");
        printString("Enter folder name:");
        printChar(" ", width() - 1);
        printChar("║");
    } else if (lineIndex == 2)
    {
        printChar("║");
        m_entry.print();
        ::printf("%s", normal);
        printChar("║");
    } else if (lineIndex + 1 == height())
    {
        // Last line, frame.
        printChar("╚");
        printChar("═", width() - 1);
        printChar("╝");
    } else if (lineIndex + 2 == height())
    {
        printChar("║");
        printString("[  OK  ] [Cancel]");
        printChar(" ", width() - 1);
        printChar("║");

    } else {
        printChar("║");
        printChar(" ", width() - 1);
        printChar("║");
    }
    
    ::printf("%s", TERM_NORMAL);
}

void
S9sDialog::printChar(
        const char *c)
{
    ::printf("%s", c);
    ++m_nChars;
;}

void
S9sDialog::printChar(
        const char *c,
        const int   lastColumn)
{
    while (m_nChars < lastColumn)
    {
        ::printf("%s", c);
        ++m_nChars;
    }
}

void
S9sDialog::printString(
        const S9sString &theString)
{
    S9sString  myString = theString;
    int        availableChars = width() - m_nChars - 1;
    
    if (availableChars <= 0)
        return;

    if ((int)theString.length() > availableChars)
        myString.resize(availableChars);

    ::printf("%s", STR(myString));
    m_nChars += myString.length();
}

void
S9sDialog::alignCenter()
{
    if (m_display != NULL)
    {
        int x = (m_display->width() - width()) / 2;
        int y = (m_display->height() - height()) / 2;

        setLocation(x, y);
    }
}
