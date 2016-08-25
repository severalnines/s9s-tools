/*
 * Copyright (C) 2011-2015 severalnines.com
 */
#include "s9sparsecontext.h"

#include "S9sGlobal"
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
 * This function should be called by the parser/lexer when an error found. A
 * human readable string will be passed and handled in the context.
 */
void 
S9sParseContext::errorFound(
        const char *errorString)
{
    if (lastToken() == NULL)
    {
        m_errorString.sprintf("%s in line %d", 
                errorString, lineNumber());
    } else {
        m_errorString.sprintf("%s in line %d near token '%s'", 
                errorString, lineNumber(), lastToken());
    }
}

void 
S9sParseContext::setErrorString(
        const S9sString &error)
{
    m_errorString = error;
}


/**
 * \returns the error string if available
 */
S9sString 
S9sParseContext::errorString() const
{
    return m_errorString;
}

/**
 * \param scannerBuffer the pointer to the scanners internal buffer. This will
 *   be handled in a stack and returned by the eofFound() function
 *
 * The scanner should call this function to start processing an include file
 * while keeping the current file to be processed when it is finished the
 * include file. handling include files is a recursive process, include files
 * can include other files.
 */
bool 
S9sParseContext::includeFound(
        const S9sString  &fileName,
        bool              isSystemFile,
        S9sString        &errorString,
        void             *scannerBuffer)
{
    S9S_UNUSED(isSystemFile)

    if (m_states.size() > 30)
    {
        errorString = "Input stack is too deep";
        return false;
    }

    S9sString  content;
    bool       success;

    success = getFileContent(fileName, content, errorString);
    m_states.push(S9sParseContextState());

    m_states.top().m_fileName      = fileName;
    m_states.top().m_inputString   = content;
    m_states.top().m_scannerBuffer = scannerBuffer;

    //m_includeFiles[fileName]       = NoPragma;
    return success;
}

/**
 * \returns the previously set scanner buffer or NULL if there are no more files
 *   to process
 *
 * This method can be called by the parser. See the documentation of 
 * includeFound() for further details about the return value.
 */
void * 
S9sParseContext::eofFound()
{
    if (m_states.empty())
    {
        S9S_WARNING("State stack is empty.");
        return NULL;
    }
    
    m_lastState = m_states.pop();
    return m_lastState.m_scannerBuffer;
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

/**
 * This method is called when an include file needs to be loaded into the
 * memory. It implements a real file read, so inherited class should be
 * implementing this virtual function to override that.
 */
bool 
S9sParseContext::getFileContent(
        const S9sString &fileName,
        S9sString       &content,
        S9sString       &errorString)
{
    bool success = false;
#if 0
    for (uint idx = 0u; idx < m_includeDirs.size(); ++idx)
    {
        S9sString path = Helpers::buildPath(m_includeDirs[idx], fileName);
        S9sFile   inputFile(path);

        if (!inputFile.exists())
            continue;

        success = inputFile.readTxtFile(content);
        if (!success)
        {
            S9S_WARNING("ERROR: %s", STR(inputFile.errorString()));
            errorString = inputFile.errorString();
        }

        return success;
    }

    errorString.sprintf("Include file '%s' was not found", STR(fileName));
    S9S_WARNING("%s", STR(errorString));
#endif
    return success;
}
