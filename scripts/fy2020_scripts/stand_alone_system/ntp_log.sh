#!/bin/bash

logPath=/var/log/growth
date=`date +"%Y%m%d"`

if [  -e ${logPath} ]; then
  date >> ${logPath}/growth_ntp.log.${date}
  ntpq -p >> ${logPath}/growth_ntp.log.${date}
fi
