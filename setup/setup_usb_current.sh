#!/usr/bin/env bash

sudo cp /boot/config.txt /boot/config.txt_`date +%Y%m%d_%H%M%S_backup`
sudo sh -c "echo 'max_usb_current=1' >> /boot/config.txt"

echo ""
echo "/boot/config.txt was updated with 'max_usb_current=1'."
echo "Please reboot to enable the increased USB current (and HDD mounting)."
echo ""
