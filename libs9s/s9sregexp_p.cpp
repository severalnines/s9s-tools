/* 
 * Copyright (C) 2011-2015 severalnines.com
 */
#include "s9sregexp_p.h"

#include "s9svariantlist.h"
#include "s9svariantmap.h"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sRegExpPrivate::S9sRegExpPrivate() :
    m_referenceCounter(1),
    m_ignoreCase(false),
    m_global(false),
    m_compiled(false)
{
}

S9sRegExpPrivate::~S9sRegExpPrivate()
{
    if (m_compiled)
        ::regfree(&m_binaryRegExp);
}

void
S9sRegExpPrivate::setIgnoreCase(
        bool value)
{
    m_lastCheckedString = "";
    m_match[0].rm_so    = -1;
    m_match[0].rm_eo    = -1;

    m_ignoreCase = value;
    
    if (!m_stringVersion.empty())
        compile(m_stringVersion);
}

void 
S9sRegExpPrivate::compile(
        const S9sString &theString)
{
    S9sString  myExp;
    int        cFlags;

    m_lastCheckedString = "";
    m_stringVersion     = theString;
    m_match[0].rm_so    = -1;
    m_match[0].rm_eo    = -1;

    cFlags = REG_EXTENDED;
    if (m_ignoreCase)
        cFlags |= REG_ICASE;

    // Just to be compatible with JavaScript
    myExp = theString;
    myExp.replace("\\d", "[[:digit:]]");
    //maybe this one?
    //myExp.replace("\\s", "[[:space:]]");

    if (m_compiled)
        ::regfree(&m_binaryRegExp);

    if (::regcomp(&m_binaryRegExp, STR(myExp), cFlags) != 0) 
    {
        S9S_WARNING("ERROR in regular expression.");
        ::regcomp(&m_binaryRegExp, "", cFlags);
    }

    m_compiled = true;
}

bool
S9sRegExpPrivate::test(
        const S9sString &rhs)
{
    int nMatch;

    if (m_global && m_lastCheckedString == rhs)
    {
        // If this a global pattern, this is the same string and we have a 
        // previous match we do continue finding the matches in the same string
        // from the previous match. This is quite sophisticated stuff here.
        if (m_match[0].rm_eo != -1)
        {
            int relIndex = m_match[0].rm_eo;

            nMatch = ::regexec(
                    &m_binaryRegExp, STR(rhs) + relIndex, 
                    S9S_REGMATCH_SIZE, m_match, 0); 

            if (nMatch == REG_NOMATCH)
            {
                m_match[0].rm_so = -1;
                m_match[0].rm_eo = -1;
            }

            for (int idx = 0; idx < S9S_REGMATCH_SIZE; ++idx)
            {
                if (m_match[idx].rm_so == -1 ||
                    m_match[idx].rm_eo == -1)
                    break;

                m_match[idx].rm_so += relIndex;
                m_match[idx].rm_eo += relIndex;
            }

            return nMatch != REG_NOMATCH;
        } else {
            return false;
        }
    } 

    m_lastCheckedString = rhs;
    nMatch = ::regexec(
            &m_binaryRegExp, STR(rhs), 
            S9S_REGMATCH_SIZE, m_match, 0); 
            
    if (nMatch == REG_NOMATCH)
    {
        m_match[0].rm_so = -1;
        m_match[0].rm_eo = -1;
    }

    return nMatch != REG_NOMATCH;
}


bool
S9sRegExpPrivate::matching(
        const S9sString &rhs)
{
    int nMatch;

    m_lastCheckedString = rhs;
    nMatch = ::regexec(
            &m_binaryRegExp, STR(rhs), 
            S9S_REGMATCH_SIZE, m_match, 0); 
    
    if (nMatch == REG_NOMATCH)
    {
        m_match[0].rm_so = -1;
        m_match[0].rm_eo = -1;
    }

    return nMatch != REG_NOMATCH;
}

S9sVariantList
S9sRegExpPrivate::match(
        const S9sString &rhs)
{
    S9sVariantList retval;

    matching(rhs);
        
    for (int idx = 0; idx < S9S_REGMATCH_SIZE; ++idx)
    {
        if (m_match[idx].rm_so == -1 ||
            m_match[idx].rm_eo == -1)
            break;

        retval << m_lastCheckedString.substr(
                m_match[idx].rm_so,
                m_match[idx].rm_eo - m_match[idx].rm_so);
    }

    return retval;
}

void
S9sRegExpPrivate::replace(
        S9sString &theString,
        S9sString  replacement)
{
    if (!matching(theString))
        return;

    for (int idx = 1; idx < S9S_REGMATCH_SIZE; ++idx)
    {
        S9sString name;

        name.sprintf("$%d", idx);
        replacement.replace(name, index(idx));
    }

    theString.replace(
            (size_t) m_match[0].rm_so,
            (size_t) m_match[0].rm_eo - m_match[0].rm_so,
            replacement);
}

S9sString
S9sRegExpPrivate::index(
        int theIndex) const
{
    S9sString retval;

    for (int idx = 0; idx < S9S_REGMATCH_SIZE; ++idx)
    {
        if (m_match[idx].rm_so == -1 ||
            m_match[idx].rm_eo == -1)
            break;

        if (idx != theIndex)
            continue;

        retval = m_lastCheckedString.substr(
                m_match[idx].rm_so,
                m_match[idx].rm_eo - m_match[idx].rm_so);

        break;
    }

    return retval;
}

void 
S9sRegExpPrivate::ref()
{
	++m_referenceCounter;
}

int 
S9sRegExpPrivate::unRef()
{
	return --m_referenceCounter;
}
