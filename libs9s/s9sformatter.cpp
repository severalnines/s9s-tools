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
        ulonglong mBytes)
{
    S9sOptions *options = S9sOptions::instance();
    S9sString   retval;
    S9sVariant  bytes = mBytes * (1024ull * 1024ull);

    if (!options->humanReadable())
    {
        retval.sprintf("%'llu", bytes.toULongLong());
    } else if (bytes.toTBytes() > 1.0)
    {
        retval.sprintf("%.1fTB", bytes.toTBytes());
    } else if (bytes.toGBytes() >= 1.0) 
    {
        retval.sprintf("%.1fGB", bytes.toGBytes());
    } else {
        retval.sprintf("%.1fMB", bytes.toMBytes());
    }

    return retval;
}

S9sString 
S9sFormatter::kiloBytesToHuman(
        ulonglong kBytes)
{
    S9sOptions *options = S9sOptions::instance();
    S9sString   retval;
    S9sVariant  bytes = kBytes * (1024ull);

    if (!options->humanReadable())
    {
        retval.sprintf("%'llu", kBytes);
    } else if (bytes.toTBytes() > 1.0)
    {
        retval.sprintf("%.1fTB", bytes.toTBytes());
    } else if (bytes.toGBytes() >= 1.0) 
    {
        retval.sprintf("%.1fGB", bytes.toGBytes());
    } else {
        retval.sprintf("%.1fMB", bytes.toMBytes());
    }

    return retval;
}
