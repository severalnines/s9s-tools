#include "s9sdebug.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/**
 * This function is for printing debug messages that are used only by
 * developers.
 */
void
s9s_print_message (
        S9sMessageLevel   type,
        const char        *function,
        const char        *formatstring,
        ...)
{
    FILE          *stream = stderr;
    va_list        args;

    va_start (args, formatstring);
    switch (type) 
    {
        case DebugMsg:
            fprintf (stream, "%s%s%s: ", 
                    TERM_GREEN TERM_BOLD, function, TERM_NORMAL);
            vfprintf (stream, formatstring, args);
            break;

        case WarningMsg:
            fprintf (stream, "%s%s%s: ", TERM_RED, function, TERM_NORMAL);
            vfprintf (stream, formatstring, args);
            break;
        
        case SystemMsg:
            fprintf (stream, "%s%s%s: ", TERM_RED, function, TERM_NORMAL);
            vfprintf (stream, formatstring, args);
            break;
    }

    va_end(args);
    fprintf(stream, "\n");
    fflush(stream);
}

