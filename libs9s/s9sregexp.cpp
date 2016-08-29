/* 
 * Copyright (C) 2016 severalnines.com
 */
#include "s9sregexp.h"
#include "s9sregexp_p.h"

#include "S9sVariant"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sRegExp::S9sRegExp() :
    m_priv(new S9sRegExpPrivate)
{
}

/**
 * \param regexp The regular expression string to immediately compile.
 * \param modifiers The modifier that might change the meaning of the regular
 *   expression
 *
 * This is a JavaScript like constructor for the class. It accepts the regular
 * expression string and a modifier, which is also a string interpreted the same
 * way it is interpreted in the JavaScript language.
 *
 * If the modifier contains the letter 'i' the compiled regular expression will
 * ignore the case of the letters while matching a pattern.
 */
S9sRegExp::S9sRegExp(
        const S9sString &regexp,
        const S9sString &modifiers) :
    m_priv(new S9sRegExpPrivate)
{
    if (modifiers.contains('i'))
        m_priv->m_ignoreCase = true;

    if (modifiers.contains('g'))
        m_priv->m_global = true;

    m_priv->compile(regexp);
}

S9sRegExp::S9sRegExp(
        const S9sString &regexp) :
    m_priv(new S9sRegExpPrivate)
{
    if (regexp.empty() || regexp[0] != '/')
    {
        m_priv->compile(regexp);
        return;
    }

    size_t lastIndex = regexp.find_last_of('/');
    if (lastIndex <= 0)
    {
        m_priv->compile(regexp);
        return;
    }

    S9sString  expression = regexp.substr(1, lastIndex - 1);
    S9sString  modifiers  = regexp.substr(
            lastIndex + 1, regexp.length() - lastIndex - 1);

    if (modifiers.contains('i'))
        m_priv->m_ignoreCase = true;

    if (modifiers.contains('g'))
        m_priv->m_global = true;

    m_priv->compile(expression);
}

S9sRegExp::S9sRegExp (
		const S9sRegExp &orig)
{
	m_priv = orig.m_priv;

	if (m_priv) 
		m_priv->ref ();
}

S9sRegExp::~S9sRegExp()
{
	if (m_priv && m_priv->unRef() == 0)
    {
        delete m_priv;
        m_priv = 0;
	}
}

/**
 * Assignment operator to utilize the implicit sharing.
 */
S9sRegExp &
S9sRegExp::operator= (
		const S9sRegExp &rhs)
{
	if (this == &rhs)
		return *this;

	if (m_priv && m_priv->unRef() == 0)
    {
        delete m_priv;
        m_priv = 0;
	}

	m_priv = rhs.m_priv;
	if (m_priv) 
    {
		m_priv->ref ();
	}

	return *this;
}

S9sRegExp &
S9sRegExp::operator= (
		const S9sString &rhs)
{
    m_priv->compile(rhs);

    return *this;
}

bool
S9sRegExp::operator== (
		const S9sString &rhs)
{
    return m_priv->matching(rhs);
}

bool
S9sRegExp::operator!= (
		const S9sString &rhs)
{
    return !m_priv->matching(rhs);
}

S9sString 
S9sRegExp::toString() const
{
    return m_priv->m_stringVersion;
}

/**
 * An operator to access the matched parts of the string. The first index is 0,
 * it returns the whole matched substring, 1 returns the substring that matched
 * for the first regexp subexpression (use '()' to mark parts of the regular
 * expression), 2 is the second subexpression and so on.
 */
S9sString 
S9sRegExp::operator[](
        int index) const
{
    return m_priv->index(index);
}

/**
 * This is a method that checks for the first match the same way the similar
 * JavaScript function does. It will consider the "global" property and if it
 * set to true it will do a continuous search for the same pattern.
 *
 * Here is an example that shows how to use this function:
 * http://www.w3schools.com/jsref/jsref_regexp_lastindex.asp
 */
bool
S9sRegExp::test(
        const S9sString &theString)
{
    return m_priv->test(theString);
}

/**
 * \returns the index of the last character matched to the whole pattern
 *
 * This method should work exactly the same way it is defined for the JavaScript
 * language regular expressions.
 */
int 
S9sRegExp::lastIndex() const
{
    return m_priv->m_match[0].rm_eo;
}

int 
S9sRegExp::firstIndex() const
{
    return m_priv->m_match[0].rm_so;
}

/**
 * 
 */
S9sVariantList
S9sRegExp::match(
        const S9sString &rhs)
{
    if (!m_priv->m_global)
        return m_priv->match(rhs);
    
    //
    // Here is how the global mode works.
    //
    S9sVariantList retval;
    for (;;)
    {
        if (!m_priv->test(rhs))
            break;

        retval << m_priv->index(0);
    }

    return retval;
}

/**
 * This function does exactly the same that is implemented in the JavaScript
 * regexp function exec(). Here is the documentation for that:
 *
 * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/RegExp/exec
 */
S9sVariantMap
S9sRegExp::exec(
        const S9sString &rhs)
{
    S9sVariantList theList;
    S9sVariantMap  theMap;

    theList = m_priv->match(rhs);
    for (uint idx = 0; idx < theList.size(); ++idx)
    {
        S9sString name;

        name.sprintf("%u", idx);
        theMap[name] = theList[idx];
    }

    theMap["index"]      = firstIndex();
    theMap["input"]      = rhs;
    // When "g" is absent, this will remain as 0.
    theMap["lastIndex"]  = m_priv->m_global ? lastIndex() : 0;
    theMap["ignoreCase"] = ignoreCase();
    theMap["global"]     = m_priv->m_global;
    theMap["multiline"]  = false;
    theMap["source"]     = m_priv->m_stringVersion;

    return theMap;
}

void
S9sRegExp::replace(
        S9sString &theString,
        S9sString  replacement)
{
    m_priv->replace(theString, replacement);
}

/**
 * Sets if the regular expression will be matched ignoring the letter case or
 * not. This function will also reset the previous matches but it will preserve
 * the regular expression expression.
 */
void 
S9sRegExp::setIgnoreCase(
        bool value)
{
    m_priv->setIgnoreCase(value);
}

/**
 *
 */
bool
S9sRegExp::ignoreCase() const
{
    return m_priv->m_ignoreCase;
}

bool
S9sRegExp::global() const
{
     return m_priv->m_global;
}

S9sVariant
S9sRegExp::source() const
{
    return m_priv->m_stringVersion;
}

void
S9sRegExp::setSource(
        const S9sVariant &value)
{
    m_priv->compile(value.toString());
}


