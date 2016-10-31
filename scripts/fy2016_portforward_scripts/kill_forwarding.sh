#!/bin/bash

process=`pgrep autossh`
kill $process
