/*
 * Copyright (C) 2011-2016 severalnines.com
 */
#include "s9sunittest.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#define TERM_CLEAR_RIGHT "\033[0K"
#define g_option_syntax_highlight true

static struct option long_options[] =
{
    { "help",             no_argument,       0, 'h' },
    { "verbose",          no_argument,       0, 'v' },
    { 0, 0, 0, 0 }
};

S9sUnitTest::S9sUnitTest() :
    m_verbose(false),
    m_isEndurance(false),
    m_isLoop(false),
    m_untilMinutes(-1),
    m_basename(0),
    m_failedCounter(0),
    m_nChecks(0),
    m_keepDatabase(false),
    m_sendMails(false),
    m_updateExamples(false),
    m_noHalt(false)
{
}

S9sUnitTest::~S9sUnitTest()
{
    free(m_basename);
    m_basename = 0;
}

void 
S9sUnitTest::failed() 
{
    m_failedCounter++; 
}
 
int 
S9sUnitTest::failedCounter() const 
{
    return m_failedCounter; 
}

/**
 * \returns how many checks was made by calling the S9S_COMPARE macro.
 */
int
S9sUnitTest::nChecks() const
{
    return m_nChecks;
}

void
S9sUnitTest::resetCounters()
{
    m_nChecks = 0;
}

bool
S9sUnitTest::haveTerminal()
{
    static bool have_terminal;
    static bool detected;

    if (detected)
        return have_terminal;

    if (isatty (fileno (stdout)))
    {
        detected = true;
        have_terminal = true;
        return have_terminal;
    }

    struct stat out_stat;
    int retval = fstat(fileno (stdout), &out_stat);
    if (retval < 0) 
    {
        // can't detect..
        return true;
    }

    // check if the output is piped
    bool piped = S_ISFIFO (out_stat.st_mode);
    bool redirected = S_ISREG (out_stat.st_mode);
    detected = true;
    have_terminal = ! piped && ! redirected;

    return have_terminal;
}


void
S9sUnitTest::printHelp()
{
    // Nothing to do here, this is a virtual function.
}

void
S9sUnitTest::printHelpAndExit (
        int exitCode)
{
    fprintf (stderr,
"Usage: %s [OPTION]... [TESTNAME]...\n\n", 
    m_basename);

    // giving a chance for the individual unit tests to print something.
    printHelp();

    fprintf(stderr, 
" -h, --help                 Print this help and exit.\n"
" -v, --verbose              Print more messages to the standard output.\n"
" -g, --logs                 Print the logs to the standard output.\n"
"\n");

    exit(exitCode);
}

#if 1
/**
 * Starts the execution of the tests taking the command line arguments into
 * account.
 */
int
S9sUnitTest::execute (
        int   argc,
        char *argv[])
{
    bool success;

    m_commandLine = "";
    for (int n = 0; argv[n] != NULL; ++n)
    {
        if (!m_commandLine.empty())
            m_commandLine += " ";

        m_commandLine += argv[n];
    }

    m_argv0 = argv[0];
    m_testName = argv[0];
    m_testName = "Helpers::basename(m_testName)";

    // set the locale to 'C'
    setlocale (LC_ALL, "C");

    m_basename = strdup(basename(argv[0]));

    int c;
    while (true)
    {
        int option_index = 0;
        c = getopt_long (argc, argv, "hvgu:p:s:elknc",
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case 'h':
                printHelpAndExit(EXIT_SUCCESS);
                break;

            case 'e':
                m_isEndurance = true;
                break;
            
            case 'l':
                m_isLoop = true;
                break;

            case 'v':
                m_verbose = true;
                break;

            case 'n':
                m_noHalt = true;
                break;
           
            case 't':
                m_testCase = optarg;
                break;

            case 1:
                m_untilMinutes = atoi(optarg);
                break;

            case 9:
                m_updateExamples = true;
                break;

            default:
                printHelpAndExit (EXIT_FAILURE);
                break;
        }
    }

    while (optind < argc)
        m_argvElements[argv[optind++]] = true;

    /**************************************************************************
     * Preparing, running the tests and finalizing.
     */
    time_t testsStartedAt = time(NULL);
    time_t now;

    setRunningTestName("prepareToRun");
    success = prepareToRun();
    if (!success)
        m_failedCounter++;

    testFunctionEnded(success);
    
    if (success)
    {
        bool repeat;
        // This should run all the tests one-by-one.
        do {
            runTest();

            now = time(NULL);
            repeat = m_isLoop;
            if (m_untilMinutes > 0 &&
                    (now - testsStartedAt) < m_untilMinutes * 60)
                repeat = true;

        } while (repeat);
    } else {
        m_failedCounter++;
    }

    //
    //
    //
    setRunningTestName("finalizeRun");
    success = finalizeRun();
    if (!success)
        m_failedCounter++;

    testFunctionEnded(success);

    /*
     * Printing the summary.
     */
    now = time(NULL);
    if (m_failedCounter > 0)
    {
        if (g_option_syntax_highlight)
        {
            printf ("%sFAILED %s: %s\n\n", "\033[1;31m", "\033[0;39m",
                    m_basename);
        } else {
            printf ("FAILED : %s\n\n", m_basename);
        }
    } else {
        if (g_option_syntax_highlight)
        {
            printf ("%sSUCCESS%s (%3d secs): %s\n\n", 
                    "\033[1;32m", "\033[0;39m",
                    (int) (now - testsStartedAt),
                    m_basename);
        } else {
            printf ("SUCCESS (%3d secs): %s\n\n", 
                    (int) (now - testsStartedAt),
                    m_basename);
        }
    }

    return m_failedCounter;
}
#endif

