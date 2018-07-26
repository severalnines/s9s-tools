/* 
 * Copyright (C) 2016 severalnines.com
 */
#include "s9sformatter.h"

#include "S9sOptions"

//#define DEBUG
#define WARNING
#include "s9sdebug.h"

S9sString 
S9sFormatter::bytesToHuman(
        ulonglong bytes) const
{
    S9sOptions *options = S9sOptions::instance();
    S9sString   retval;
    S9sVariant  variant = bytes;

    if (!options->humanReadable())
    {
        retval.sprintf("%'llu", variant.toULongLong());
    } else if (variant.toTBytes() > 1.0)
    {
        retval.sprintf("%.1fTB", variant.toTBytes());
    } else if (variant.toGBytes() >= 1.0) 
    {
        retval.sprintf("%.1fGB", variant.toGBytes());
    } else {
        retval.sprintf("%.1fMB", variant.toMBytes());
    }

    return retval;
}

S9sString 
S9sFormatter::mBytesToHuman(
        ulonglong mBytes) const
{
    return bytesToHuman(mBytes * (1024ull * 1024ull));
}

S9sString 
S9sFormatter::kiloBytesToHuman(
        ulonglong kBytes) const
{
    return bytesToHuman(kBytes * 1024ull);
}
