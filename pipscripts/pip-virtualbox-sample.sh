#! /bin/bash
MYNAME=$(basename $0)
MYDIR=$(dirname $0)
MYDIR=$(readlink -m "$MYDIR")
VERSION="0.0.1"
VERBOSE=""
LOGFILE=""
SERVER=""

function get_vm_os()
{
    local uuid="$1"

    VBoxManage showvminfo $uuid | grep "^Guest OS:" | awk -F: '{print $2}'
}

#VBoxManage list runningvms --long
UUIDS=$(\
    VBoxManage list runningvms --long | \
    grep "UUID:            " | \
    awk 'BEGIN{FS="UUID:            "}{print $2}')

for uuid in $UUIDS; do
    vm=$(VBoxManage showvminfo $uuid | sed -e'/^USB Device Filters:/,$ d' | grep "Name:"  | awk 'BEGIN{FS="Name:            "}{print $2}')
    os=$(get_vm_os $uuid)

    echo "    uuid : '$uuid'"
    echo "      vm : '$vm'"
    echo "      os : '$os'"
    echo " command : VBoxManage showvminfo $uuid"
    VBoxManage controlvm "$vm" screenshotpng ${vm}.png
done
