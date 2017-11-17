/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#include "S9sVariantMap"

#include <cmath>

#include "S9sVariantList"
#include "S9sJsonParseContext"

#define YY_EXTRA_TYPE S9sJsonParseContext *
#include "json_parser.h"
#include "json_lexer.h"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

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

/**
 * \returns true if and only if the string was successfully parsed
 *
 * If the input string has syntax error in it the content of the JSON message
 * will not be changed. If the parsing is successfull, the content of the JSON
 * message will be clared and filled with the new values.
 */
bool
S9sVariantMap::parse(
        const char *source)
{
    S9sJsonParseContext context(source);
    int retval;
    bool success;

    json_lex_init(&context.m_flex_scanner);
    json_set_extra(&context, context.m_flex_scanner);

    retval = json_parse(context);
    success = retval == 0;

    json_lex_destroy(context.m_flex_scanner);

    if (success)
    {
        clear();

        std::vector<S9sString> newKeys = context.keys();
        for (uint idx = 0; idx < newKeys.size(); ++idx)
            (*this)[newKeys[idx]] = context[newKeys[idx]];
    }

    return success;
}

/**
 * Converts the variant map to a JSON string.
 */
S9sString
S9sVariantMap::toString() const
{
    return toString(0, *this);
}

/**
 * Private function, part of the S9sVariantMap::toString() implemtation.
 */
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

enum AssignmentState
{
    Start,
    ReadName,
    ReadOperator,
    ReadValueSpace,
    ReadValue,
    ReadValueQuoted,
    ReadFieldSep
};

/**
 * This method implements a parser that will process strings like 
 * "alias = somealias" or "alias = other; cdt_path = '/'". These seems to
 * be useful to pass more complex command line strings.
 */
bool
S9sVariantMap::parseAssignments(
        const S9sString &input)
{
    AssignmentState state = Start;
    S9sString       name;
    S9sString       value;
    int  n = 0;
    char c;
    bool retval = true;

    clear();

    for (;;)
    {
        c = input[n];
        S9S_DEBUG("[%03d] '%c' in %s", n, c, STR(input));
        S9S_DEBUG("      name: '%s' value: '%s'", STR(name), STR(value));

        switch (state)
        {
            case Start:
                S9S_DEBUG("Start");
                if (c == '\0')
                {
                    return true;
                } else if (c == '\"')
                {
                    // No quoting yet.
                    return false;
                } else if (c == ' ' || c == '\t')
                {
                    ++n;
                } else {
                    state = ReadName;
                }
                break;

            case ReadName:
                S9S_DEBUG("ReadName");
                if (c == '\0')
                {
                    return false;
                } else if (c == ' ' || c == '=' || c == ':') 
                {
                    state = ReadOperator;
                } else {
                    name += c;
                    ++n;
                }
                break;

            case ReadOperator:
                S9S_DEBUG("ReadOperator");
                if (c == '\0')
                {
                    return false;
                } else if (c == '=' || c == ':')
                {
                    state = ReadValueSpace;
                    ++n;
                } else {
                    ++n;
                }
                break;

            case ReadValueSpace:
                S9S_DEBUG("ReadValueSpace");
                if (c == '\0')
                {
                    return false;
                } else if (c == ' ' || c == '\t')
                {
                    ++n;
                } else {
                    state = ReadValue;
                }
                break;

            case ReadValue:
                S9S_DEBUG("ReadValue");
                if (c == '\0')
                {
                    S9S_DEBUG("%s = %s", STR(name), STR(value));
                    this->operator[](name) = value;
                    return true;
                } else if (c == ';' || c == ',')
                {
                    this->operator[](name) = value;
                    name.clear();
                    value.clear();

                    state = Start;
                    ++n;
                } else if (c == '\'')
                {
                    ++n;
                    state = ReadValueQuoted;
                } else {
                    value += c;
                    ++n;
                }
                break;

            case ReadValueQuoted:
                S9S_DEBUG("ReadValueQuoted");
                if (c == '\0')
                {
                    return false;
                } else if (c == '\'')
                {
                    S9S_DEBUG("%s = %s", STR(name), STR(value));
                    this->operator[](name) = value;
                    name.clear();
                    value.clear();
                    ++n;

                    state = ReadFieldSep;
                } else {
                    value += c;
                    ++n;
                }
                break;

            case ReadFieldSep:
                S9S_DEBUG("ReadFieldSep");
                if (c == '\0')
                {
                    return true;
                } else if (c == ' ')
                {
                    ++n;
                } else if (c == ';' || c == ',')
                {
                    ++n;
                    state = Start;
                } else {
                    return false;
                }
                break;

        }
    }

    return retval;
}
    
bool 
S9sVariantMap::isSubSet(
        const S9sVariantMap &superSet) const
{
    S9sVector<S9sString> theKeys = keys();

    for (uint idx = 0u; idx < theKeys.size(); ++idx)
    {
        const S9sString &key = theKeys[idx];

        if (!superSet.contains(key))
            return false;

        if (at(key) == superSet.at(key))
            continue;
            
        return false;
    }

    return true;
}

/**
 * Private function, part of the S9sVariantMap::toString() implemtation.
 */
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

/**
 * Private function, part of the S9sVariantMap::toString() implemtation.
 */
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
                } else if (std::isinf (dblval))
                {
                    retval += m_enableSpecialNums ? "Infinity" : "0.0";
                } else {
                    retval += value.toString();
                }
            break;
        }

        case Map:
        case Node:
        case Account:
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

/**
 * Private function, part of the S9sVariantMap::toString() implemtation.
 */
S9sString
S9sVariantMap::indent(
        int depth) const
{
    S9sString retval;
    
    for (int n = 0; n < depth; ++n)
        retval += "    ";

    return retval;
}

/**
 * Private function, part of the S9sVariantMap::toString() implemtation.
 */
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

