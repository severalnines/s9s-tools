#ifdef DEBUG_ALL
#  ifndef DEBUG
#    define DEBUG
#  endif
#  ifndef WARNING
#    define DEBUG
#  endif
#  ifndef MESSAGE
#    define DEBUG
#  endif
#endif

#ifdef PUBLIC_RELEASE
#  ifdef DEBUG
#    undef DEBUG
#  endif
#  ifdef WARNING
#    undef WARNING
#  endif
#endif

/*
 * If the debug facility is enabled we also enable all the warning messages.
 */
#if defined(DEBUG) && !defined(WARNING)
#  define WARNING
#endif

#if defined(WARNING) && !defined(CRITICAL)
#  define CRITICAL
#endif

/**
 * Enum to be used as a severity level for debug messages.
 */
typedef enum S9sMessageLevel
{
    DebugMsg,
    SystemMsg,
    WarningMsg
} S9sMessageLevel;

#undef S9S_DEBUG
#ifdef DEBUG
/**
 * The CMON_DEBUG macro is used to print normal debug messages to be seen only
 * during the development.
 */
#  define S9S_DEBUG(...) s9s_print_message (\
        DebugMsg, \
        __func__, \
        __VA_ARGS__)
#else
/**
 * The S9S_DEBUG macro is used to print normal debug messages to be seen only
 * during the development.
 */
#  define S9S_DEBUG(...) { /* Nothing... */ }
#endif

#undef S9S_WARNING
#ifdef WARNING
/**
 * The CMON_WARNING is used to print warning messages to be seen only during the
 * development.
 */
#  define S9S_WARNING(...) s9s_print_message (\
        WarningMsg, \
        __func__, \
        __VA_ARGS__)
#else
/**
 * The S9S_WARNING is used to print warning messages to be seen only during the
 * development.
 */
#  define S9S_WARNING(...) { /* Nothing... */ }
#endif

#ifndef S9SDEBUG_H
/** Protector macro, dosygen complains about it if there is no documentation. */
#define S9SDEBUG_H

//#define DEBUG_SUPPRESS_COLOR
#ifdef DEBUG_SUPPRESS_COLOR
#  define TERM_RED     ""
#  define TERM_YELLOW  ""
#  define TERM_GREEN   ""
#  define TERM_BLUE    ""
#  define TERM_NORMAL  ""
#  define TERM_BOLD    ""
#  define TERM_UNDERLINE    ""
#  define XTERM_COLOR_1 ""
#  define XTERM_COLOR_2 ""
#  define XTERM_COLOR_3 ""
#  define XTERM_COLOR_4 ""
#  define XTERM_COLOR_5 ""
#  define XTERM_COLOR_6 ""
#  define XTERM_COLOR_7 ""
#  define XTERM_COLOR_8 ""
#  define XTERM_COLOR_9 ""
#  define XTERM_COLOR_10 ""
#  define XTERM_COLOR_11 ""
#  define XTERM_COLOR_12 ""
#  define XTERM_COLOR_13 ""
#  define XTERM_COLOR_14 ""
#  define XTERM_COLOR_15 ""
#  define XTERM_COLOR_16 ""
#else 
/** Yellow terminal color sequence. */
#  define TERM_RED  "\033[1;31m" 
/** Red terminal color sequence. */
#  define TERM_YELLOW     "\033[1;33m" 
/** Green terminal color sequence. */
#  define TERM_GREEN   "\033[1;32m"
/** Blue terminal color sequence. */
#  define TERM_BLUE    "\033[1;34m"
/** Terminal reset sequence. */
#  define TERM_NORMAL  "\033[0;39m"
/** Turns the terminal to bold. */
#  define TERM_BOLD    "\033[1m"

#  define TERM_INVERSE "\033[7m"

/** Turns underline in the terminal. */
#  define TERM_UNDERLINE    "\033[4m"
/** Dark red. */
#  define XTERM_COLOR_1 "\033[38;5;1m"
/** Dark green. */
#  define XTERM_COLOR_2 "\033[38;5;2m"
/** Dark orange. */
#  define XTERM_COLOR_3 "\033[38;5;3m"
/** Dark blue. */
#  define XTERM_COLOR_4 "\033[38;5;4m"
/** Brown. */
#  define XTERM_COLOR_5 "\033[38;5;5m"
/** Greenish blue. */
#  define XTERM_COLOR_6 "\033[38;5;6m"
/** Dark grey. */
#  define XTERM_COLOR_7 "\033[38;5;8m"
/** Red color. */
#  define XTERM_COLOR_8 "\033[38;5;9m"
/** Green color. */
#  define XTERM_COLOR_9 "\033[38;5;10m"
/** Blue color. */
#  define XTERM_COLOR_10 "\033[38;5;12m"
/** Light brown color. */
#  define XTERM_COLOR_11 "\033[38;5;13m"
/** Light blue color. */
#  define XTERM_COLOR_12 "\033[38;5;14m"
/** Terminal custom color. */
#  define XTERM_COLOR_13 "\033[38;5;17m"
/** Clear dark blue terminal color sequence. */
#  define XTERM_COLOR_14 "\033[38;5;21m"
/** Blackish blue terminal color sequence. */
#  define XTERM_COLOR_15 "\033[38;5;19m"
/** Neon color terminal color sequence. */
#  define XTERM_COLOR_16 "\033[38;5;93m"

#define XTERM_COLOR_RED          "\033[0;31m"
#define XTERM_COLOR_GREEN        "\033[0;32m"
#define XTERM_COLOR_ORANGE       "\033[0;33m"
#define XTERM_COLOR_BLUE         "\033[0;34m"
#define XTERM_COLOR_PURPLE       "\033[0;35m"
#define XTERM_COLOR_CYAN         "\033[0;36m"
#define XTERM_COLOR_LIGHT_GRAY   "\033[0;37m"
#define XTERM_COLOR_DARK_GRAY    "\033[1;30m"
#define XTERM_COLOR_LIGHT_RED    "\033[1;31m"
#define XTERM_COLOR_LIGHT_GREEN  "\033[1;32m"
#define XTERM_COLOR_YELLOW       "\033[1;33m"
#define XTERM_COLOR_LIGHT_BLUE   "\033[1;34m"
#define XTERM_COLOR_LIGHT_PURPLE "\033[1;35m"
#define XTERM_COLOR_LIGHT_CYAN   "\033[1;36m"
#define XTERM_COLOR_WHITE        "\033[1;37m"


#endif

void 
s9s_print_message (
        S9sMessageLevel  type,
        const char       *function,
        const char       *formatstring,
        ...);

/**
 * A macro to print booleans.
 */
#define S9S_BOOL(_boolval) ((_boolval ? "true" : "false"))

#endif

