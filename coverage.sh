#! /bin/bash

MYNAME=$(readlink -m $0)
MYDIR=$(dirname $MYNAME)
MYNAME=$(basename $MYNAME)

TESTARGS=""
NOCLEAN="no"
NOCONFIG="no"
TESTNAME=""

function printHelpAndExit
{
    cat <<EOF
Usage: $MYNAME [OPTION]...

  $MYNAME - Creates coverage information report for the project.

  -h, --help         Print help and exit.
  --noclean          Do not execute make clean/distclean.
  --noconfig         Do not execute autogen/configure.
  --regen            Just call lcov to regenerate the report.
  --test=TEST        Set the name of the unit test to perform.
  --sqlpassword=pwd  The SQL root password used for testing.

EOF
    exit 1
}

#
# Processing command line options.
#
ARGS=$(getopt \
    -o h \
    -l "help,noclean,noconfig,regen,test:,sqlpassword:" \
    -n "$MYNAME" -- "$@")

function execLcov
{
    #
    # Creating the report.
    #
    lcov \
        --capture \
        --directory . \
        --directory libs9s/ \
        --output-file coverage.info && \
      lcov --remove coverage.info \
        '/opt/*' '/usr/include/*' '*tests/*' \
        --output-file $MYDIR/coverage_filtered.info && \
      genhtml $MYDIR/coverage_filtered.info --output-directory $MYDIR/coverage

    #firefox $MYDIR/coverage/src/index.html
}

eval set -- "$ARGS"
while true; do
    case "$1" in
        -h|--help)
            shift
            printHelpAndExit
            ;;

        --noclean)
            NOCLEAN="true"
            shift
            ;;

        --noconfig)
            NOCONFIG="true"
            shift
            ;;

        --test)
            shift
            TESTNAME="$1"
            shift
            ;;

        --regen)
            execLcov
            exit 0
            ;;

        --sqlpassword)
            shift
            TESTARGS="$TESTARGS --sqlpassword=$1"
            shift
            ;;

        --)
            shift
            break
            ;;
    esac
done

if [ "$TESTNAME" ]; then
    for testname in $(echo "$TESTNAME" | tr ',' ' '); do
        if [ -x "$testname" ]; then
            # There is perhaps a unit test with this name...
            echo "Unit test with the name ${testname} found."
        else
            echo "No unit test with the name ${testname} found."
            exit 1
        fi
    done
fi

if [ "$NOCLEAN" == "no" ]; then
    make clean || true
    make distclean || true
    rm -fv */*.gcda */*.gcno */*/*.gcda */*/*.gcno
fi

if [ "$NOCONFIG" == "no" ]; then
    ./autogen.sh --enable-gcov
    if [ $? -ne 0 ]; then
        exit 1
    fi
fi

make -j10
if [ $? -ne 0 ]; then
    exit 1
fi

#
# Running the tests to create coverage data.
#
if [ "$TESTNAME" ]; then
    for testname in $(echo "$TESTNAME" | tr ',' ' '); do
        $testname/$testname $TESTARGS
    done
else
    tests/runalltests.sh $TESTARGS
fi

execLcov

