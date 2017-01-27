/*
 * Severalnines Tools
 * Copyright (C) 2016  Severalnines AB
 *
 * This file is part of s9s-tools.
 *
 * s9s-tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * S9sTools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with S9sTools. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "S9sString"

/**
 * A helper class to produce uniform but variable width column tables on the
 * terminal and/or standard output.
 */
class S9sFormat 
{
    public:
        S9sFormat();
        
        void setColor(const char *colorStart, const char *colorEnd);
        void setRightJustify(const bool value);

        int realWidth() const;

        void widen(const S9sString &value);
        void widen(const int value);
        void widen(const ulonglong value);

        void printf(const int value) const;
        void printf(const ulonglong value) const;
        void printf(const S9sString &value) const;

        static S9sString toSizeString(const ulonglong value);

    private:
        int         m_width;
        bool        m_withFieldSeparator;
        const char *m_colorStart;
        const char *m_colorEnd;
        bool        m_rightJustify;
};
