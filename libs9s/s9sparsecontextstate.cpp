/*
 * Copyright (C) 2011-2015 severalnines.com
 */
#include "s9sparsecontextstate.h"

S9sParseContextState::S9sParseContextState() :
    m_parserCursor(0),
    m_currentLineNumber(1),
    m_scannerBuffer(0)
{
}

