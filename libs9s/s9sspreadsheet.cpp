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
    S9sObject(),
    m_screenRows(-1),
    m_screenColumns(-1),
    m_selectedCellRow(0),
    m_selectedCellColumn(0)
{
    m_properties["class_name"] = "CmonSpreadsheet";
}
 
S9sSpreadsheet::S9sSpreadsheet(
        const S9sVariantMap &properties) :
    S9sObject(properties),
    m_screenRows(25),
    m_screenColumns(80)
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

void
S9sSpreadsheet::setScreenSize(
        uint columns, 
        uint rows)
{
    m_screenRows = rows;
    m_screenColumns = columns;
}

int
S9sSpreadsheet::selectedCellRow() const
{
    return m_selectedCellRow;
}

int 
S9sSpreadsheet::selectedCellColumn() const
{
    return m_selectedCellColumn;
}

void
S9sSpreadsheet::selectedCellLeft()
{
    if (m_selectedCellColumn > 0)
        m_selectedCellColumn--;
}

void
S9sSpreadsheet::selectedCellRight()
{
    m_selectedCellColumn++;
}

void
S9sSpreadsheet::selectedCellUp()
{
    if (m_selectedCellRow > 0)
        m_selectedCellRow--;
}

void
S9sSpreadsheet::selectedCellDown()
{
    m_selectedCellRow++;
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

S9sString
S9sSpreadsheet::contentString(
        const uint sheet,
        const uint column,
        const uint row) const
{
    S9sVariantMap theCell = cell(sheet, column, row);

    return theCell["contentString"].toString();
}

const char *
S9sSpreadsheet::cellBegin(
        const uint sheet,
        const uint column,
        const uint row) const
{
    if ((int)column == m_selectedCellColumn &&
            (int(row) == m_selectedCellRow))
    {
        return headerColorBegin();
    }

    return "";
}

const char *
S9sSpreadsheet::cellEnd(
        const uint sheet,
        const uint column,
        const uint row) const
{
    if ((int)column == m_selectedCellColumn &&
            (int(row) == m_selectedCellRow))
    {
        return headerColorEnd();
    }

    return "";
}

bool
S9sSpreadsheet::isAlignRight(
        const uint sheet,
        const uint column,
        const uint row) const
{
    S9sVariantMap theCell   = cell(sheet, column, row);
    S9sString     valueType = theCell["valuetype"].toString();

    if (valueType == "Double")
        return true;
    else if (valueType == "Int")
        return true;

    return false;
}


void
S9sSpreadsheet::print() const
{
    int thisColumn = 0;

    if (m_screenRows < 2u || m_screenColumns < 5u)
        return;

    /*
     * Printing the header line.
     */
    ::printf("     ");
    ::printf("%s", headerColorBegin());

    thisColumn = 5;
    for (uint col = 0u; col < 16; ++col)
    {
        int       theWidth = columnWidth(col);
        S9sString label;

        if (thisColumn + theWidth > (int) m_screenColumns + 1)
            break;

        label += 'A' + col;
        
        for (uint n = 0; n < (theWidth - label.length()) / 2; ++n)
            ::printf(" ");

        ::printf("%s", STR(label));
        
        for (uint n = 0; n < (theWidth - label.length()) / 2; ++n)
            ::printf(" ");

        thisColumn += theWidth;
    }

    for (;thisColumn < (int)m_screenColumns;++thisColumn)
        ::printf(" ");

    //::printf("%s", TERM_ERASE_EOL);
    ::printf("%s", headerColorEnd());
    ::printf("\r\n");

    /*
     *
     */
    for (uint row = 0u; row < m_screenRows - 1; ++row)
    {
        ::printf("%s", headerColorBegin());
        ::printf(" %3u ", row);
        ::printf("%s", headerColorEnd());

        for (uint col = 0u; col < 8; ++col)
        {
            int       theWidth = columnWidth(col);
            S9sString theValue = value(0, col, row);

            if ((int)theValue.length() > theWidth)
                theValue.resize(theWidth);

            // 
            ::printf("%s", cellBegin(0, col, row));

            //
            // Printing the cell content.
            //
            if (!isAlignRight(0, col, row))
            {
                ::printf("%s", STR(theValue));
                if (theWidth > (int)theValue.length())
                {
                    for (uint n = 0; n < theWidth - theValue.length(); ++n)
                        ::printf(" ");
                }
            } else {
                if (theWidth > (int)theValue.length())
                {
                    for (uint n = 0; n < theWidth - theValue.length(); ++n)
                        ::printf(" ");
                }
                ::printf("%s", STR(theValue));
            }
            
            // 
            ::printf("%s", cellEnd(0, col, row));
        }
        
        ::printf("\r\n");
    }
}

int
S9sSpreadsheet::columnWidth(
        uint column) const
{
    return 14;
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
