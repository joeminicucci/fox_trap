#!/bin/bash

macfile="./Mac.txt"

mac=$(cat "$macfile")

channelfile="./Channel.txt"

channel=$(cat "$channelfile")

echo $mac
echo $channel

airodump-ng -c $channel --bssid $mac -w output l
