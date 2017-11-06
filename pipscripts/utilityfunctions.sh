#! /bin/bash

UTILITY_FUNCTIONS_VERSION="1.0.0"

#
# Various utility functions that are used (or should be used) in many scrips.
# This is a very important file, because it makes possible to re-use the code.
# If we re-use a code in multiple scripts and multiple projects we only need to
# maintain one instance and work less while achieving more.
#

#
# Prints the name of the script and its version number. This funtion gets this
# information from the global variables.
#
function printVersionAndExit()
{
    echo "$MYNAME Version $VERSION"
    echo ""
    exit 0
}

#
# Prints an error message to the standard error. The text will not mixed up with
# the data that is printed to the standard output.
#
function printError()
{
    local datestring=$(date "+%Y-%m-%d %H:%M:%S")

    echo -e "$*" >&2

    if [ "$LOGFILE" ]; then
        echo -e "$datestring ERROR $MYNAME($$) $*" >>"$LOGFILE"
    fi
}

#
# Prints all the arguments but only if the program is in the verbose mode.
#
function printVerbose()
{
    local datestring=$(date "+%Y-%m-%d %H:%M:%S")

    if [ "$VERBOSE" == "true" ]; then
        echo -e "$*" >&2
    fi

    if [ "$LOGFILE" ]; then
        echo -e "$datestring DEBUG $MYNAME($$) $*" >>"$LOGFILE"
    fi
}

#
# $1: date in any format accepted by the date(1) program and the underscore
#   format (e.g. 2016_03_20) is also supported
#
# This function will convert and print the date. If the date is valid it will be
# printed in yyyy-mm-dd format (e.g. '2016-02-16'), if it is invalid it will be
# the empty string.
#
function canonicalizeDate()
{
    local date="$1"

    if [ -z "$date" ]; then
        return
    fi

    date=$(echo "$date" | tr '_' '-')
    echo $(date "+%Y-%m-%d" -d "$date" 2>/dev/null)
    return $?
}

#
# $1: date in any format accepted by the date(1) program
#
# This function will convert and print the date. If the date is valid it will be
# printed in yyyy_mm_dd format (e.g. '2016_02_16'), if it is invalid it will be
# the empty string.
#
function date2underscore()
{
    local date="$1"

    if [ -z "$date" ]; then
        return
    fi
    
    date=$(canonicalizeDate "$date")
    if [ -z "$date" ]; then
        return
    fi

    echo $(date "+%Y_%m_%d" -d "$date" 2>/dev/null)
    return $?
}

#
# $1: UNIX time epoch
#
function epoch2date()
{
    echo `date --date @$1 "+%Y-%m-%d"`
}

#
# $1: a date string
#
function date2epoch()
{
    echo $(date --date "$1" +%s)
}

#
# $1: a date string
#
# Prints the date that is one day before the given date.
#
function previousDay()
{
    local date=$(canonicalizeDate "$1")

    if [ -z "$date" ]; then
        return 1
    fi

    date "+%Y-%m-%d" -d "$date - 1 days" 2>/dev/null
}

#
# $1: a date string
#
# Prints the date that is one day after the given date.
#
function nextDay()
{
    local date=$(canonicalizeDate "$1")

    if [ -z "$date" ]; then
        return 1
    fi

    date "+%Y-%m-%d" -d "$date + 1 days" 2>/dev/null
}

#
# Prints the age of the file in seconds, e.g. 1 means the file was modified 1
# second ago.
#
# $1: the file name
#
function fileAgeInSeconds()
{
    echo $((`date +%s` - `stat -L --format %Y $1` ))
}

#
# Very simple function to get the current time in UNIX Epoch. This format can be
# used to do calculations.
#
function epochTime()
{
    date +%s
}

#
# This function takes two times, calculates the difference and prints it in
# human readable format. So 01:02:03 means 1 hour, 2 minutes and 3 seconds. This
# comes very handy when we log how many time we needed to do a certain
# processing.
#
# $1: start date&time in epoch
# $2: end date&time in epoch
#
function elapsedTimeString()
{
    local start_date="$1"
    local end_date="$2"
    local T=$((end_date-start_date))

    printf "%02d:%02d:%02d" \
        "$((T/3600%24))" "$((T/60%60))" "$((T%60))"
}

#
# Prints the age of the file in hours, e.g. 1 means the file was modified at
# least 1 hour ago, no more than 2 hours ago.
#
# $1: the file name
#
function fileAgeInHours()
{
    local retval=$(fileAgeInSeconds "$1")
    let retval/=60
    let retval/=60

    echo $retval
}

#
# Prints the age of the file in days, e.g. 1 means the file was modified at
# least 1 day ago, no more than 2 days ago.
#
# $1: the file name
#
function fileAgeInDays()
{
    local retval=$(fileAgeInSeconds "$1")
    let retval/=60
    let retval/=60
    let retval/=24

    echo $retval
}


#
# $1: the name of the directory
#
# Not creating a directory might be dangerious, so this function exits if the
# directory could not be created. Well, if it is already there, no danger, it
# just returns.
#
function ensureHasDirectory()
{
    local dir="$1"

    if [ ! -d "$dir" ]; then
        mkdir -p "$dir" 2>/dev/null
    fi
    
    if [ ! -d "$dir" ]; then
        printError "Failed to create directory '$dir', exiting."
        exit 3
    fi

    # We always return true, if we fail we will not return at all.
    return 0
}

