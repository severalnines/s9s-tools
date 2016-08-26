/* 
 * Copyright (C) 2011-2015 severalnines.com
 */
#pragma once

#if 0
    #ifdef __clang__
      #include <stdatomic.h>
      typedef atomic_int S9sAtomicInt;
    #else
      #if (__GNUC__ <= 4) && (__GNUC_MINOR__ <= 4)
        #include <cstdatomic>
      #else
        #include <atomic>
      #endif
      typedef std::atomic_int S9sAtomicInt;
    #endif
#else
    // FIXME: need to find out how to do this when it is not supported by the
    // compiler.
    typedef int S9sAtomicInt;
#endif
