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
#include "s9sdisplay.h"

#include "S9sOptions"
#include "S9sCluster"
#include "S9sContainer"
#include "S9sRpcReply"
#include "S9sMutexLocker"
#include "S9sDateTime"

#define DEBUG
//#define WARNING
#include "s9sdebug.h"

#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdio.h>

struct termios orig_termios1;

void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios1);
    ::printf("%s", TERM_CURSOR_ON);
    ::printf("%s", TERM_AUTOWRAP_ON);
}

void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios1);
    memcpy(&new_termios, &orig_termios1, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
    
    ::printf("%s", TERM_CURSOR_OFF);
    ::printf("%s", TERM_AUTOWRAP_OFF);
}

int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(0, &fds);

    return select(1, &fds, NULL, NULL, &tv);
}


S9sDisplay::S9sDisplay() :
    S9sThread(),
    m_refreshCounter(0),
    m_lastKey1(0),
    m_columns(0),
    m_rows(0),
    m_selectionIndex(0)
{
    S9sOptions       *options = S9sOptions::instance();
    bool              success;

    set_conio_terminal_mode();

    m_outputFileName = options->outputFile();
    if (!m_outputFileName.empty())
    {
        m_outputFile = S9sFile(m_outputFileName);
        if (m_outputFile.exists())
        {
            PRINT_ERROR("File '%s' already exists.", STR(m_outputFileName));
            exit(1);
        }

        success = m_outputFile.openForAppend();
        if (!success)
        {
            PRINT_ERROR("%s", STR(m_outputFile.errorString()));
            exit(1);
        }

        m_outputFile.close();

    }
}

S9sDisplay::~S9sDisplay()
{
    reset_terminal_mode();
}

int 
S9sDisplay::exec()
{
    do {
        bool refreshed = false;

        // Reading the key the user may have hit.
        if (kbhit())
        {
            int code;

            m_lastKey1 = 0;
            code = read(fileno(stdin), (void*)&m_lastKey1, 3);
            if (code < 0)
            {
                S9S_WARNING("code: %d", code);
            }

            m_mutex.lock();
            processKey(m_lastKey1);
            refreshScreen();
            refreshed = true;
            m_mutex.unlock();
        }

        // Refreshing the screen.
        if (!refreshed)
        {
            m_mutex.lock();
            refreshScreen();
            m_mutex.unlock();
            usleep(500000);
        }
            
    } while (!shouldStop());

    return 0;
}

char
S9sDisplay::rotatingCharacter() const
{
    char charset[] = { '/', '-', '\\',  '|' };

    return charset[m_refreshCounter % 3];
}

/**
 * This method should be called when a new screen update cycle is started. This
 * will jump the cursor to the upper left corner as well as doing some other
 * things for the screen refresh.
 */
void
S9sDisplay::startScreen()
{
    struct winsize w;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    if (m_refreshCounter == 0 || 
            m_columns == w.ws_col ||
            m_rows    == w.ws_row)
    {
        //::printf("%s", TERM_CLEAR_SCREEN);
        ::printf("%s", TERM_HOME);
    } else {
        ::printf("%s", TERM_HOME);
    }
    
    m_columns = w.ws_col;
    m_rows    = w.ws_row;
    m_lineCounter = 0;
}

/**
 * This method will print one message on the middle of the screen.
 */
void
S9sDisplay::printMiddle(
        const S9sString text)
{
    int nSpaces;

    for (;m_lineCounter < m_rows / 2;)
    {
        ::printf("%s", TERM_ERASE_EOL);
        ::printf("\r\n");
        ++m_lineCounter;
    }

    nSpaces = (m_columns - text.length()) / 2;
    for (;nSpaces > 0;--nSpaces)
        ::printf(" ");

    ::printf("%s", STR(text));
    ::printf("%s", TERM_ERASE_EOL);
    ::printf("\r\n");
    ++m_lineCounter;
}


