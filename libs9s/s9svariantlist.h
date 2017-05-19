/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

#include "S9sVariant"
#include "S9sVector"

class S9sVariantList : public S9sVector<S9sVariant>
{
    public:
        S9sVariant average() const;
        S9sVariant max() const;

};
