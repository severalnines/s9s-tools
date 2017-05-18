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

    private:
        typedef std::vector<S9sVariant> t_row;
        std::vector<t_row>               m_columns; 
};
