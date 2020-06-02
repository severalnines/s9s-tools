#! /bin/bash

./ft_galera.sh --provider-version=5.7 --vendor="percona" --rw-port=4406 --ro-port=4407 $*
