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

#include "S9sWidget"
#include "S9sButton"

class S9sDisplay;

class S9sDialog : public S9sWidget
{
    public:
        S9sDialog(S9sDisplay *display);
        virtual ~S9sDialog();

        void setTitle(const S9sString &text);
        S9sString title() const;
        void setMessage(const S9sString &text);
        S9sString message() const;

        virtual void processKey(int key);

        bool isOkPressed() const;
        void setIsOkPressed(bool value);

        bool isCancelPressed() const;
        
        virtual S9sString text() const;
        virtual void setText(const S9sString &value);

        virtual void refreshScreen();
        virtual void printLine(int lineIndex);

    protected:
        void printChar(const char *c);

        void printChar(
                const char *c,
                const int   lastColumn);
        
        void printString(
                const S9sString &theString);

        void alignCenter();

    protected:
        S9sDisplay      *m_display;
        S9sButton        m_okButton;
        S9sButton        m_cancelButton;
        S9sString        m_title;
        S9sString        m_message1;
        S9sString        m_message2;
        bool             m_okPressed;
        bool             m_cancelPressed;
    
        /** Transient value shows the position in the line. */
        int             m_nChars;

        const char     *m_normalColor;
};
