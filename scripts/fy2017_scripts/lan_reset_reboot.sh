#!/bin/bash

ping -c 1 thdr.info >> /dev/null
if [ $? == 0 ];then
    exit 0
fi
sleep 5s
ping -c 1 thdr.info >> /dev/null
if [ $? == 0 ];then
    exit 0
fi
date
echo "reset"
#sudo /sbin/dhclient -r eth0 && sudo /sbin/dhclient eth0
sudo /sbin/dhclient -r wlan0 && sudo /sbin/dhclient wlan0
sleep 120
ping -c 1 thdr.info >> /dev/null
if [ $? == 0 ];then
    exit 0
fi
sudo reboot
