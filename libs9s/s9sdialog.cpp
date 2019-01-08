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
    m_display(display),
    m_okButton("OK"),
    m_cancelButton("Cancel"),
    m_okPressed(false),
    m_cancelPressed(false)
{
    m_normalColor = TERM_NORMAL "\033[47m" "\033[90m";
}

S9sDialog::~S9sDialog()
{
}

void
S9sDialog::setTitle(
        const S9sString &text)
{
    m_title = text;
}

S9sString
S9sDialog::title() const
{
    return m_title;
}

void
S9sDialog::setMessage(
        const S9sString &text)
{
    S9sVariantList lines = text.split("\n");

    m_message1 = "";
    m_message2 = "";

    if (lines.size() > 0)
        m_message1 = lines[0].toString();

    if (lines.size() > 1)
        m_message2 = lines[1].toString();

}

S9sString
S9sDialog::message() const
{
    return m_message1;
}

bool
S9sDialog::isOkPressed() const
{
    return m_okPressed;
}

void 
S9sDialog::setIsOkPressed(
        bool value)
{
    m_okPressed = value;
}

bool
S9sDialog::isCancelPressed() const
{
    return m_cancelPressed;
}

S9sString
S9sDialog::text() const
{
    return "";
}
     
void
S9sDialog::setText(
        const S9sString &value)
{
    // ignored
}

void
S9sDialog::processKey(
        int key)
{
    s9s_log("S9sDialog::processKey()");
    switch (key)
    {
        case S9S_KEY_ESC:
            m_cancelPressed = true;
            break;

        case S9S_KEY_ENTER:
            m_okPressed = true;
            break;
    }
}

void
S9sDialog::refreshScreen()
{
    alignCenter();

    for (int row = y(); row < y() + height(); ++row)
    {
        S9sDisplay::gotoXy(x(), row);
        printLine(row - y());
    }

    fflush(stdout);
}

void
S9sDialog::printLine(
        int lineIndex)
{
    const char *normal     = m_normalColor; 

    m_nChars = 0;
    ::printf("%s", normal);

    if (lineIndex == 0)
    {
        S9sString myTitle = title();

        if (myTitle.empty())
        {
            printChar("╔");
            printChar("═", width() - 1);
            printChar("╗");
        } else {
            int titleStart;
        
            printChar("╔");

            myTitle = " " + myTitle + " ";
            titleStart = (width() - 2 - myTitle.length()) / 2;
            if (titleStart >= 0)
            {
                printChar("═", titleStart);
            }
            
            printString(myTitle);
            printChar("═", width() - 1);
            printChar("╗");
        }
    } else if (lineIndex == 1)
    {
        printChar("║");
        printString(m_message1);
        printChar(" ", width() - 1);
        printChar("║");
    } else if (lineIndex == 2)
    {
        printChar("║");
        printString(m_message2);
        printChar(" ", width() - 1);
        printChar("║");
    } else if (lineIndex + 1 == height())
    {
        // Last line, frame.
        printChar("╚");
        printChar("═", width() - 1);
        printChar("╝");
    } else if (lineIndex + 2 == height())
    {
        int buttonWidth;

        printChar("║");

        buttonWidth = m_okButton.width() + 1 + m_cancelButton.width();
        printChar(" ", width() - 1 - buttonWidth);

        m_okButton.print();
        printChar(" ");
        m_cancelButton.print();
        m_nChars += buttonWidth;


        //printString("[  OK  ] [Cancel]");
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
