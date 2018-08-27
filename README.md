# Fox Trap

Fox Trap is a modular, wireless mesh based Command and Control infrastructure for locating mobile rogue access points, and conducting wireless red team ops in the field. Each **[Node]{https://github.com/joeminicucci/fox_trap/blob/master/fox_bot}** works by having tasks run synchronously, and then dropping into a communication mode where the mesh network exchanges information and passes it back to a **[root node]{https://github.com/joeminicucci/fox_trap/tree/master/fox_track}**. 

## Implementation

###Tasking
The current implementation scans for target BSSIDs from a list of targets, and then reports back to the root node as soon as it detects a target. The following tasks are used to accomplish this:

   * botInitializationTask : Opens the mesh comm mode
   * channelHopTask : Changes the wireless channels at specified time intervals
   * resyncTask : Sets flag to resynchronize node to the mesh at a guaranteed period
   * snifferInitializationTask : Places the node into 'sniffing' promiscuous mode and scans the air for a MAC contained in the targets list
   * sendAlertTask : Drops out of sniffing mode when a target is found and continuously reports to the mesh



## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

What things you need to install the software and how to install them

```
* [PlatformIO](https://github.com/esp8266/arduino#using-platformio)
* **OPTIONAL** [Signal-CLI](https://github.com/AsamK/signal-cli)
```

### Installing

We recommend using Atom IDE and PlatfomIO as a development environment. Assuming you have those installed, simply clone the project and open it with PlatformIO. If you want to create the project from scratch, be sure to set NodeMCU as the chipset firmware.



## Software used

* [PlatformIO](https://github.com/esp8266/arduino#using-platformio)
* [Painless Mesh](https://gitlab.com/painlessMesh/painlessMesh)
* [Arduino Task Scheduler](https://github.com/arkhipenko/TaskScheduler)
* [Ray Burnette's Wifi Sniffer](https://www.hackster.io/rayburne/esp8266-mini-sniff-f6b93a)
* [rw950431's Probe collection logic](https://github.com/rw950431/ESP8266mini-sniff)
* [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
* [Signal-CLI](https://github.com/AsamK/signal-cli)

## Authors

* **[Joe Minicucci](https://github.com/joeminiccci)** - *Software implementation and architecture*
* **[Todd Cronin](https://github.com/t0ddpar0dy)** - *Hardware and conceptual design*

## License

This project is licensed under the GNU General Public License - see the [LICENSE.gpl](LICENSE.gpl) file for details
 
## Hardware

* [ESP8266](https://www.amazon.com/gp/product/B071HCX3X7/ref=oh_aui_search_detailpage?ie=UTF8&psc=1)
* [FTDI 3.3v USB-Serial](https://www.amazon.com/SparkFun-FTDI-Basic-Breakout-3-3V/dp/B004G52QR0)
* [Breadboard, jumpers, resistors (10k), switches](https://www.amazon.com/SunFounder-Sidekick-Breadboard-Resistors-Mega2560/dp/B00DGNZ9G8)

## Acknowledgments

* Defcon Wireless Village - Thanks for holding the Defcon wireless WTF and for the inspiration to create Fox Trap
