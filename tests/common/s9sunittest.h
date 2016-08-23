/*
 * Copyright (C) 2011-2016 severalnines.com
 */
#pragma once

#include "S9sString"

/**
 * Base class for all the unit tests with various helper functions and macros.
 */
class S9sUnitTest
{
    public:
        S9sUnitTest();
        virtual ~S9sUnitTest();
        
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
        bool runTest(const char *testName);

        bool compare (
                const char *fileName,
                const int   lineNumber,
                const char *varName,
                const int   value1,
                const int   value2);

    protected:
        S9sString   m_errorString;

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

