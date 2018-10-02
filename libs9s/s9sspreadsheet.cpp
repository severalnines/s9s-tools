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
#include "s9sspreadsheet.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sSpreadsheet::S9sSpreadsheet() :
    S9sObject()
{
    m_properties["class_name"] = "CmonSpreadsheet";
}
 
S9sSpreadsheet::S9sSpreadsheet(
        const S9sVariantMap &properties) :
    S9sObject(properties)
{
    if (!m_properties.contains("class_name"))
        m_properties["class_name"] = "CmonSpreadsheet";
}

S9sSpreadsheet::~S9sSpreadsheet()
{
}

S9sSpreadsheet &
S9sSpreadsheet::operator=(
        const S9sVariantMap &rhs)
{
    setProperties(rhs);
    
    return *this;
}

S9sString
S9sSpreadsheet::value(
        const uint sheet,
        const uint column,
        const uint row) const
{
    S9sVariantMap theCell = cell(sheet, column, row);

    return theCell["value"].toString();
}

void
S9sSpreadsheet::print() const
{
    ::printf("     ");

    ::printf("%s", headerColorBegin());
    for (uint col = 0u; col < 10; ++col)
    {
        int       theWidth = columnWidth(col);
        //int       printed  = 0;
        S9sString label;

        label += 'A' + col;
        
        for (uint n = 0; n < (theWidth - label.length()) / 2; ++n)
        {
            ::printf(" ");
        }

        ::printf("%s", STR(label));
        
        for (uint n = 0; n < (theWidth - label.length()) / 2; ++n)
        {
            ::printf(" ");
        }
    }
    ::printf("%s", headerColorEnd());
    ::printf("\n");

    for (uint row = 0u; row < 10; ++row)
    {
        ::printf("%s", headerColorBegin());
        ::printf(" %3u ", row);
        ::printf("%s", headerColorEnd());

        for (uint col = 0u; col < 10; ++col)
        {
            int       theWidth = columnWidth(col);
            S9sString theValue = value(0, col, row);

            if ((int)theValue.length() > theWidth)
            {
                theValue.resize(theWidth);
            }

            ::printf("%10s ", STR(theValue));
            if (theWidth > (int)theValue.length())
            {
                for (uint n = 0; n < theWidth - theValue.length(); ++n)
                {
                    ::printf(" ");
                }
            }

        }
        
        ::printf("\n");
    }
}

int
S9sSpreadsheet::columnWidth(
        uint column) const
{
    return 10;
}

S9sVariantMap
S9sSpreadsheet::cell(
        const uint sheet,
        const uint column,
        const uint row) const
{
    S9sVariantMap retval;

    if (m_cells.empty())
        m_cells = property("cells").toVariantList();

    for (uint idx = 0u; idx < m_cells.size(); ++idx)
    {
        S9sVariantMap theCell = m_cells[idx].toVariantMap();

        S9S_DEBUG("idx: %u", idx);
        if (theCell["sheetIndex"].toInt() != (int)sheet)
            continue;
        
        if (theCell["rowIndex"].toInt() != (int)row)
            continue;
        
        if (theCell["columnIndex"].toInt() != (int)column)
            continue;

        retval = theCell;
        break;
    }

    S9S_DEBUG("%u, %u, %u -> %s", 
            sheet, column, row,
            STR(retval.toString()));
    return retval;
}
        
const char *
S9sSpreadsheet::headerColorBegin() const
{
    return TERM_INVERSE;
}

const char *
S9sSpreadsheet::headerColorEnd() const
{
    return TERM_NORMAL;
}
