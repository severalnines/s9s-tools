/* 
 * Copyright (C) 2011-2017 severalnines.com
 */
#pragma once

#include "S9sVariant"
#include "S9sVariantList"
#include <vector>

class S9sGraph
{
    public:
        S9sGraph();
        virtual ~S9sGraph();

        void appendValue(const S9sVariant &value);

        void transformWidth(int width);

    private:
        S9sVariantList  m_rawData;
};
