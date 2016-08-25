/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#include "S9sVariantMap"

#include <cmath>

#include <S9sVariantList>

static const char dblQuot = '"';

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

S9sString
S9sVariantMap::toString() const
{
    return toString(0, *this);
}

S9sString
S9sVariantMap::toString(
        int                  depth, 
        const S9sVariantMap &variantMap) const
{
    S9sVector<S9sString> theKeys = variantMap.keys();
    S9sString            retval;

    retval = indent(depth) + "{\n";
    for (uint idx = 0; idx < theKeys.size(); ++idx)
    {
        retval += indent(depth + 1);
        retval += quote(theKeys[idx]);
        retval += ": ";
       
        S9sVariant value = variantMap.at(theKeys[idx]);
        retval += toString(depth, value);

        if (idx + 1 < theKeys.size())
            retval += ',';

        retval += "\n";
    }

    retval += indent(depth) + "}";

    return retval;
}

S9sString
S9sVariantMap::toString (
        int                   depth,
        const S9sVariantList &theList) const
{
    S9sString retval;

    retval += "[ ";
    
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        if (idx > 0)
            retval += ", ";

        retval += toString(depth, theList[idx]);
    }

    retval += " ]";

    return retval;
}

S9sString
S9sVariantMap::toString (
        int               depth,
        const S9sVariant &value) const
{
    S9sString retval;

    switch (value.type())
    {
        case String:
            retval += quote(value.toString());
            break;

        case Int:
        case Bool:
        case Ulonglong:
            retval += value.toString();
            break;

        case Double: 
            {
                // FIXME: maybe this classification shouldn't be here, but here
                // we return 'JSon' compatible strings, so might be this is the
                // correct place
                double dblval = value.toDouble();
                bool   m_enableSpecialNums = true;
                
                if (std::isnan (dblval)) 
                {
                    retval += m_enableSpecialNums ? "NaN" : "0.0";
                } else if (std::isinf (dblval) > 0) 
                {
                    retval += m_enableSpecialNums ? "Infinity" : "0.0";
                } else if (std::isinf (dblval) < 0) 
                {
                    retval += m_enableSpecialNums ? "-Infinity" : "0.0";
                } else {
                    retval += value.toString();
                }
            break;
        }

        case Map:
            retval += "\n";
            retval += toString(depth + 1, value.toVariantMap());
            break;

        case List:
            retval += toString(depth, value.toVariantList());
            break;
        
        default:
            // Let's use 'null' for invalid/null data (http://www.json.org/)
            retval.sprintf ("null");
            break;
    }

    return retval;
}


S9sString
S9sVariantMap::indent(
        int depth) const
{
    S9sString retval;
    
    for (int n = 0; n < depth; ++n)
        retval += "    ";

    return retval;
}

S9sString
S9sVariantMap::quote(
        const S9sString &s) const
{
    S9sString retval;

    retval += dblQuot;

    // lets encode the double quote, and some
    // other special chars in the returned JSon
    for (uint idx = 0; idx < s.length(); ++idx)
    {
        if (s[idx] == dblQuot) 
        {
            retval += '\\';
            retval += dblQuot;
        } else if (s[idx] == '\n') 
        {
            retval += "\\n";
        } else if (s[idx] == '\r') 
        {
            retval += "\\r";
        } else if (s[idx] == '\t') 
        {
            retval += "\\t";
        } else if (s[idx] == '\\') 
        {
            retval += "\\\\";
        } else 
        {
            retval += s[idx];
        }
    }

    retval += dblQuot;
    return retval;
}

