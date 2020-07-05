#!/bin/bash

ping -c 1 www.google.co.jp >> /dev/null
if [ $? == 0 ];then
    echo "connected"
    exit 0
fi
sleep 5s
ping -c 1 www.google.co.jp >> /dev/null
if [ $? == 0 ];then
    echo "connected"
    exit 0
fi
logPath=/var/log/growth
date=`date +"%Y%m%d"`

date >> ${logPath}/growth_network_reset.log.${date}
echo "LAN device reset" >> ${logPath}/growth_network_reset.log.${date}
sudo /sbin/dhclient -r eth0
sleep 3s
/sbin/ifdown wlan0
sleep 5s
/sbin/ifup wlan0s
sleep 5s
sudo /sbin/dhclient eth0
