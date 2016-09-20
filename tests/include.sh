S9S=$(which s9s)
FAILED="no"
TEST_SUITE_NAME=""
TEST_NAME=""

#
# This function should be called before the functional tests are executed.
# Currently this only prints a message for the user, but this might change.
#
function startTests ()
{
    TEST_SUITE_NAME=$(basename $0 .sh)

    echo ""
    echo "***********************"
    echo "* $TEST_SUITE_NAME"
    echo "***********************"
}

#
# This function should be called when the function tests are executed. It prints
# a message about the results and exits with the exit code that is true if the
# tests are passed and false if at least one test is failed.
#
function endTests ()
{
    if isSuccess; then
        echo "SUCCESS: $(basename $0 .sh)"
        exit 0
    else
        echo "FAILURE: $(basename $0 .sh)"
        exit 1
    fi
}

#
# This is the BASH function that executes a functional test. The functional test
# itself should be implemented as a BASH function.
#
function runFunctionalTest ()
{
    TEST_NAME=$1

    if ! isSuccess; then
        printf "  %-26s: SKIPPED\n" $1
        return 1
    else
        $1
    fi

    if ! isSuccess; then
        printf "  %-26s: FAILURE\n" $1
    else 
        printf "  %-26s: SUCCESS\n" $1
    fi
}

#
# Returns true if none of the tests failed before, false if some bug was
# detected.
#
function isSuccess 
{
    if [ "$FAILED" == "no" ]; then
        return 0
    fi

    return 1
}

#
# Returns true if the --verbose option was provided.
#
function isVerbose 
{
    if [ "$VERBOSE" == "true" ]; then
        return 0
    fi

    return 1
}

#
# Prints the message passed as command line options if the test is executet in
# verbose mode (the --verbose command line option was provided).
#
function printVerbose 
{
    isVerbose && echo "$@"
}

#
# Prints a message about the failure and sets the test failed state.
#
function failure
{
    echo "$TEST_SUITE_NAME::$TEST_NAME(): $1."
    FAILED="true"
}

#
# This function will check if a core file is created and fails the test if so.
#
function checkCoreFile 
{
    for corefile in data/core /tmp/cores/*; do 
        if [ -e "$corefile" ]; then
            failure "Some core file(s) found."
            ls -lha $corefile
        fi
    done
}

