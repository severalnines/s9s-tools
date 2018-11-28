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
#include "S9sTreeNode"
#include "S9sRpcReply"

class S9sInfoPanel :
    public S9sWidget
{
    public:
        S9sInfoPanel();
        virtual ~S9sInfoPanel();

        void printLine(int lineIndex);

        void setInfoController(
                const S9sString &hostName,
                const int        port,
                const bool       useTls);

        void setInfoRequestName(
                const S9sString  requestName);

        void setInfoLastReply(
                const S9sRpcReply &reply);

        void setInfoNode(
                const S9sTreeNode &node);

        S9sString controllerUrl() const;

    private:
        void printString(const S9sString &theString);

        void printNameValue(
                const S9sString &name,
                const S9sString &value);

        void printChar(int c);
        void printChar(const char *c);
        void printChar(const char *c, const int lastColumn);

    private:
        S9sString    m_hostName;
        int          m_port;
        bool         m_useTls;
        S9sString    m_requestName;
        S9sRpcReply  m_lastReply;
        S9sTreeNode  m_node;
        /** Transient value shows the position in the line. */
        int          m_nChars;
};
