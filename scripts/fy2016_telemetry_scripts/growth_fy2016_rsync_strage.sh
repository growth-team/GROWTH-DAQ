#!/bin/bash

umount /dev/sda1
sleep 5s
mount /dev/sda1 /media/pi/Transcend
sleep 5s
rsync -avr /home/pi/work/growth /media/pi/Transcend/
