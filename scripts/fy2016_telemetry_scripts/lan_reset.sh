#!/bin/bash

ping -c 1 thdr.info >> /dev/null
if [ $? == 0 ];then
    echo "connected"
    exit 0
fi
sleep 5s
ping -c 1 thdr.info >> /dev/null
if [ $? == 0 ];then
    echo "connected"
    exit 0
fi
echo "reset"
sudo /sbin/dhclient -r eth0 && sudo /sbin/dhclient eth0
