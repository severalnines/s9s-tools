/*
 * Copyright (C) 2011-2015 severalnines.com
 */
#pragma once

#include "s9sstring.h"
#include "s9sstack.h"
#include "s9smap.h"
#include "s9svariantlist.h"
#include "s9sparsecontextstate.h"

class S9sParseContext
{
    public:
        S9sParseContext(const char *input);
        S9sParseContext(const S9sParseContext &orig);
        virtual ~S9sParseContext();

        void setInput(const S9sString &input);
        S9sString input() const;
        int yyinput(char *buffer, int maxsize);
        
        void setFileName(const S9sString &fileName);
        const S9sString &fileName() const;

        int lineNumber() const;
        void incrementLineNumber();

        void tokenFound(const char *token);
        const char *lastToken() const;

        void errorFound(const char *errorString);
        void setErrorString(const S9sString &error);
        S9sString errorString() const;

        bool includeFound(
                const S9sString  &fileName,
                bool              isSystemFile,
                S9sString        &errorString,
                void             *scannerBuffer);

        void *eofFound();
        void reset();

    protected:
        bool getFileContent(
                const S9sString &fileName,
                S9sString       &content,
                S9sString       &errorString);

    public:
        /** Flex code needs this pointer to store its internal state in it. */
        void *m_flex_scanner; 

    private:
        S9sStack<S9sParseContextState>  m_states;
        S9sParseContextState            m_lastState;
        const char                     *m_currentToken;
        S9sString                       m_errorString;
        S9sString                       m_emptyString;
        S9sVariantList                  m_includeDirs;
        // path -> pragma
        S9sMap<S9sString, int>          m_includeFiles;
};
