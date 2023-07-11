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
#include "s9sbutton.h"

#include "s9sdisplay.h"

#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sButton::S9sButton() :
    S9sWidget()
{
    setHeight(1);
}

S9sButton::S9sButton(
        const S9sString &labelText) :
    m_labelText(labelText)
{
    setHeight(1);
    setWidth(labelText.length() + 2);
}

S9sButton::~S9sButton()
{
}

        
void 
S9sButton::print() const
{
    ::printf("[%s]", STR(m_labelText));
}

