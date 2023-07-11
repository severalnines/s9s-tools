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

#include "s9swidget.h"

class S9sDisplayList : public S9sWidget
{
    public:
        S9sDisplayList();
        virtual ~S9sDisplayList();

        int firstVisibleIndex() const;
        int lastVisibleIndex() const;

        virtual bool processButton(uint button, uint x, uint y);
        virtual void processKey(int key);

        void setSelectionEnabled(const bool value);
        bool isSelectionEnabled() const;
        int selectionIndex() const;
        void setSelectionIndex(int index);

        void setNumberOfItems(int n);
        int numberOfItems() const;

        void setHeaderHeight(int height);
        void setFooterHeight(int height);
        int listHeight() const;

        void selectionUp(int nSteps = 1);
        void selectionDown(int nSteps = 1);
        bool isSelected(const int index) const;
        bool isIndexVisible(const int index) const;

        void ensureSelectionVisible();

    private:
        int  m_selectionEnabled;
        int  m_selectionIndex;
        int  m_startIndex;
        int  m_numberOfItems;
        int  m_headerHeight;
        int  m_footerHeight;
};
