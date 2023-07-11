/*
 * Copyright (C) 2011-2015 severalnines.com
 */
#pragma once

#define STR_LIST(_thelist) ((_thelist).toString(';').c_str())

#include "s9svector.h"
#include "s9sstring.h"

class S9sStringList : public S9sVector<S9sString>
{
};

