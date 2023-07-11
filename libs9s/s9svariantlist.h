/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

#include <s9svariant.h>
#include <s9svector.h>

class S9sVariantList : public S9sVector<S9sVariant>
{
    public:
        S9sVariant sum() const;
        S9sVariant average() const;
        S9sVariant max() const;
        S9sVariant min() const;

        S9sString toJsonString(
                int                   depth,
                const S9sFormatFlags &formatFlags) const;
};
