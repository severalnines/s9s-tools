/* 
 * Copyright (C) 2011-2015 severalnines.com
 */
#pragma once

#include "s9sunion.h"

class S9sVariant
{
    public:
        inline S9sVariant();

    private:
        S9sBasicType    m_type;
};

inline 
S9sVariant::S9sVariant() :
    m_type(Invalid)
{
}

