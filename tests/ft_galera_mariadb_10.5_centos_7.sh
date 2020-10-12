#! /bin/bash

./ft_galera.sh              \
    --os-vendor     "centos" \
    --os-release     7      \
    --vendor=mariadb        \
    --provider-version=10.5 \
    $*
