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
#include "s9sdisplaylist.h"

S9sDisplayList::S9sDisplayList() :
    m_selectionEnabled(false),
    m_selectionIndex(0),
    m_startIndex(0),
    m_numberOfItems(0),
    m_x(0),
    m_y(0),
    m_width(0),
    m_height(0)
{
}

S9sDisplayList::~S9sDisplayList()
{
}

void
S9sDisplayList::selectionUp()
{
    if (m_selectionEnabled)
        m_selectionIndex--;
    else
        m_startIndex--;
}

void 
S9sDisplayList::selectionDown()
{
    if (m_selectionEnabled)
        m_selectionIndex++;
    else 
        m_startIndex++;
}

bool
S9sDisplayList::isSelected(
        const int index) const
{
    return m_selectionEnabled && index == m_selectionIndex;
}

bool
S9sDisplayList::isVisible(
        const int index) const
{
    if (index < m_startIndex)
        return false;

    if (index - m_startIndex > m_height)
        return false;

    return true;
}

void
S9sDisplayList::setLocation(
        int x, 
        int y)
{
    m_x = x;
    m_y = y;
}

void
S9sDisplayList::setSize(
        int nColumns,
        int nRows)
{
    m_width  = nColumns;
    m_height = nRows;
}

bool
S9sDisplayList::contains(
        int x,
        int y) const
{
    return 
        x >= m_x && x < m_x + m_width &&
        y >= m_y && y < m_y + m_height;
}

void
S9sDisplayList::setNumberOfItems(
        int n)
{
    m_numberOfItems = n;

    if (m_selectionIndex >= m_numberOfItems)
        m_selectionIndex = m_numberOfItems - 1;
}

void
S9sDisplayList::setSelectionEnabled(
        const bool value)
{
    m_selectionEnabled = value;
}

bool
S9sDisplayList::isSelectionEnabled() const
{
    return m_selectionEnabled;
}

int 
S9sDisplayList::selectionIndex() const
{
    return m_selectionIndex;
}

// startindex + height = selectionIndex
void
S9sDisplayList::ensureSelectionVisible()
{
    if (m_selectionEnabled)
    {
        if (m_selectionIndex < 0)
            m_selectionIndex = 0;

        if (m_selectionIndex >= m_numberOfItems)
            m_numberOfItems = m_numberOfItems - 1;

        if (m_selectionIndex - m_startIndex + 1 > m_height)
        {
            // The selection is below the last visible line, need to scroll.
            m_startIndex = m_selectionIndex - m_height + 1;
        } else if (m_startIndex > m_selectionIndex)
        {
            m_startIndex = m_selectionIndex;
        }
    } else {
        if (m_startIndex + m_height > m_numberOfItems)
            m_startIndex = m_numberOfItems - m_height;
        
        if (m_startIndex < 0)
            m_startIndex = 0;
    }
}
