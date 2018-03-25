# DEF CON 26, Woreless CTF BSSID sensor network

## Background

DEF CON features a Wireless Village with a capture the flag (CTF) 
competition. 
This CTF offers Bluetooth, Traditional RF, and WiFi challenges. 
Each category contains a radio direction finding (RDF) challenge.
The WiFi variant comes in two flavors: Find the Fox, and Hide & Seek.

Find the Fox requires contestants to locate a wireless access point (AP) 
who's location constantly changes. Typically, this requires a human to 
carry an AP as the meander about the conference property (2017, Caesars 
Palace).

Hide & Seek follows the same principles as FtF, but is stationary, 
hidden someplace on the conference property. 

## Purpose

This software controls an ESP8266 and allows competitors to deploy a 
network of wireless sensors in the 2.4GHz range that report back to a 
central node with details about 
802.11 signals they receive.

## Concept

![alt text](https://bitbucket.com/m1n1/dc719_wctf/src/master/img/esp8266.jpg)
![alt text](https://bitbucket.com/m1n1/dc719_wctf/src/master/img/map.png)

## TODO:

- Determine best way to handle distributed comms, i.e. set up in chain, unicast messages, synchronus, a-sync, etc.
- List limitations, i.e. power, storage, processor load, spatial limitations (steel, stone, skin)
- Plan deployment strategy, i.e. when to deploy, where, what...
- Integrate GPS
- Make every DC719 member a sensor
- Hand out sensors to DC26 attendees?
- RE DC badge to act as sensor?
- Drink



