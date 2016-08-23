/*
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

#include "S9sString"
#include "S9sMap"

#define S9S_UNIT_TEST_MAIN(classname) \
int main (int argc, char *argv[]) \
{\
    classname object; \
    int retval = object.execute(argc, argv); \
    return retval;\
}

#define S9S_COMPARE(actual,required) \
    { \
        if (!compare(__FILE__, __LINE__, #actual, required, actual)) \
            return false; \
    }

#define S9S_VERIFY(value) \
    S9S_COMPARE(value, true)

#define PERFORM_TEST(functionName,resultvariable) \
{ \
    m_errorString.clear(); \
    if (m_argvElements.empty() || m_argvElements.contains(#functionName)) \
    { \
        setRunningTestName(#functionName); \
        if (haveTerminal()) \
            printf ("  %-24s: RUNNING ", #functionName); \
        fflush(stdout); \
        if (!functionName()) { \
            testFunctionEnded(false); \
            failed(); \
            resultvariable = false; \
            resetCounters(); \
        } else { \
            testFunctionEnded(true); \
            resetCounters(); \
            fflush(stdout); \
        } \
        setRunningTestName(""); \
    } \
}

/**
 * Base class for all the unit tests with various helper functions and macros.
 */
class S9sUnitTest
{
    public:
        S9sUnitTest();
        virtual ~S9sUnitTest();
       
        int execute(int argc, char *argv[]);

        void failed();
        int failedCounter() const;

        int nChecks() const;
        void resetCounters();
        bool haveTerminal();

        virtual void printHelp();
        virtual void printHelpAndExit (int exitCode);

        S9sString testName() const;
        void setTestName(const S9sString &name);
        S9sString testCase() const;
        void setTestCase(const S9sString &name);
        bool prepareToRun();
        bool finalizeRun();
        void setRunningTestName(const S9sString name);
        time_t testFunctionStartTime() const;
        void message(const char *formatString, ...);
        void incrementChecks();
        void testFunctionEnded(bool success);

        virtual bool runTest(const char *testName = NULL);
        
        bool compare (
                const char *fileName,
                const int   lineNumber,
                const char *varName,
                const int   value1,
                const int   value2);

        bool compare (
                const char *fileName,
                const int   lineNumber,
                const char *varName,
                const std::string value1,
                const std::string value2);

    protected:
        S9sString               m_errorString;
        S9sString               m_argv0;
        S9sMap<S9sString, bool> m_argvElements;

    private:
        S9sString   m_testName;
        S9sString   m_commandLine;
        bool        m_verbose;
        bool        m_isEndurance;
        bool        m_isLoop;
        int         m_untilMinutes;
        char       *m_basename;
        int         m_failedCounter;
        int         m_nChecks;
        bool        m_keepDatabase;
        S9sString   m_runningTestName;
        bool        m_sendMails;
        time_t      m_testFunctionStartTime;
        S9sString   m_testCase;
        bool        m_updateExamples;
        bool        m_noHalt;
};

