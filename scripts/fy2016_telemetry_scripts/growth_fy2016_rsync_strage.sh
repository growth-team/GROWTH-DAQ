#!/bin/bash

<<<<<<< HEAD
=======
umount /dev/sda1
sleep 5s
mount /dev/sda1 /media/pi/Transcend
sleep 5s
>>>>>>> e05a20ace979e1fcaaea3fd7851560b4e6f0d349
rsync -avr /home/pi/work/growth /media/pi/Transcend/
