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

        void transform(int newWidth, int newHeight);

    private:
        S9sVariantList  m_rawData;
};