#
# $1: the name of the file
#
# Prints the number of lines for a file. The file might be an ASCII file of an
# ASCII file compressed with gzip having the .gz extension.
#
function countLinesOfFile() 
{
    local file="$1"
    local nlines

    if [ ! -f "$file" ]; then
	    echo 0
    	return 0
    fi

    if [ "${file: -3}" == ".gz" ]; then
        nlines=$(zcat $file | wc -l | cut -d' ' -f1)
    else
        nlines=$(cat $file | wc -l | cut -d' ' -f1)
    fi

    echo "$nlines"
    return 0
}

#
# $1: the file name
#
# This function is simply printing the file size in bytes.
#
function fileSize()
{
    local file=$1
    local size

    if [ ! -f "$file" ]; then
        echo 0
    else 
        size=$(ls -l "$file" | cut -d' ' -f5)
        echo $size
    fi
}

#
# $1: the file name
#
# This function is printing the file size in Megabytes. 
#
function filesize_mb()
{
    local file=$1
    local size

    if [ ! -f "$file" ]; then
        echo 0
    else 
        size=$(ls -l "$file" | cut -d' ' -f5)
        let size=size/1024
        let size=size/1024
        echo $size
    fi
}

#
# $1: the string to find
# $2: the array to investigate
#
function containsElement() 
{
    local element
    for element in "${@:2}"; do 
        [[ "$element" == "$1" ]] && return 0; 
    done
  
    return 1
}

#
#
# Example:
#   result=($(findGzFiles "$TMPDIR" "2015_01_13" "com net"))
#
function findGzFiles()
{    
    local input_dir="$1"
    local date="$2"
    local exceptions="$3"

    notname=""
    for exception in $exceptions ; do
	    name="$date"_$exception".csv.gz"
	    notname="$notname ! -name $name"
    done
    
    find $input_dir/ $notname -name "$date*.csv.gz" -type f
}

#
# This function prints how many child processes the script has.
#
function numberOfChildProcesses()
{
    jobs -p | wc -l
}

#
# $1: max number of processes to start, 0 if omitted
#
# This function will wait until the number of the child processes (of this BASH
# instance) goes below a maximum number.
#
function waitForProcessesToEnd()
{
    local max="$1"
    local nProc

    if [ -z "$max" ]; then
        max=0
    fi

    while true; do
        nProc=$(numberOfChildProcesses)
       
        if [ "$nProc" -eq 0 ]; then
            #echo "No childs, no need to wait."
            break;
        fi

        if [ "$nProc" -lt "$max" ]; then
            #echo "Have only $nProc, ok."
            break;
        fi

        #printVerbose "Have $nProc, processes, need to wait."
        jobs > /dev/null
        sleep 5
    done
}

#
# $1: the directory to clean
#
# This function will go through a directory jumping into the subdirectories if
# necessary and will delete all the regular files that are older than a certain
# age. We can use this function to delete log files that are too old.
#
function deleteOldLogFiles()
{
    local dir_name="$1"
    local file
    local base_name
    local age

    pushd "$dir_name" >/dev/null
    if [ ! $? ]; then
        printError "Could not chdir to '$dir_name'."
        return 1
    #else
    #    printf "Cleaning directory '$PWD':\n"
    fi

    for file in *.log; do
        if [ ! -f "$file" ]; then
            continue
        fi

        base_name=$(basename $file)
        age=$(fileAgeInDays "$file")
        #printf "%3d day(s) '%s'\n" $age $base_name

        if [ "$age" -gt 14 ]; then
            rm -f "$file"
        fi
    done
    
    for file in *; do
        if [ -d "$file" ]; then
            deleteOldLogFiles "$file"
        fi
    done

    popd >/dev/null
}

function lockFile()
{
    local id="$1"
    local mybasename=$(basename "$MYNAME" .sh)
    local lock
    
    if [ "$id" ]; then
        lock="/var/tmp/${mybasename}_${id}.pid"
    else
        lock="/var/tmp/${mybasename}.pid"
    fi

    echo "$lock"
}

#
# $1: instance ID, a unique ID that will be locked
#
# This is a not super-safe, but good enough locking mechanism using simple BASH
# to implement a lock file. Calling this function should prevent the script to
# run in two instances at the same time.
#
function checkOtherInstances()
{
    local id="$1"
    local mybasename=$(basename "$MYNAME" .sh)
    local lock

    # 
    # Every instance has to use the same lock file, so it is in a public place.
    # This needs root access, or at least write access to /var/tmp/, if that
    # becomes an issue we simply move this to the /tmp or something.
    #
    if [ "$id" ]; then
        lock="/var/tmp/${mybasename}_${id}.pid"
    else
        lock="/var/tmp/${mybasename}.pid"
    fi

    printVerbose "Using lock file '$lock'."
    if [[ -e "${lock}" ]]; then
        local pid=$(cat ${lock})
        
        if [ "$pid" -eq $$ ]; then
            # The lock file is there but it is our PID. This is not super safe
            # since the PIDs are re-used, but should work.
            printVerbose "We already have a lock, ok."
        elif [[ -e /proc/${pid} ]]; then
            # The process that created the PID file is still running, we exit.
            printError "Process ${pid} is still running, exiting."
            exit 1
        else
            # Clean up previous lock file. The creator should have done this,
            # but this auto-clean-up feature is still very handy.
            printVerbose "Process ${pid} is not running, removing lock file."
            rm -f ${lock}
        fi
    fi

    trap "rm -f ${lock}; exit $?" INT TERM EXIT
    echo "$$" > ${lock}

    if [ ! -e ${lock} ]; then
        printError "Unable to create lock file at ${lock}, giving up."
        exit 1
    fi
}

