#! /bin/bash
MYNAME=$(basename $0)
MYBASENAME=$(basename $0 .sh)
MYDIR=$(dirname $0)
STDOUT_FILE=ft_errors_stdout
VERBOSE=""
VERSION="1.0.0"

function printError()
{
    echo "$*"
}

CLUSTER_NAME="$1"

if [ -z "$CLUSTER_NAME" ]; then
    printError "Cluster name is not provided."
    exit 6
fi

for file in scripts/cluster_scripts/*.js; do
    basename=$(basename $file)

    s9s tree --mkdir --batch /$CLUSTER_NAME/scripts
    s9s tree --touch --batch /$CLUSTER_NAME/scripts/$basename
    cat $file | s9s tree --save --batch /$CLUSTER_NAME/scripts/$basename
done

