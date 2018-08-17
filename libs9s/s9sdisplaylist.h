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

class S9sDisplayList
{
    public:
        S9sDisplayList();
        virtual ~S9sDisplayList();

        void setVisible(bool value);
        bool isVisible() const;

        bool processButton(uint button, uint x, uint y);
        void processKey(int key);

        void setSelectionEnabled(const bool value);
        bool isSelectionEnabled() const;
        int selectionIndex() const;

        void setLocation(int x, int y);
        void setSize(int nColumns, int nRows);
        bool contains(int x, int y) const;
        void setNumberOfItems(int n);

        void selectionUp(int nSteps = 1);
        void selectionDown(int nSteps = 1);
        bool isSelected(const int index) const;
        bool isVisible(const int index) const;

        void ensureSelectionVisible();

    private:
        bool m_isVisible;
        int  m_selectionEnabled;
        int  m_selectionIndex;
        int  m_startIndex;
        int  m_numberOfItems;
        
        int  m_x;
        int  m_y;
        int  m_width;
        int  m_height;
};
