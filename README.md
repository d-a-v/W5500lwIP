
This repository has been [merged into esp8266/Arduino](https://github.com/esp8266/Arduino/pull/6680) and is now obsolete.
===================================

You can use the git version of esp8266/Arduino or the [unofficial esp8266/Arduino snapshot release](https://d-a-v.github.io/esp8266/Arduino) regularly or on-request rebuilded.

For reference:


Ethernet drivers for lwIP on esp8266
------------------------------------

This library brings full ethernet (RJ45 cables) connectivity into an already
existing lwIP stack, such as the one in esp8266.

Every network services should work
* dhcp (client only for now)
* dns
* http(s) clients+servers
* mqtt
* ... your already working network services
* no need for separate UDP/TCP api (like uIP or others, lwIP is already here)

W5100, W5500 and enc28j60 drivers are slightly modified versions of

https://github.com/njh/W5100MacRaw

https://github.com/njh/W5500MacRaw

https://github.com/njh/EtherSia (enc28j60)

How to use
----------
Example soon (watch the few opened issues)

As for now, the network interface is working as DHCP client only

What's next
-----------
* allow dhcp server
* provide better network helpers (like arduino's `WiFi.status()`)

Notes (esp8266 and w5500, w5100?)
---------------------------------
* the TCP/IP part of W5x00 chips is not used, the ethernet interface only is.
* Do not directly connect SPI CS(ChipSelect) to esp8266'SS(SlaveSelect) (GPIO15), use another pin (hacks using a transistor exist for using this pin).

Roadmap
-------
* integration in esp8266 arduino core (this repository will be obsoleted)
* dhcp server / driver for enc28j60
* ~NAT (esp8266/arduino core (lwip2))~ (done)

