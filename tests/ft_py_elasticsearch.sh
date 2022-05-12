#!/bin/bash

# need to source all the env. Only exported variables will execute
source ./include.sh

# start tests (cleanups before test. example: clean backup folder)
startTests

# call python test (environment variables exported on include are visible for python test)
#python pys9s/ft_elasticsearch.py
python pys9s/ft_user.py

# python test should fullfilled in all required environment variables
endTests
