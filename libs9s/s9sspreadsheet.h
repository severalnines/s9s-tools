/*
 * Severalnines Tools
 * Copyright (C) 2018 Severalnines AB
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

#include "S9sObject"

/**
 * A class that represents a node/host/server. 
 */
class S9sSpreadsheet : public S9sObject
{
    public:
        S9sSpreadsheet();
        S9sSpreadsheet(const S9sVariantMap &properties);

        virtual ~S9sSpreadsheet();

        S9sSpreadsheet &operator=(const S9sVariantMap &rhs);


        void setScreenSize(uint columns, uint rows);
        void print() const;

        S9sString value(
                const uint sheet,
                const uint column,
                const uint row) const;
        
        S9sString contentString(
                const uint sheet,
                const uint column,
                const uint row) const;

        bool isAlignRight(
                const uint sheet,
                const uint column,
                const uint row) const;

        int columnWidth(uint column) const;

        int selectedCellRow() const;
        int selectedCellColumn() const;
        void selectedCellLeft();
        void selectedCellRight();
        void selectedCellUp();
        void selectedCellDown();

    private: 
        const char *headerColorBegin() const;
        const char *headerColorEnd() const;

        S9sVariantMap
            cell(
                    const uint sheet,
                    const uint column,
                    const uint row) const;

        const char *
            cellBegin(
                    const uint sheet,
                    const uint column,
                    const uint row) const;

        const char *
            cellEnd(
                    const uint sheet,
                    const uint column,
                    const uint row) const;
    private:
        mutable S9sVariantList m_cells;
        uint                   m_screenRows;
        uint                   m_screenColumns;
        int                    m_selectedCellRow;
        int                    m_selectedCellColumn;
};

