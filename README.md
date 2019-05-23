
Ethernet driver for lwIP on esp8266
-----------------------------------

This library brings full ethernet (RJ45 cables) connectivity into an already
existing lwIP stack, such as the one in esp8266.

Every network services will work without further work
* dhcp (client only for now)
* dns
* http(s) clients+servers
* mqtt
* ... your already working network services
* no need for separate UDP/TCP api (like uIP or others, lwIP is already here)

This library does not need anymore the external W5500 driver
(was W5500 ethernet-only driver https://github.com/d-a-v/W5500MacRaw)

How to use
----------
Example soon (watch the few opened issues)

As for now, the network interface is working as DHCP client only

What's next
-----------
* allow dhcp server
* provide better network helpers (like arduino's `WiFi.status()`)
* W5100 and enc28j60 similar driver

Notes (esp8266 and w5500)
-------------------------
* the TCP/IP part of W5x00 chips is not used, the ethernet interface only is.
* Do not directly connect SPI CS(ChipSelect) to esp8266'SS(SlaveSelect) (GPIO15), use another pin (hacks using a transistor exist for using this pin).

Roadmap
-------
* driver for W5100
* integration in esp8266 arduino core (this repository will be obsolete)
* dhcp server / driver for enc28j60
* NAT (in lwip2 arduino core)

