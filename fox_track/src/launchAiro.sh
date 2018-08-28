#!/bin/bash

macfile="./Mac"

mac=$(cat "$macfile")

channelfile="./Channel"

channel=$(cat "$channelfile")

echo $mac
echo $channel

airodump-ng -c $channel --bssid $mac -w output l
