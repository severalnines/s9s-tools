#! /bin/bash

./ft_galera.sh \
    --log \
    --os-vendor=centos \
    --os-release="8-Stream" \
    --provider-version=5.7 \
    --vendor="percona" \
    $*

