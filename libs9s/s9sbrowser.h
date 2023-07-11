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

#include "s9sdisplaylist.h"
#include "s9streenode.h"

class S9sBrowser :
    public S9sDisplayList
{
    public:
        S9sBrowser();
        virtual ~S9sBrowser();

        virtual void processKey(int key);
        //virtual bool processButton(uint button, uint x, uint y);        

        void printLine(int lineIndex);
        void setCdt(const S9sTreeNode &node);

        S9sString path() const;

        S9sTreeNode selectedNode() const;
        S9sString selectedNodeFullPath() const;

        S9sTreeNode activatedNode() const;
        S9sString activatedNodeFullPath() const;
        void resetActivatedStatus();

        void setSelectionIndexByName(const S9sString &name);

    private:
        void printString(const S9sString &theString);
        void printChar(int c);
        void printChar(const char *c);
        void printChar(const char *c, const int lastColumn);

    private:
        S9sTreeNode                  m_rootNode;

        S9sTreeNode                  m_subTree;
        /** Valid if the enter key is pressed on the current item. */
        S9sString                    m_acivatedPath;
        /** Valid if the enter key is pressed on the current item. */
        S9sTreeNode                  m_activatedNode;
        /** The path shown in the list. */
        S9sString                    m_path;
        /** The name of the selected item. */
        S9sString                    m_name;
        /** In debug mode we show some debug on the screen. */
        bool                         m_isDebug;
        /** Transient value shows the position in the line. */
        int                          m_nChars;
};
