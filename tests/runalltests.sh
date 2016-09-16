#! /bin/bash
STOP_ON_FAILURE="no"
stopOnCrash="yes"
nFailedTests=0
hadSignal=""

# http://www.gnu.org/software/libc/manual/html_node/Heap-Consistency-Checking.html#Heap-Consistency-Checking
# on malloc issues: 2, abort is called immediately
export MALLOC_CHECK_=2

# coredump handler is works only if the following
# are set on the test-executor system:
# ulimit -l unlimited
# ulimit -c unlimited
# echo "/tmp/coredump.%e.%p.%h.%t" > /proc/sys/kernel/core_pattern

# for mysql connection you will need to pass --sqluser --sqlpassword params to run all the tests

#
# $1: test name
# $2-$N: parameters
#
function runTest() 
{
    local retcode
    local n
    local signal_name
    TESTCASE=$1

    if [ "$STOP_ON_FAILURE" != "no" -a $nFailedTests -gt 0 ]; then
        return 0
    fi

    shift
    echo "***********************"
    echo "* ${TESTCASE}"
    echo "***********************"
    if [ -x "${TESTCASE}/${TESTCASE}" ]; then
        ${TESTCASE}/${TESTCASE} $@
    elif [ -x "tests/${TESTCASE}/${TESTCASE}" ]; then
        cd tests 
        ${TESTCASE}/${TESTCASE} $@
    fi

    retcode=$?

    if [ $retcode -ne 0 ]; then
        let nFailedTests+=1
    fi

    if [ "$retcode" -gt 128 ]; then
        n=$((${retcode}-128))
        signal_name=$(kill -l $n)
        hadSignal="true"
        echo "The '${TESTCASE}/${TESTCASE} $@' received SIG${signal_name}."
    fi

    if [ $stopOnCrash != "no" -a -e core ]; then
        echo "The '${TESTCASE}/${TESTCASE} $@' produced a core file."
        exit 1
    fi
}

rm -f core tests/core

runTest ut_library $@
runTest ut_s9sstring $@
runTest ut_s9snode $@
runTest ut_s9svariant $@
runTest ut_s9svariantmap $@
runTest ut_s9sregexp $@
runTest ut_s9soptions $@
runTest ut_s9srpcclient $@
runTest ut_s9sconfigfile $@

echo
echo
if [ "$nFailedTests" -eq 0 ]; then
    echo "PASSED: all unit tests"
else
    echo "FAILED: $nFailedTests unit tests"

    if [ "$hadSignal" ]; then
        IFS='
        '
        # find the coredump files generated in last 30mins
        for CORE in $(find /tmp -maxdepth 1 -mmin -30 -name core*); do
            # make sure we can read the coredump
            sudo -n chmod a+r ${CORE}
            # this works nicely for GDB 7.9 but not for 7.4 :-S
            #EXE=$(gdb -q --core=${CORE} -ex 'info proc exe' \
            #      -ex quit 2>&1 | grep exe | cut -d= -f2 | tr -d \')
            EXE=$(gdb -q --core=${CORE} -ex quit 2>&1 | \
                  grep 'Core was generated' | \
                  cut -d'`' -f2 | cut -d"'" -f1 | tr -d \')
            echo "*"
            echo "* Executable: ${EXE}"
            echo "* CoreDump  : ${CORE}"
            echo "*"
            if [ "${EXE}x" != "x" ]; then
              gdb -q --core=${CORE} -ex 'thread apply all back' \
                -ex quit --args ${EXE} 2>&1 | tee -a ut_backtraces.txt
            else
              gdb -q --core=${CORE} -ex 'thread apply all back' \
                -ex quit 2>&1 | tee -a ut_backtraces.txt
            fi
        done
    fi

    # lets return with an error (to trigger the Jenkins job failure)
    exit 1
fi

