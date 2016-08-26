/* 
 * Copyright (C) 2011-2015 severalnines.com
 */
#pragma once

#include "S9sAtomicInt"
#include "S9sString"
#include "S9sVariant"

#include <regex.h>

#define S9S_REGMATCH_SIZE 20

/**
 * Private data tags for the S9sRegExp class.
 */
class S9sRegExpPrivate
{
    public:
        S9sRegExpPrivate();
        ~S9sRegExpPrivate();
       
        void setIgnoreCase(bool value);
        void compile(const S9sString &theString);
        bool test(const S9sString &rhs);
        bool matching(const S9sString &rhs);
        S9sVariantList match(const S9sString &rhs);
        void replace(S9sString &theString, S9sString replacement);

        S9sString index(int theIndex) const;
        
        void ref();
        int unRef();

    private:
        S9sAtomicInt    m_referenceCounter;
        bool            m_ignoreCase;
        bool            m_global;
        S9sString       m_stringVersion;
        S9sString       m_lastCheckedString;
        bool            m_compiled;
        regex_t         m_binaryRegExp;
        regmatch_t      m_match[S9S_REGMATCH_SIZE];

        friend class S9sRegExp;
        friend class UtS9sRegExp;
};
