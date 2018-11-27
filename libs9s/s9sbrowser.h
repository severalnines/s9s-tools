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

#include "S9sDisplayList"
#include "S9sTreeNode"

class S9sBrowser :
    public S9sDisplayList
{
    public:
        S9sBrowser();
        virtual ~S9sBrowser();

        virtual void processKey(int key);

        void printLine(int lineIndex);
        void setCdt(const S9sTreeNode &node);
        S9sTreeNode selectedNode() const;

    private:
        S9sTreeNode                  m_rootNode;
        S9sTreeNode                  m_subTree;
        S9sString                    m_path;
        bool                         m_isDebug;
};
