#!/bin/bash

# need to source all the env. Only exported variables will execute
source ./include.sh

# start tests (cleanups before test. example: clean backup folder)
startTests
reset_config
grant_user

# call python test (environment variables exported on include are visible for python test)
rm .pys9s_results_env
export USE_FT_FULL="YES"
python pys9s/ft_user.py
# python test should fulfilled in all required environment variables on .pys9s_results_env
export $(grep -v '^#' .pys9s_results_env | xargs)

endTests
