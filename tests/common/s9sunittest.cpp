/*
 * Copyright (C) 2011-2016 severalnines.com
 */
#include "s9sunittest.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define TERM_CLEAR_RIGHT "\033[0K"
#define g_option_syntax_highlight true

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
" -u, --sqluser=USER         MySQL username (to create and use the 'testdb').\n"
" -p, --sqlpassword=PASSWORD Password for the local testing MySQL user.\n"
" -e, --endurance            Turn on endurance test cases (longer runs).\n"
" --update-examples          Update the example files for the documentation.\n"
" -l, --loop                 Loop test cases indefinitely until aborted.\n"
" --until=MINUTES            Loop until the specified time elapsed.\n"
" -k, --keepdatabase         Do not remove the cmon database.\n"
" -c, --color                Print in color.\n"
" --sendmails                Enables the sending of test emails.\n"
" --emailaddress=ADDRESS     If set emails are going to be sent to here.\n"
" --template=NAME            Run using the named environment (defaults to\n"
"                            'vagrant').\n"
" --mysql-version=STRING     Set the MySQL version for tests that use it. The\n"
"                            default is '5.5'\n"
" --vendor=NAME              Set the vendor name for tests that use it. \n"
"                            Supported values are 'percona', 'mariadb', \n"
"                            'codership', '10gen'. The default is \n"
"                            'percona'\n"
//" --testcase=NAME            What test to run. For now, used for mongo vendor only.\n"
" --os=OSNAME                Sets the name of the operating system. The test\n"
"                            should use the name of the os as a search\n"
"                            category in the test config \n"
"                            (e.g. ~/.cmon/<TEMPLATE>.conf).\n"
" -n, --nohalt               Do not halt the VMs when finished.\n"
"\n");

    exit(exitCode);
}

#if 0
/**
 * Starts the execution of the tests taking the command line arguments into
 * account.
 */
int
CmonUnitTest::execute (
        int   argc,
        char *argv[])
{
    bool success;

    // Check for local config file to read in values before checking 
    // them on the command line
    CmonString configFileName("~/.cmon/unit_test.conf");
    CmonConfigFile configFile;
    configFile.setFileName(configFileName);
    if (configFile.sourceFileExists())
    {
        if (!configFile.parseSourceFile())
        {
            fprintf(stderr, "\nFailed to parse the %s configuration file+", 
                    STR(configFileName));
            return false;
        }

        m_sqluser = configFile.variableValue("sqluser");
        m_sqlpassword = configFile.variableValue("sqlpassword");
        m_sqlhost = configFile.variableValue("sqlhost");
    }

    m_commandLine = "";
    for (int n = 0; argv[n] != NULL; ++n)
    {
        if (!m_commandLine.empty())
            m_commandLine += " ";

        m_commandLine += argv[n];
    }

    m_argv0 = argv[0];
    m_testName = argv[0];
    m_testName = Helpers::basename(m_testName);

    // before we anything here, check if we need to re-start ourselves with
    // 'sudo'
    if (needRootAccess() && geteuid() != 0)
    {
        int retval = 0;
        // so, lets call ourselves with non-interactive sudo
        CmonString cmdline = "sudo -n ";
        
        for (char **argv2 = argv; argv2 != 0 && *argv2 != 0; argv2++)
        {
           cmdline << CmonString (*argv2);
           cmdline << " ";
        }

        printf("* test needs 'root' account, starting:\n%s\n", STR(cmdline));
        fflush(stdout);

        retval = system(STR(cmdline));
        if (WEXITSTATUS(retval) > 128 && WEXITSTATUS(retval) <= (128+64))
        {
            CMON_WARNING("ERROR: got SIGNAL %d.", WEXITSTATUS(retval) - 128);
        }

        exit(WEXITSTATUS(retval));
    }

    // set the locale to 'C'
    setlocale (LC_ALL, "C");

    m_basename = strdup (basename(argv[0]));

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

            case 'g':
                g_option_debug_log = true;
                break;

            case 'u':
                m_sqluser = optarg;
                break;

            case 'p':
                m_sqlpassword = optarg;
                break;

            case 's':
                m_sqlhost = optarg;
                break;

            case 'k':
                m_keepDatabase = true;
                break;

            case 'n':
                m_noHalt = true;
                break;
           
            case 'c':
                g_option_syntax_highlight = true;
                break;

            case 't':
                m_testCase = optarg;
                break;

            case 1:
                m_untilMinutes = atoi(optarg);
                break;

            case 2:
                m_emailAddress = optarg;
                break;

            case 3:
                m_sendMails = true;
                break;

            case 4:
                enableCrashHandler (true);
                break;

            case 5:
                m_templateName = optarg;
                break;

            case 6:
                m_vendor = optarg;
                break;

            case 7:
                m_mysqlVersion = optarg;
                break;

            case 8:
                m_operatingSystem = optarg;
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
    {
        m_argvElements[argv[optind++]] = true;
    }

    // some default values...
    if (m_sqluser.empty())
        m_sqluser = "root";

    if (m_sqlpassword.empty())
        m_sqlpassword = "p";

    // for "localhost" mysql client might use local socket which we don't want,
    // so we fall back to 127.0.0.1
    if (m_sqlhost.empty () || m_sqlhost == "localhost")
        m_sqlhost = "127.0.0.1";

    /**************************************************************************
     * Ovverruling some values for all the configurations.
     */
    CmonConfiguration::setOverride(PropCcUser, sqlUser());
    CmonConfiguration::setOverride(PropCcMySqlPassword, sqlPassword());
    CmonConfiguration::setOverride(PropCcMySqlHost, sqlHost());
    CmonConfiguration::setOverride(PropOsUser, CmonString(getenv("USER")));
    CmonConfiguration::setOverride(PropOsUserHome, CmonString(getenv("HOME")));
    CmonConfiguration::setOverride(PropSshKeypath, testsshkey);

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

    CMON_WARNING("m_failedCounter: %d", m_failedCounter);
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
