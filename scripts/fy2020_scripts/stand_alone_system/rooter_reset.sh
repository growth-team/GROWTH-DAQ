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
echo "rooter reset" >> ${logPath}/growth_network_reset.log.${date}
casperjs /home/pi/scripts/scraping_raspi/reset_mobile_aterm.js