/**
 * When the test is started this function will be the basename of the executable
 * file (AKA basename(argv[0]).
 */
S9sString 
S9sUnitTest::testName() const
{ 
    return m_testName; 
}

void
S9sUnitTest::setTestName(
        const S9sString &name)
{
    m_testName = name;
}

S9sString 
S9sUnitTest::testCase() const
{ 
    return m_testCase;
}

void
S9sUnitTest::setTestCase(
        const S9sString &name)
{
    m_testCase = name;
}

bool
S9sUnitTest::prepareToRun()
{
    return true;
}

bool
S9sUnitTest::finalizeRun()
{
    return true;
}

void
S9sUnitTest::setRunningTestName(
        const S9sString name)
{
    m_runningTestName = name;
    m_testFunctionStartTime = time(NULL);
}

time_t
S9sUnitTest::testFunctionStartTime() const
{
    return m_testFunctionStartTime;
}

/**
 * Helper function to print messages while the test is running.
 */
void
S9sUnitTest::message(
        const char *formatString,
        ...)
{
    if (m_runningTestName.empty())
        return;

	printf("\r  %-24s: RUNNING ",
            STR(m_runningTestName));

    S9sString  theString;
    va_list     arguments;
    
    va_start(arguments, formatString);
    theString.vsprintf (formatString, arguments);
    va_end(arguments);
    
    theString.replace("\n", "\\n");
    theString.replace("\r", "\\r");
    // XXX: strange i can't see any actual printout here... :-S

    printf(TERM_CLEAR_RIGHT);
    fflush(stdout);
}

void
S9sUnitTest::incrementChecks()
{
    m_nChecks++;
    if (haveTerminal())
        message("(%4d checks) ", m_nChecks);
}

void
S9sUnitTest::testFunctionEnded(
        bool success)
{
    if (success)
    {
        if (g_option_syntax_highlight) 
            printf ("\r  %-24s: SUCCESS (%4d checks, %3ds) %s\n", 
                STR(m_runningTestName), 
                nChecks(), 
                (int) (time(NULL) - testFunctionStartTime()), 
                TERM_CLEAR_RIGHT); 
        else 
            printf ("\r  %-24s: SUCCESS (%4d checks, %3ds) \n", 
                STR(m_runningTestName), 
                nChecks(),
                (int) (time(NULL) - testFunctionStartTime()));

        //resetCounters(); 
        fflush(stdout); 
    } else {
        printf ("\r  %-24s: FAILED \n", STR(m_runningTestName)); 
        //resetCounters(); 
        fflush(stdout); 
    }
}

/**
 * If the argument is a test name this method should run that test or if it is a
 * NULL pointer all the test should be executed.
 */
bool
S9sUnitTest::runTest(
        const char *testName)
{
    return true;
}

bool
S9sUnitTest::compare (
        const char *fileName,
        const int   lineNumber,
        const char *varName,
        const int   value1,
        const int   value2)
{
    incrementChecks();

    if (value1 == value2)
        return true;

    printf("Test failed in file %s at line %d.\n", fileName, lineNumber);
    if (!m_errorString.empty())
        printf("*** error         : %s\n", STR(m_errorString));
    printf("*** expression    : %s\n", varName);
    printf("*** required value: %d\n", value1);
    printf("*** actual value  : %d\n", value2);
    printf("\n");
    fflush(stdout);

    m_failedCounter++;
    return false;
}

bool
S9sUnitTest::compare (
        const char *fileName,
        const int   lineNumber,
        const char *varName,
        bool        value1,
        bool        value2)
{
    incrementChecks();

    if (value1 == value2)
        return true;

    printf("Test failed in file %s at line %d.\n", fileName, lineNumber);
    if (!m_errorString.empty())
        printf("*** error         : %s\n", STR(m_errorString));
    printf("*** expression    : %s\n", varName);
    printf("*** required value: %s\n", value1 ? "true" : "false");
    printf("*** actual value  : %s\n", value2 ? "true" : "false");
    printf("\n");
    fflush(stdout);

    m_failedCounter++;
    return false;
}

bool
S9sUnitTest::compare (
        const char *fileName,
        const int   lineNumber,
        const char *varName,
        const std::string value1,
        const std::string value2)
{
    incrementChecks();

    if (value1 == value2)
        return true;

    printf("Test failed in file %s at line %d.\n", fileName, lineNumber);
    if (!m_errorString.empty())
        printf("*** error         : %s\n", STR(m_errorString));
    printf("*** expression    : %s\n", varName);
    printf("*** required value: '%s'\n", value1.c_str());
    printf("*** actual value  : '%s'\n", value2.c_str());
    printf("\n");
    fflush(stdout);

    m_failedCounter++;
    return false;
}

