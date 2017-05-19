/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#include "s9svariantlist.h"

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
    
    for (uint idx = 0u; idx < size(); ++idx)
    {
        if (idx == 0u || biggest < operator[](idx))
            biggest = operator[](idx);
    }

    return biggest;
}
