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
#include "s9swidget.h"

S9sWidget::S9sWidget() :
    m_x(0),
    m_y(0),
    m_width(0),
    m_height(0),
    m_isActive(true)
{
}
 
S9sWidget::~S9sWidget()
{
}


void
S9sWidget::setLocation(
        int x, 
        int y)
{
    m_x = x;
    m_y = y;
}

void
S9sWidget::setSize(
        int nColumns,
        int nRows)
{
    m_width  = nColumns;
    m_height = nRows;
}

bool
S9sWidget::contains(
        int x,
        int y) const
{
    return 
        x >= m_x && x < m_x + m_width &&
        y >= m_y && y < m_y + m_height;
}

int
S9sWidget::y() const
{
    return m_y;
}

int
S9sWidget::x() const
{
    return m_x;
}

int
S9sWidget::height() const
{
    return m_height;
}

int
S9sWidget::width() const
{
    return m_width;
}


/**
 * \returns True if the widget accepts key press events.
 */
bool
S9sWidget::isActive() const
{
    return m_isActive;
}

void
S9sWidget::setActive(
        bool active)
{
    m_isActive = active;
}
