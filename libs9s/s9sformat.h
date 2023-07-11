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

#include "s9sstring.h"

/**
 * A helper class to produce uniform but variable width column tables on the
 * terminal and/or standard output.
 */
class S9sFormat 
{
    public:
        enum Alignment 
        {
            AlignLeft,
            AlignRight,
            AlignCenter,
        };

        enum Unit
        {
            /** No unit is set. */
            UnitUnknown,
            /** The number represents milliseconds. */
            UnitMs,
            /** The number represents bytes. */
            UnitBytes,
        };

        S9sFormat();
        S9sFormat(const char *colorStart, const char *colorEnd);
       
        S9sFormat operator+(const S9sFormat &rhs);
        
        void setColor(const char *colorStart, const char *colorEnd);
        void setRightJustify();
        void setCenterJustify();
        void setUnit(const S9sFormat::Unit unit);
        void setHumanReadable(const bool value);

        int realWidth() const;

        void setWidth(int width);
        void setEllipsize(bool ellipsize = true);

        S9sString toString(const double value) const;

        void widen(const S9sString &value);
        void widen(const int value);
        void widen(const ulonglong value);
        void widen(const double value);

        void printf(const int value) const;
        void printf(const ulonglong value) const;
        void printf(const S9sString &value, bool color = true) const;
        void printf(const double value, bool color = true) const;
        void printHeader(const S9sString &value);


        static S9sString toSizeString(const ulonglong value);

    private:
        Unit        m_unit;
        bool        m_humanreadable;
        int         m_width;
        bool        m_withFieldSeparator;
        const char *m_colorStart;
        const char *m_colorEnd;
        Alignment   m_alignment;
        bool        m_ellipsize;
};
