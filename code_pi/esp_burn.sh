#!/bin/bash
#========================================================================================
#
#   Burn ESP using Raspberry Pi
#
#   This script calls esptool to upload the firmware to the ESP8266.
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License
#   version 3 as published by the Free Software Foundation.
#
#   Dependencies:
#     pip3 install esptool
#     sudo apt install -y jq
#     add "enable_uart=1" to /boot/config.txt 
#     disable console on serial throught raspi-config
#
#   Use: manual ./esp_burn.sh firmware_name.bin
#   
#   Permission: sudo chmod +x esp_burn.sh
#
#   Version: 2.0 - 09/2023
#   Author: ArgusR <argusr@me.com>
#
#========================================================================================

# Status file used to tell the system that the uC (micro-controller) is receiving update.
DB_MGMT="/home/pi/db/mgmt.json"

tmp=$(mktemp)
chmod =rw "$tmp"
jq '.uC_burning = "True" ' $DB_MGMT > "$tmp"
mv "$tmp" $DB_MGMT


# Error message in case of missing argument
if [ "$1" == "" ]; then echo Missing firmware
 exit 1;
else
 # Stop scripts that communicate with the uC
 sudo pkill -f serial_monitor &
 sudo pkill -f serial_api &
 # Put the uC in upload mode
 /home/pi/./esp_reset flash
 # if ends in .bin use full argument
 ext=${1:${#1}-4}
 if [ "$ext" == ".bin" ]; then
   esptool.py --port /dev/serial0 write_flash -fm dio 0x00000 $1
 # otherwise add the .bin
 else
   esptool.py --port /dev/serial0 write_flash -fm dio 0x00000 $1.bin
 fi
fi

# Clean burning flag from status file
jq '.uC_burning = "False" ' $DB_MGMT > "$tmp"
mv "$tmp" $DB_MGMT

sleep 1

# Reset ESP
/home/pi/./esp_reset pulse
