#!/bin/bash

portNum=`ruby /home/pi/scripts/stand_alone_system/parse_port_num.rb`
sleep 30s
while [ 1 = 1 ];do
    autossh wada@thdr.info -M 0 -C -N -R ${portNum}:localhost:22
done
