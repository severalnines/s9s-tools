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
#include "s9squestiondialog.h"

#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sQuestionDialog::S9sQuestionDialog(
        S9sDisplay *display) :
    S9sDialog(display)
{
    m_normalColor     = TERM_NORMAL "\033[48;5;160m";
}

S9sQuestionDialog::~S9sQuestionDialog()
{
}

