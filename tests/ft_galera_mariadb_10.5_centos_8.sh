#! /bin/bash

./ft_galera.sh              \
    --os-vendor     "centos" \
    --os-release     8      \
    --vendor=mariadb        \
    --provider-version=10.5 \
    $*
