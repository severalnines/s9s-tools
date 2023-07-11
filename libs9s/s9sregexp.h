/* 
 * Copyright (C) 2016 severalnines.com
 */
#pragma once

#include "s9sstring.h"
#include "s9svariantmap.h"

class S9sRegExpPrivate;
class S9sVariantList;
class S9sVariant;

/**
 * A high level implicitly shared class to handle regular expressions. This
 * class is using the compile/execute schema, so it is very efficient when used
 * to match multiple strings. It is also high level, implements a number of
 * operators to write a readable code.
 */
class S9sRegExp 
{
    public:
        S9sRegExp();
        S9sRegExp(const S9sRegExp &orig);
        S9sRegExp(const S9sString &regexp);
        S9sRegExp(const S9sString &regexp, const S9sString &modifiers);

        virtual ~S9sRegExp();

        S9sRegExp &operator=(const S9sRegExp &rhs);
        S9sRegExp &operator=(const S9sString &rhs);
        bool operator==(const S9sString &rhs);
        bool operator!=(const S9sString &rhs);
        S9sString operator[](int index) const;

        bool test(const S9sString &theString);
        S9sVariantList match(const S9sString &rhs);
        S9sVariantMap exec(const S9sString &rhs);

        int lastIndex() const;
        int firstIndex() const;
        void replace(S9sString &theString, S9sString replacement);

        void setIgnoreCase(bool value);
        bool ignoreCase() const;

        bool global() const;

        S9sVariant source() const;
        void setSource(const S9sVariant &value);

        virtual S9sString toString() const;

    private:
        S9sRegExpPrivate *m_priv;

        friend class UtS9sRegExp;
};


