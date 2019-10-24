#include "s9sdebug.h"

#include "S9sOptions"
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

void
s9s_log(
        const char    *file,
        const int      line,
        const char    *formatstring,
        ...)
{
    static int  sequence = 0;
    S9sOptions *options = S9sOptions::instance();
    S9sString   fileName = options->logFile();
    S9sString   logLine;

    if (!fileName.empty())
    {
        FILE    *stream;
        va_list  args;

        stream = fopen(STR(fileName), "a");
        if (stream == NULL)
            return;

        va_start(args, formatstring);
        logLine.vsprintf(formatstring, args);
        fprintf(stream, "%05d %20s:%5d DEBUG %s\n", 
                sequence, file, line, STR(logLine));
        fflush(stream);
        va_end(args);
        
        fclose(stream);

        ++sequence;
        if (sequence > 99999)
            sequence = 0;
    }
}

