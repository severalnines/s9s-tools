/*
 * Copyright (C) 2011-2015 severalnines.com
 */
#include "s9sparsecontext.h"

#include <string.h>

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sParseContext::S9sParseContext(
        const char *input) :
    m_flex_scanner(0)
{
    m_states.push(S9sParseContextState());
    m_states.top().m_inputString = input;

    m_currentToken = 0;
}

S9sParseContext::S9sParseContext(
        const S9sParseContext &orig)
{
    m_states       = orig.m_states;
    m_currentToken = 0;
}

S9sParseContext::~S9sParseContext()
{
}

/**
 * Stores the input string that will be parsed in the object and prepares to
 * parse.
 */
void
S9sParseContext::setInput(
        const S9sString &input)
{
    if (m_states.empty())
        m_states.push(S9sParseContextState());

    m_states.top().m_inputString  = input;
    m_states.top().m_parserCursor = 0;
}

/**
 * \returns the input string that will be parsed, is being parsed or was parsed
 *   before
 */
S9sString 
S9sParseContext::input() const
{
    if (m_states.empty())
        return m_lastState.m_inputString;

    return m_states.top().m_inputString;
}

/**
 * Sets the file name for the context. Even if no real files are used this
 * method should be called to set the file name because that file name will
 * appear in the error messages (together with the line number) to help the 
 * user to identify the place of an * error.
 */
void
S9sParseContext::setFileName(
        const S9sString &fileName)
{
    if (m_states.empty())
        return;

    m_states.top().m_fileName = fileName;
}

/**
 * \returns the file name that is currently processed
 *
 * This is used when generating the error messages.
 */
const S9sString &
S9sParseContext::fileName() const
{
    if (m_states.empty())
        return m_lastState.m_fileName;

    return m_states.top().m_fileName;
}

/**
 * \returns the current line number
 *
 * We use this line number in error messages, so it starts from 1 at the first
 * line. 
 */
int 
S9sParseContext::lineNumber() const
{
    if (m_states.empty())
        return m_lastState.m_currentLineNumber;

    return m_states.top().m_currentLineNumber;
}

/**
 * Increments the current line number that is used in error messages. The parser
 * or more likely the lexer should call it to keep track of the line numbers
 */
void
S9sParseContext::incrementLineNumber()
{
    if (m_states.empty())
        return;

    ++m_states.top().m_currentLineNumber;
}

/**
 * The token is a string that helps locate an error inside the line. If the
 * lexer sets the token in the context it will be added to the error strings.
 */
void
S9sParseContext::tokenFound(
        const char *token)
{
    m_currentToken = token;
}

const char *
S9sParseContext::lastToken() const
{
    return m_currentToken;
}

/**
 * Public function called before parsing, resets the context.
 */
void
S9sParseContext::reset()
{
    m_states.top().m_currentLineNumber = 1;
    m_errorString.clear();
    m_currentToken = NULL;
}

/**
 * \param buffer the data should be put here
 * \param maxsize the size of the buffer
 * \returns how many characters are available
 *
 * This is the standard lex function that the lexer calls to read the input.
 */
int 
S9sParseContext::yyinput(
        char *buffer, 
        int   maxsize)
{
    if (m_states.empty())
        return 0;

	const char *data = STR(m_states.top().m_inputString);
	int         dataLength = m_states.top().m_inputString.length();

	int numBytes = maxsize;
	if (numBytes > dataLength - m_states.top().m_parserCursor)
		numBytes = dataLength - m_states.top().m_parserCursor;

	if (numBytes < 0)
		numBytes = 0;

	if (numBytes > 0)
	{
		memcpy(buffer, data + m_states.top().m_parserCursor, numBytes);
		m_states.top().m_parserCursor += numBytes;
	}

	return numBytes;
}
