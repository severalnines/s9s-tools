/* 
 * Copyright (C) 2011-2017 severalnines.com
 */
#include "s9svariantarray.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

const S9sVariant S9sVariantArray::sm_emptyValue;

S9sVariantArray::S9sVariantArray()
{
}

S9sVariantArray::~S9sVariantArray()
{
}

/**
 * \returns how many columns are in the array.
 */
uint 
S9sVariantArray::columns() const
{
    return m_columns.size();
}

/**
 * \returns how many rows are in the array.
 */
uint 
S9sVariantArray::rows() const
{
    if (m_columns.empty())
        return 0;

    return m_columns[0].size();
}

/**
 * \returns the const reference to the value stored at the specified location.
 *
 * If the array doesn't hold a value at the specified location this function
 * will resize the array so that it covers the given location.
 */
S9sVariant &
S9sVariantArray::at(
        const unsigned int col,
        const unsigned int row)
{
    if (m_columns.size() <= col)
        m_columns.resize(col + 1);

    // The column 0 has always the maximum size, so we know how big the array
    // is.
    if (m_columns[0].size() <= row)
        m_columns[0].resize(row + 1);

    if (m_columns[col].size() <= row)
        m_columns[col].resize(row + 1);

    return m_columns[col][row];
}

/**
 * \returns the const reference to the value stored at the specified location.
 *
 * If the array doesn't hold a value at the specified location this function
 * will return a reference to an invalid value.
 */
const S9sVariant &
S9sVariantArray::at(
        const unsigned int col,
        const unsigned int row) const
{
    if (m_columns.size() <= col)
        return sm_emptyValue;

    if (m_columns.at(col).size() <= row)
        return sm_emptyValue;

    return m_columns.at(col).at(row);
}

/**
 * Drops all values from the array.
 */
void
S9sVariantArray::clear()
{
    //m_columns.resize(0);
    m_columns = std::vector<t_row>();
}
