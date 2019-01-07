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
#include "S9sRpcReply"

class S9sEditor :
    public S9sWidget
{
    public:
        S9sEditor();
        virtual ~S9sEditor();

        S9sString path() const;
        S9sString content() const;

        void setIsReadOnly(bool value);
        bool isReadonly() const;

        int numberOfLines() const;

        bool isSaveRequested() const;
        void setSaveRequested(bool value);

        virtual void processKey(int key);

        void printLine(int lineIndex);

        void setObject(
                const S9sString     &path,
                const S9sVariantMap &object);

        S9sString objectPath() const;
        time_t objectSetTime() const;

        void showCursor();

    private:
        void printString(const S9sString &theString);
        void printChar(int c);
        void printChar(const char *c);
        void printChar(const char *c, const int lastColumn);

        S9sString lineAt(int index);

    private:
        bool             m_readOnly;
        bool             m_saveRequested;

        /** Transient value shows the position in the line. */
        int              m_nChars;

        /** Information about the object we show in preview. */
        S9sString        m_objectPath;
        S9sVariantMap    m_object;
        time_t           m_objectSetTime;

        S9sVariantList   m_lines;
        int              m_lineOffset;
        int              m_cursorX;
        int              m_cursorY;
};
