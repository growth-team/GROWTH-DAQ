#!/bin/bash

portNum=2400
sleep 30s
while [ 1 = 1 ];do
    autossh wada@thdr.info -M 0 -C -N -R ${portNum}:localhost:22
done
