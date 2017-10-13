#! /bin/bash

MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
VERBOSE=""
VERSION="0.0.3"
LOG_OPTION="--wait"
S9S_CONFIG_DIR="$HOME/.s9s"

cd $MYDIR
source include.sh
            
DONT_PRINT_TEST_MESSAGES="true"
PRINT_COMMANDS="true"

#
# This one just creates 100 databases.
#
MAX=400
i=300
while [ $i -lt $MAX ];
do
    time s9s cluster --create-database --cluster-id=1 --db-name=DB$i

    retcode=$?
    if [ "$retcode" -ne 0 ]; then
        break
    fi

    i=`expr $i + 1`
done
