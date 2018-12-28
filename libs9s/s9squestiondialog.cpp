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
#include "s9squestiondialog.h"

#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sQuestionDialog::S9sQuestionDialog(
        S9sDisplay *display) :
    S9sDialog(display)
{
}

S9sQuestionDialog::~S9sQuestionDialog()
{
}

void
S9sQuestionDialog::printLine(
        int lineIndex)
{
    const char *normal     = TERM_NORMAL "\033[48;5;160m";

    m_nChars = 0;
    ::printf("%s", normal);

    if (lineIndex == 0)
    {
        S9sString title = "Delete";

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
        printString("Delete CDT entry?");
        printChar(" ", width() - 1);
        printChar("║");
    } else if (lineIndex + 2 == height())
    {
        printChar("║");
        printString("[  OK  ] [Cancel]");
        printChar(" ", width() - 1);
        printChar("║");
    } else if (lineIndex + 1 == height())
    {
        // Last line, frame.
        printChar("╚");
        printChar("═", width() - 1);
        printChar("╝");
    } else {
        printChar("║");
        printChar(" ", width() - 1);
        printChar("║");
    }

    ::printf("%s", TERM_NORMAL);
}
