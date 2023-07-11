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
#include "s9sdisplay.h"

S9sDisplayList::S9sDisplayList() :
    S9sWidget(), 
    m_selectionEnabled(true),
    m_selectionIndex(0),
    m_startIndex(0),
    m_numberOfItems(0)
{
}

S9sDisplayList::~S9sDisplayList()
{
}

/**
 * \param nSteps Shows how many steps the selection will be moved upwards.
 */
void
S9sDisplayList::selectionUp(
        int nSteps)
{
    if (m_selectionEnabled)
    {
        m_selectionIndex -= nSteps;

        if (m_selectionIndex < 0)
            m_selectionIndex = 0;
    } else {
        m_startIndex -= nSteps;
    }
}

/**
 * param nSteps Shows how many steps the selection will be moved downwards.
 */
void 
S9sDisplayList::selectionDown(
        int nSteps)
{
    if (m_selectionEnabled)
    {
        m_selectionIndex += nSteps;

        if (m_selectionIndex >= m_numberOfItems)
            m_selectionIndex = m_numberOfItems - 1;
    } else {
        m_startIndex += nSteps;
    }
}

bool
S9sDisplayList::isSelected(
        const int index) const
{
    return m_selectionEnabled && index == m_selectionIndex;
}

bool
S9sDisplayList::isIndexVisible(
        const int index) const
{
    if (index < m_startIndex)
        return false;

    if (index - m_startIndex > listHeight())
        return false;

    return true;
}

int 
S9sDisplayList::firstVisibleIndex() const
{
    return m_startIndex;
}

int
S9sDisplayList::lastVisibleIndex() const
{
    int retval = m_startIndex + listHeight() - 1;
#if 0
    if (retval >= m_numberOfItems)
        retval = m_numberOfItems - 1;
#endif
    return retval;
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
    else if (!hasFocus() && contains(x, y))
    {
        if (button == 0 || button == 1 || button == 2)
        {
            setHasFocus(true);
            return true;
        }

        return false;
    }

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

        case S9S_KEY_PGDN:
            selectionDown(listHeight());
            break;

        case S9S_KEY_PGUP:
            selectionUp(listHeight());
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

int
S9sDisplayList::numberOfItems() const
{
    return m_numberOfItems;
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

void
S9sDisplayList::setSelectionIndex(
        int index)
{
    m_selectionIndex = index;
}

void
S9sDisplayList::setHeaderHeight(
        int height)
{
    m_headerHeight = height;
}

void
S9sDisplayList::setFooterHeight(
        int height)
{
    m_footerHeight = height;
}

int
S9sDisplayList::listHeight() const
{
    return height() - m_headerHeight - m_footerHeight;
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
            m_selectionIndex = m_numberOfItems - 1;

        if (m_selectionIndex - m_startIndex + 1 > listHeight())
        {
            // The selection is below the last visible line, need to scroll.
            m_startIndex = m_selectionIndex - listHeight() + 1;
        } else if (m_startIndex > m_selectionIndex && m_selectionIndex >= 0)
        {
            // The selection is above the first visible line, need to scroll.
            m_startIndex = m_selectionIndex;
        }
    } else {
        if (m_startIndex + listHeight() > m_numberOfItems)
            m_startIndex = m_numberOfItems - listHeight();
        
        if (m_startIndex < 0)
            m_startIndex = 0;
    }
}
