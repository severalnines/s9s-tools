#! /bin/bash

while true; do
    date=$(date "+%H:%M:%S")
    output=$($*)
    clear
    echo "$date    Command: $*"
    echo -en "$output"
    sleep 3
done

