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
#include "S9sDisplay"

S9sDisplayList::S9sDisplayList() :
    m_isVisible(false),
    m_selectionEnabled(true),
    m_selectionIndex(0),
    m_startIndex(0),
    m_numberOfItems(0)
{
}

S9sDisplayList::~S9sDisplayList()
{
}

void
S9sDisplayList::selectionUp(
        int nSteps)
{
    if (m_selectionEnabled)
        m_selectionIndex -= nSteps;
    else
        m_startIndex -= nSteps;
}

void 
S9sDisplayList::selectionDown(
        int nSteps)
{
    if (m_selectionEnabled)
        m_selectionIndex += nSteps;
    else 
        m_startIndex += nSteps;
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
S9sDisplayList::setVisible(
        bool value)
{
    m_isVisible = value;
}

int 
S9sDisplayList::firstVisibleIndex() const
{
    return m_startIndex;
}

int
S9sDisplayList::lastVisibleIndex() const
{
    int retval = m_startIndex + m_height - 1;
#if 0
    if (retval >= m_numberOfItems)
        retval = m_numberOfItems - 1;
#endif
    return retval;
}

bool
S9sDisplayList::isVisible() const
{
    return m_isVisible;
}

/**
 * \returns True if the mouse event is processed and should not considered by
 *   other widgets.
 */
bool
S9sDisplayList::processButton(
        uint button, 
        uint x, 
        uint y)
{
    if (!isVisible())
        return false;
    else if (!isActive())
        return false;

    if (!contains(x, y))
        return false;

    switch (button)
    {
        case 64:
            selectionUp();
            break;

        case 65:
            selectionDown();
            break;
    }

    return true;
}

void
S9sDisplayList::processKey(
        int key)
{
    if (!isVisible())
        return;

    switch (key)
    {
        case S9S_KEY_DOWN:
            selectionDown();
            break;

        case S9S_KEY_UP:
            selectionUp();
            break;

        case S9S_KEY_PGUP:
            selectionUp(m_height);
            break;

        case S9S_KEY_PGDN:
            selectionDown(m_height);
            break;

        default:
            ::printf(" %x ", key);
            //sleep(5);
    }
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
            // The selection is above the first visible line, need to scroll.
            m_startIndex = m_selectionIndex;
        }
    } else {
        if (m_startIndex + m_height > m_numberOfItems)
            m_startIndex = m_numberOfItems - m_height;
        
        if (m_startIndex < 0)
            m_startIndex = 0;
    }
}
