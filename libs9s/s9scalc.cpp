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
#include "s9scalc.h"

#include "S9sDateTime"
#include <unistd.h>

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sCalc::S9sCalc(
        S9sRpcClient            &client) : 
    S9sDisplay(true),
    m_client(client)
{
    m_formulaEntry.setLocation(1, 2);
    m_formulaEntry.setActive(false);
}

S9sCalc::~S9sCalc()
{
}

void
S9sCalc::setSpreadsheetName(
        const S9sString &name)
{
    m_spreadsheetName = name;
}

S9sString
S9sCalc::spreadsheetName() const
{
    return m_spreadsheetName;
}

void
S9sCalc::main()
{
    S9sRpcReply reply;
    bool        success;

    success = m_client.getSpreadsheet();
    if (!success)
    {
        m_errorString = "Error getting spreadsheet.";
    } else {
        reply = m_client.reply();
        m_spreadsheet = reply["spreadsheet"].toVariantMap();
        //m_errorString = reply["spreadsheet"].toVariantMap().toString();
        //m_errorString.replace("\n", "\r\n");
        //::printf("-> \n%s\n", STR(m_errorString));
        //sleep(100);
    }


    start();

        while (true)
        {
#if 0
            while (!m_client.isAuthenticated())
            {
                m_client.maybeAuthenticate();

                if (!m_client.isAuthenticated())
                    usleep(3000000);
            }

            m_lastReply = S9sRpcReply();
            m_client.subscribeEvents(S9sMonitor::eventHandler, (void *) this);
            m_lastReply = m_client.reply();
#endif
            //sleep(1);
            usleep(10000);

        }
}

/**
 * Virtual function that called when the user pressed a key on the keyboard.
 */
void
S9sCalc::processKey(
        int key)
{
    if (m_formulaEntry.isActive())
    {
        if (key == S9S_KEY_ENTER)
        {
            m_formulaEntry.setActive(false);
            return;
        } else {
            m_formulaEntry.processKey(key);
        }

        return;
    }

    switch (key)
    {
        case 'q':
        case 'Q':
            exit(0);
            break;

        case S9S_KEY_DOWN:
            m_spreadsheet.selectedCellDown();
            m_formulaEntry.setActive(false);
            updateEntryText();
            break;

        case S9S_KEY_UP:
            m_spreadsheet.selectedCellUp();
            m_formulaEntry.setActive(false);
            updateEntryText();
            break;

        case S9S_KEY_RIGHT:
            m_spreadsheet.selectedCellRight();
            m_formulaEntry.setActive(false);
            updateEntryText();
            break;

        case S9S_KEY_LEFT:
            m_spreadsheet.selectedCellLeft();
            m_formulaEntry.setActive(false);
            updateEntryText();
            break;

        case S9S_KEY_ENTER:
            if (!m_formulaEntry.isActive())
            {
                m_formulaEntry.setActive(true);
            }
            break;
    }
}

void 
S9sCalc::processButton(
        uint button, 
        uint x, 
        uint y)
{
    S9sDisplay::processButton(button, x, y);
}

/**
 * \returns True if the program should continue refreshing the screen, false to
 *   exit.
 *
 * Virtual function that should paint the whole screen once. The mutex is 
 * locked when this method is called.
 */
bool
S9sCalc::refreshScreen()
{
    printf("%s", TERM_CURSOR_OFF);

    startScreen();
    printHeader();

    
    m_formulaEntry.print();
    printNewLine();
    
    m_spreadsheet.setScreenSize(columns(), rows() - 4);
    m_spreadsheet.print();
    //printMiddle();

    printFooter();
    m_formulaEntry.showCursor();

    return true;
}

/**
 * Printing the top part of the screen.
 */
void
S9sCalc::printHeader()
{
    S9sDateTime dt = S9sDateTime::currentDateTime();
    S9sString   title = "S9S Calc";
    const char *bold = TERM_SCREEN_TITLE_BOLD;
    const char *normal = TERM_SCREEN_TITLE;

    if (!spreadsheetName().empty())
        title = spreadsheetName();

    ::printf("%s%s%s ", bold, STR(title), normal);
    ::printf("%s ", STR(dt.toString(S9sDateTime::LongTimeFormat)));
    ::printf("0x%08x ",      lastKeyCode());
    ::printf("%02dx%02d ",   columns(), rows());

    printNewLine();
    
    // The editor comes here...
    //printNewLine();
}

void
S9sCalc::printFooter()
{
    //const char *bold   = TERM_SCREEN_TITLE_BOLD;
    const char *normal = TERM_SCREEN_TITLE;

    ::printf("%s ", normal);

    if (m_errorString.empty())
    {
        ::printf("ok");
    } else {
        ::printf("%s", STR(m_errorString));
    }
        
    // No new-line at the end, this is the last line.
    ::printf("%s", TERM_ERASE_EOL);
    ::printf("%s", TERM_NORMAL);
    fflush(stdout);    
}

void
S9sCalc::updateEntryText()
{
    int      col = m_spreadsheet.selectedCellColumn();
    int      row = m_spreadsheet.selectedCellRow();
    S9sString content = m_spreadsheet.contentString(0, col, row);

    m_formulaEntry.setText(content);
}

