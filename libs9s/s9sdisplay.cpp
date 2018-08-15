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

    ::printf("%s", "\e[?9l");
    ::printf("%s", "\e[?47l");
}

/**
 * \param interactive If this is true the cursor will be disabled, a new screen
 *   buffer will be used (no scroll in the terminal), the automatic line wrap
 *   will be disabled, the mouse tracking will be enabled, so a screen oriented
 *   UI will be provided.
 */
S9sDisplay::S9sDisplay(
        bool interactive) :
    S9sThread(),
    m_refreshCounter(0),
    m_columns(0),
    m_rows(0),
    m_isStopped(0)
{
    m_lastKeyCode.lastKeyCode = 0;
    m_lastButton = 0;
    m_lastX      = 0;
    m_lastY      = 0;

    setConioTerminalMode(interactive);
}

S9sDisplay::~S9sDisplay()
{
    reset_terminal_mode();
}

bool
S9sDisplay::setOutputFileName(
        const S9sString &fileName)
{
    bool success = true;

    m_outputFileName = fileName;
    if (!m_outputFileName.empty())
    {
        m_outputFile = S9sFile(m_outputFileName);

        // FIXME: Here we do exit.
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
    } else {
        m_outputFile = S9sFile();
    }

    return success;
}

bool
S9sDisplay::setInputFileName(
        const S9sString &fileName)
{
    bool success = true;

    m_inputFileName = fileName;
    if (!m_inputFileName.empty())
    {
        m_inputFile = S9sFile(m_inputFileName);

        // FIXME: Here we do exit.
        if (!m_inputFile.exists())
        {
            PRINT_ERROR("Input file '%s' does not exist.", STR(fileName));
            exit(1);
        }
    } else {
        m_inputFile = S9sFile();
    }

    return success;
}

bool
S9sDisplay::hasInputFile() const
{
    return !m_inputFileName.empty();
}

int
S9sDisplay::lastKeyCode() const
{
    return m_lastKeyCode.lastKeyCode;
}

char
S9sDisplay::rotatingCharacter() const
{
    char charset[] = { '/', '-', '\\',  '|' };

    return charset[m_refreshCounter % 3];
}

/**
 * \returns How many columns the terminal has.
 */
int
S9sDisplay::columns() const
{
    return m_columns;
}

/**
 * \returns How many rows the terminal has.
 */
int 
S9sDisplay::rows() const
{
    return m_rows;
}

void 
S9sDisplay::gotoXy(
        int x,
        int y)
{
    S9sString sequence;

    sequence.sprintf("\033[%d;%dH", y, x);
    ::printf("%s", STR(sequence));
}

int 
S9sDisplay::exec()
{
    bool refreshOk = true;
    do {
        bool refreshed = false;

        // Reading the key the user may have hit.
        if (kbhit())
        {
            int code;

            m_lastKeyCode.lastKeyCode = 0;
            code = read(fileno(stdin), (void*)&m_lastKeyCode, 6);
            if (code < 0)
            {
                S9S_WARNING("code: %d", code);
            }

            // Processing the input.
            m_mutex.lock();

            if (m_lastKeyCode.inputBuffer[0] == 0x1b &&
                    m_lastKeyCode.inputBuffer[1] == 0x5b &&
                    m_lastKeyCode.inputBuffer[2] == 0x4d)
            {
                uint btn = m_lastKeyCode.inputBuffer[3] - 32;
                uint x   = m_lastKeyCode.inputBuffer[4] - 32;
                uint y   = m_lastKeyCode.inputBuffer[5] - 32;
                processButton(btn, x, y);
                #if 0
                ::printf ("\n\rbutton:%u\n\rx:%u\n\ry:%u\n\n\r", btn, x, y);
                for (int idx = 0; idx < 6; ++idx)
                {
                    printf("[%d] 0x%x\n\r", 
                            idx,
                            (int)m_lastKeyCode.inputBuffer[idx]);
                }

                sleep(1);
                #endif
            } else {
                processKey(m_lastKeyCode.lastKeyCode);
            }

            refreshOk = refreshScreen();
            refreshed = true;
            m_mutex.unlock();
        }

        // Refreshing the screen.
        if (!refreshed)
        {
            m_mutex.lock();
            refreshOk = refreshScreen();
            m_mutex.unlock();
        }
            
        for (int idx = 0; idx < 100; ++idx)
        {
            if (kbhit())
                break;

            usleep(10000);
        }
    } while (!shouldStop() && refreshOk);

    return 0;
}

/**
 * \param button The mouse button code.
 * \param x The x coordinate measured in characters.
 * \param y The y coordinate measured in characters.
 *
 * Virtual function that is called when the user pressed a mouse button. The
 * default implementation simply stores the mouse press event so that debug
 * methods can print it.
 */
void 
S9sDisplay::processButton(
        uint button, 
        uint x, 
        uint y)
{
    m_lastButton = button;
    m_lastX      = x;
    m_lastY      = y;
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
            columns() == w.ws_col ||
            rows()    == w.ws_row)
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

    for (;m_lineCounter < rows() / 2;)
    {
        ::printf("%s", TERM_ERASE_EOL);
        ::printf("\r\n");
        ++m_lineCounter;
    }

    nSpaces = (m_columns - text.length()) / 2;
    for (;nSpaces > 0; --nSpaces)
        ::printf(" ");

    ::printf("%s", STR(text));
    ::printf("%s", TERM_ERASE_EOL);
    ::printf("\r\n");
    ++m_lineCounter;
}

void
S9sDisplay::printNewLine()
{
    ::printf("%s", TERM_ERASE_EOL);
    ::printf("\n\r");
    ::printf("%s", TERM_NORMAL);
    ++m_lineCounter;
}

/**
 * https://stackoverflow.com/questions/8476332/writing-a-real-interactive-terminal-program-like-vim-htop-in-c-c-witho
 */
void 
S9sDisplay::setConioTerminalMode(
        bool interactive)
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios1);
    memcpy(&new_termios, &orig_termios1, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
    
    if (interactive)
    {
        ::printf("%s", TERM_CURSOR_OFF);
        ::printf("%s", TERM_AUTOWRAP_OFF);
    
        // Switch to the alternate buffer screen
        ::printf("%s", "\e[?47h");

        // Enable mouse tracking
        ::printf("%s", "\e[?9h");
    }
}

int 
S9sDisplay::kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(0, &fds);

    return select(1, &fds, NULL, NULL, &tv);
}



