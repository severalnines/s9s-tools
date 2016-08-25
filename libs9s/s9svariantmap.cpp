/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#include "S9sVariantMap"

S9sVector<S9sString> 
S9sVariantMap::keys() const
{
    S9sVector<S9sString> retval;

    for (std::map<S9sString,S9sVariant>::const_iterator it = this->begin(); it != this->end(); ++it) 
    {
        retval.push_back(it->first);
    }

    return retval;
}
