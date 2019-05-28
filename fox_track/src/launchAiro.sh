#!/bin/bash

macfile="./Mac"

mac=$(cat "$macfile")

channelfile="./Channel"

channel=$(cat "$channelfile")

echo $mac
echo $channel

/usr/sbin/airodump-ng -c $channel --bssid $mac -w output wlan0mon
