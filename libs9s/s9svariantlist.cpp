/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#include "s9svariantlist.h"

#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sVariant 
S9sVariantList::average() const
{
    S9sVariant sum;

    if (size() == 0u)
        return sum;

    for (uint idx = 0u; idx < size(); ++idx)
        sum += operator[](idx);

    sum /= (int) size();

    return sum;
}

S9sVariant
S9sVariantList::max() const
{
    S9sVariant biggest;
    
    #if 0
    S9S_WARNING(" size(): %u", size());
    for (uint idx = 0u; idx < size(); ++idx)
    {
        S9S_WARNING("%03u %s(%s)", 
                idx,
                STR(operator[](idx).toString()),
                STR(operator[](idx).typeName()));
    }
    #endif

    for (uint idx = 0u; idx < size(); ++idx)
    {
        if (idx == 0u || operator[](idx) > biggest)
            biggest = operator[](idx);
    }

    return biggest;
}
