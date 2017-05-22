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

        int nValues() { return (int) m_rawData.size(); };
        double max() { return m_rawData.max().toDouble(); };

        void realize();

    protected:
        void transform(int newWidth, int newHeight);
        void print(int newWidth, int newHeight);

        const char *yLabelFormat() const;

    private:
        S9sVariantList  m_rawData;
        S9sVariantList  m_transformed;
        int             m_width, m_height;
};
