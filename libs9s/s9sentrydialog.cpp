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
#include "s9sentrydialog.h"

#include "S9sDisplay"

#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sEntryDialog::S9sEntryDialog(
        S9sDisplay *display) :
    S9sDialog(display)
{
}

S9sEntryDialog::~S9sEntryDialog()
{
}

S9sString
S9sEntryDialog::text() const
{
    return m_entry.text();
}

void
S9sEntryDialog::setText(
        const S9sString &value)
{
    m_entry.setText(value);
}

void
S9sEntryDialog::refreshScreen()
{
    #if 0
    PRINT_LOG("S9sDialog::refreshScreen()");
    PRINT_LOG("***      y(): %d", y());
    PRINT_LOG("*** height(): %d", height());
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
S9sEntryDialog::processKey(
        int key)
{
    PRINT_LOG("S9sEntryDialog::processKey()");
    switch (key)
    {
        case S9S_KEY_ESC:
            m_cancelPressed = true;
            break;

        case S9S_KEY_ENTER:
            m_okPressed = true;
            break;

        default:
            m_entry.processKey(key);
    }
}

void
S9sEntryDialog::printLine(
        int lineIndex)
{
    const char *normal     = m_normalColor; 

    m_nChars = 0;
    ::printf("%s", normal);

    if (lineIndex == 2)
    {
        printChar("║");
        m_entry.print();
        ::printf("%s", normal);
        printChar("║");
    } else {
        S9sDialog::printLine(lineIndex);
    }
    
    ::printf("%s", TERM_NORMAL);
}

