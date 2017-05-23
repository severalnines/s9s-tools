/* 
 * Copyright (C) 2011-2017 severalnines.com
 */
#pragma once

#include "S9sVariant"
#include "S9sVariantList"

#include <math.h>
#include <vector>

class S9sGraph
{
    public:
        enum AggregateType 
        {
            Max,
            Min,
            Average
        };

        S9sGraph();
        virtual ~S9sGraph();

        void setAggregateType(S9sGraph::AggregateType type);
        void setColor(const bool useColor);

        virtual void appendValue(S9sVariant value);
        virtual void realize();

        void setTitle(const S9sString &title);

        int nColumns() const;
        int nRows() const;
        S9sString line(const int idx);

        int nValues() const;
        S9sVariant max() const;

        void print() const;

    protected:
        void transform(int newWidth, int newHeight);
        void createLines(int newWidth, int newHeight);

        const char *yLabelFormat() const;
        S9sString yLabel(double baseLine) const;

    private:
        S9sVariant aggregate(const S9sVariantList &data) const;

    private:
        AggregateType   m_aggregateType;
        S9sVariantList  m_rawData;
        S9sVariantList  m_transformed;
        int             m_width, m_height;
        S9sVariantList  m_lines;
        S9sString       m_title;
        bool            m_color;
};

template<typename T>
T roundMultiple( T value, T multiple )
{
    if (multiple == 0) 
        return value;

    return static_cast<T>(
            round(
                static_cast<double>(value) / 
                static_cast<double>(multiple)) * 
            static_cast<double>(multiple));
}
