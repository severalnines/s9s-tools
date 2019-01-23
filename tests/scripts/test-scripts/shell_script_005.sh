#! /bin/bash

#!/bin/bash
function signal_handler() 
{
    echo "Signal received"
    exit 0
}

trap signal_handler SIGINT SIGTERM

echo "Going to sleep for 60s."
sleep 60


