#!/bin/bash

detectorID=`ruby /home/pi/scripts/stand_alone_system/parse_detector_id.rb`
devicePath=/dev/sda1

if [ ! -e /media/pi/GROWTH-DATA ]; then
   mkdir -p /media/pi/GROWTH-DATA
fi

mountpoint /media/pi/GROWTH-DATA
mountVerify=$?
if [ ${mountVerify} != 0 ]; then
    sudo umount ${devicePath}
    sleep 3s
    sudo mount ${devicePath} /media/pi/GROWTH-DATA
    sleep 3s
fi

mountpoint /media/pi/GROWTH-DATA
mountVerify=$?

if [ ${mountVerify} == 0 ]; then
    echo "mounted"
    month=`date -d "1 months ago" +"%Y%m"`
    if [ ! -e /media/pi/GROWTH-DATA/data/${detectorID}/${month} ]; then
	mkdir -p /media/pi/GROWTH-DATA/data/${detectorID}/${month}
    fi
    cd /media/pi/GROWTH-DATA/data/${detectorID}/${month}
    find /home/pi/work/growth/data/${detectorID}/${month} -name '*.fits.gz' | xargs -I% mv % /media/pi/GROWTH-DATA/data/${detectorID}/${month}/
    
    month=`date +"%Y%m"`
    if [ ! -e /media/pi/GROWTH-DATA/data/${detectorID}/${month} ]; then
	mkdir -p /media/pi/GROWTH-DATA/data/${detectorID}/${month}
    fi
    cd /media/pi/GROWTH-DATA/data/${detectorID}/${month}
    find /home/pi/work/growth/data/${detectorID}/${month} -name '*.fits.gz' | xargs -I% mv % /media/pi/GROWTH-DATA/data/${detectorID}/${month}/
fi
