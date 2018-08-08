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

#include "S9sString"
#include "S9sFormatter"
#include "S9sVariantMap"
#include "S9sEvent"
#include "S9sNode"
#include "S9sServer"
#include "S9sCluster"
#include "S9sJob"
#include "S9sMutex"
#include "S9sThread"
#include "S9sFile"

#define S9S_KEY_DOWN 0x425b1b
#define S9S_KEY_UP   0x415b1b

/**
 * A UI screen that can be used as a parent class for views continuously
 * refreshed on the terminal screen.
 */
class S9sDisplay : public S9sThread
{
    public:
        S9sDisplay();
        virtual ~S9sDisplay();

        int lastKeyCode() const;

        int columns() const;
        int rows() const;

    protected:
        virtual int exec();
        
    protected:
        virtual void processKey(int key) = 0;
        virtual void processButton(uint button, uint x, uint y);
        virtual void refreshScreen() = 0;

        void startScreen();
        
        virtual void printHeader() = 0;
        virtual void printFooter() = 0;

        void printMiddle(const S9sString text);
        void printNewLine();
        
        char rotatingCharacter() const;

    private:
        void setConioTerminalMode();
        int kbhit();

    protected:
        S9sMutex                     m_mutex;
        S9sFormatter                 m_formatter;
        int                          m_refreshCounter;
        
        union {
            unsigned char  inputBuffer[6];
            int   lastKeyCode;
        } m_lastKeyCode;

        int                          m_columns;
        int                          m_rows;
        int                          m_lineCounter;
        S9sFile                      m_outputFile;
        S9sString                    m_outputFileName;
        int                          m_lastButton;
        int                          m_lastX;
        int                          m_lastY;
};
