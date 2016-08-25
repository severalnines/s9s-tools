/* 
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

typedef unsigned long int ulong;
typedef unsigned long long int ulonglong;
typedef unsigned int uint;
typedef long long int longlong;

/**
 * Use this to avoid unused variable warnings.
 */
template <typename T>
inline void s9sUnused(T &x) { (void)x; }

#define S9S_UNUSED(x) s9sUnused(x);
