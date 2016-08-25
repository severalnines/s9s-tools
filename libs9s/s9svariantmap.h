/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

#include "S9sVariant"
#include "S9sMap"
#include "S9sVector"
#include "S9sString"

class S9sVariantMap : public S9sMap<S9sString, S9sVariant>
{
    public:
        S9sVariantMap() : S9sMap<S9sString, S9sVariant>() {};
        virtual ~S9sVariantMap() {};

        S9sVector<S9sString> keys() const;
};
