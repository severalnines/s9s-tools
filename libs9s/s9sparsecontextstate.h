/*
 * Copyright (C) 2011-2015 severalnines.com
 */
#pragma once

#include "s9sstring.h"

/**
 * A state that is used by the CmonParseContext to represent one input
 * file/string. These are used in a stack when the parser supports include
 * files.
 */
class S9sParseContextState
{
    public:
        S9sParseContextState();

        S9sString    m_inputString;
        int          m_parserCursor;
        int          m_currentLineNumber;
        S9sString    m_fileName;
        void        *m_scannerBuffer;
};
