/* 
 * Copyright (C) 2011-2017 severalnines.com
 */
#pragma once

#include "S9sVariant"
#include <vector>

class S9sVariantArray 
{
    public:
        S9sVariantArray();
        virtual ~S9sVariantArray();

        uint columns() const;
        uint rows() const;

        S9sVariant &at(
                const unsigned int col,
                const unsigned int row);

        const S9sVariant &at(
                const unsigned int col,
                const unsigned int row) const;

        void clear();

    private:
        static const S9sVariant sm_emptyValue;

        typedef std::vector<S9sVariant> t_row;
        std::vector<t_row>               m_columns; 
};
