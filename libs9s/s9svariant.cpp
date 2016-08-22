/*
 * Copyright (C) 2011-2016 severalnines.com
 */
#include "s9svariant.h"

 
S9sVariant::~S9sVariant()
{
    clear();
}

void
S9sVariant::clear()
{
    switch (m_type) 
    {
        case Invalid:
        case Bool:
        case Int:
        case Ulonglong:
        case Double:
            // Nothing to do here.

        case String:
            delete m_union.stringValue;
            break;

        case Map:
        case List:
        case Array:
            // Not yet implemented.
            break;
    }
}
