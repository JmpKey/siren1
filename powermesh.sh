#!/bin/bash

# args
if [ "$#" -ne 3 ]; then
    echo "Using: powermash.sh <name ibss> <frequency ibss> <IP/24>"
    exit 1
fi

# value
NETWORK_NAME=$1
FREQUENCY=$2
ASSIGNED_IP=$3
INTERFACE=$(iw dev | grep Interface | awk '{print $2}')

# exec
sudo systemctl stop NetworkManager
sudo ip link set $INTERFACE down
sudo iw $INTERFACE set type ibss
sudo ip link set $INTERFACE up
sudo iw $INTERFACE ibss join "$NETWORK_NAME" "$FREQUENCY"
sudo ip addr add "$ASSIGNED_IP" dev "$INTERFACE"

echo "OK"
