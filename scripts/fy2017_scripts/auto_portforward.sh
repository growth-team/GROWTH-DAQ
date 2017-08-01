#!/bin/bash

port=2023

sleep 180s
while [ 1 = 1 ];do
    autossh wada@thdr.info -M 0 -o "ServerAliveInterval 30" -C -N -f -R ${port}:localhost:22
    sleep 900s
    process=`pgrep autossh`
    kill $process
done
