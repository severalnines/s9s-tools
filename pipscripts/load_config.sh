#! /bin/bash

if [ -f "/etc/s9s-cmon-test/project.conf" ]; then
    source /etc/s9s-cmon-test/project.conf
else
    echo "File '/etc/s9s-cmon-test/project.conf' was not found." >&2
    exit 5
fi



if [ -z "$PROJECT_DATA_ROOT" ]; then
cat <<EOF
Environment variable PROJECT_DATA_ROOT is empty.
Please specify it in /etc/s9s-cmon-test/project.conf file.

This should be the directory where test schedules, results,
logs, worker host statuses are kept. Should be acessible
by the web server and the test scripts as well.
EOF
    exit -1
fi

PROJECT_LOGDIR="${PROJECT_DATA_ROOT}/log"
PROJECT_TEST_REPORT_DIR="${PROJECT_DATA_ROOT}/test-results"
PROJECT_HOST_STAT_DIR="${PROJECT_DATA_ROOT}/servers"




if [ "${PROJECT_CC_WORKER_DIR}" = "" ]; then
    cat <<EOF
Environment variable PROJECT_CC_WORKER_DIR is empty.
Please specify it in /etc/s9s-cmon-test/project.conf file.

This should be the git repo directory where new cmon to test
is built. It might be reseted and special commit checked out.
Use PROJECT_AUTO_PULL_WORKER=false to avoid unwanted loss of
work in progress.
EOF
    exit -1
fi



if [ "${PROJECT_S9S_WORKER_DIR}" = "" ]; then
    cat <<EOF
Environment variable PROJECT_S9S_WORKER_DIR is empty.
Please specify it in /etc/s9s-cmon-test/project.conf file.

This should be the git repo directory where new s9s cli
to test is built. It might be reseted and special commit
checked out.

Use PROJECT_AUTO_PULL_TESTORIGIN=false to avoid unwanted
loss of work in progress.
EOF
    exit -1
fi



if [ "${PROJECT_AUTO_PULL_WORKER}" = "" ]; then
    cat <<EOF
Environment variable PROJECT_AUTO_PULL_WORKER is empty.
Please specify it in /etc/s9s-cmon-test/project.conf file.

If set to true, then 'git reset' and 'git pull' will be
executed before starting a test in these directories:
PROJECT_CC_WORKER_DIR
PROJECT_S9S_WORKER_DIR
EOF
    exit -1
fi



if [ "${PROJECT_CC_TESTORIGIN_DIR}" = "" ]; then
    cat <<EOF
Environment variable PROJECT_CC_TESTORIGIN_DIR is empty.
Please specify it in /etc/s9s-cmon-test/project.conf file.

This should be the git repo directory from where the test
scripts should be executed.

Use PROJECT_AUTO_PULL_TESTORIGIN=false to avoid unwanted
loss of work in progress.
EOF
    exit -1
fi



if [ "${PROJECT_S9S_TESTORIGIN_DIR}" = "" ]; then
    cat <<EOF
Environment variable PROJECT_S9S_TESTORIGIN_DIR is empty.
Please specify it in /etc/s9s-cmon-test/project.conf file.

This should be the git repo directory from where the test
scripts should be executed.

Use PROJECT_AUTO_PULL_TESTORIGIN=false to avoid unwanted
loss of work in progress.
EOF
    exit -1
fi



if [ "${PROJECT_AUTO_PULL_TESTORIGIN}" = "" ]; then
    cat <<EOF
Environment variable PROJECT_AUTO_PULL_TESTORIGIN is empty.
Please specify it in /etc/s9s-cmon-test/project.conf file.

If set to true, then 'git reset' and 'git pull' will be
executed before starting a test in these directories:
PROJECT_CC_TESTORIGIN_DIR
PROJECT_S9S_TESTORIGIN_DIR
EOF
    exit -1
fi



PROJECT_SCRIPT_ROOT=${PROJECT_CC_TESTORIGIN_DIR}/tests/scripts/test_ui

if [ "${PROJECT_OWNER}" = "" ]; then
    PROJECT_OWNER="$USER"
fi

if [ "${}" = "" ]; then
    S9STEST_ADMIN_USER="admin_${PROJECT_OWNER}"
fi
if [ "${}" = "" ]; then
    S9STEST_ADMIN_USER_PASSWORD="adminpwd"
fi
if [ "${}" = "" ]; then
    S9STEST_USER="${PROJECT_OWNER}"
fi
if [ "${}" = "" ]; then
    S9STEST_USER_PASSWORD="pwd"
fi



