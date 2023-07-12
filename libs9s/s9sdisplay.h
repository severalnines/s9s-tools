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
#pragma once

#include "s9sstring.h"
#include "s9svariantmap.h"
#include "s9sevent.h"
#include "s9snode.h"
#include "s9sserver.h"
#include "s9scluster.h"
#include "s9sjob.h"
#include "s9smutex.h"
#include "s9sthread.h"
#include "s9swidget.h"
#include "s9sfile.h"

#define S9S_KEY_DOWN      0x425b1b
#define S9S_KEY_UP        0x415b1b
#define S9S_KEY_RIGHT     0x435b1b
#define S9S_KEY_LEFT      0x445b1b
#define S9S_KEY_PGUP      0x7e355b1b
#define S9S_KEY_PGDN      0x7e365b1b
#define S9S_KEY_ENTER     0x0d
#define S9S_KEY_BACKSPACE 0x7f
#define S9S_KEY_DELETE    0x7e335b1b
#define S9S_KEY_HOME      0x00485b1b
#define S9S_KEY_END       0x00465b1b

#define S9S_KEY_F2        0x00514f1b
#define S9S_KEY_F3        0x00524f1b
#define S9S_KEY_F4        0x00534f1b
#define S9S_KEY_F6        0x37315b1b
#define S9S_KEY_F7        0x38315b1b
#define S9S_KEY_F8        0x39315b1b

#define S9S_KEY_SHIFT_F4  0x3b315b1b

#define S9S_KEY_F10       0x31325b1b
#define S9S_KEY_ESC       0x1b

/**
 * A UI screen that can be used as a parent class for views continuously
 * refreshed on the terminal screen.
 */
class S9sDisplay : 
    public S9sThread,
    public S9sWidget
{
    public:
        S9sDisplay(bool interactive = true, bool rawTerminal = true);
        virtual ~S9sDisplay();
       
        virtual void main();
        virtual void processKey(int key);
        virtual bool processButton(uint button, uint x, uint y);

        bool setOutputFileName(const S9sString &fileName);
        bool setInputFileName(const S9sString &fileName);
        bool hasInputFile() const;

        int lastKeyCode() const;

        static void gotoXy(int x, int y);

    protected:
        virtual int exec();
                
        virtual bool refreshScreen();
        virtual void printHeader();
        virtual void printFooter();

        void startScreen();
        
        void printMiddle(const S9sString text);
        void printNewLine();
        
        char rotatingCharacter() const;

    protected:
        void setConioTerminalMode(
                bool interactive,
                bool rawTerminal);

        bool kbhit();

    protected:
        bool                         m_rawTerminal;
        bool                         m_interactive;
        S9sMutex                     m_mutex;
        int                          m_refreshCounter;
        
        /** Contains information about the last keyboard/mouse event. */
        union {
            unsigned char  inputBuffer[6];
            int            lastKeyCode;
        } m_lastKeyCode;

        /** Shows which line we are in, counting printing newlines. */
        int                          m_lineCounter;
        S9sFile                      m_outputFile;
        S9sString                    m_outputFileName;
        S9sFile                      m_inputFile;
        S9sString                    m_inputFileName;
        int                          m_lastButton;
        int                          m_lastX;
        int                          m_lastY;
        bool                         m_isStopped;
};

void reset_terminal_mode();
