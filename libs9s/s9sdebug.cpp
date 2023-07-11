#include "s9sdebug.h"

#include "s9soptions.h"
#include "s9sdatetime.h"
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
    S9sOptions *options = S9sOptions::instance();
    S9sString   fileName = options->logFile();
    S9sString   logLine;
    time_t      now = time(NULL);

    if (!fileName.empty())
    {
        FILE    *stream;
        va_list  args;

        stream = fopen(STR(fileName), "a");
        if (stream == NULL)
            return;

        va_start(args, formatstring);
        logLine.vsprintf(formatstring, args);
        fprintf(stream, "%s %20s:%5d DEBUG %s\n", 
                S9S_TIME_T(now), file, line, STR(logLine));
        fflush(stream);
        va_end(args);
        
        fclose(stream);
    }
}

