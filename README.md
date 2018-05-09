
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

This library needs
* W5500 ethernet-only driver https://github.com/d-a-v/W5500MacRaw
  (which is a fork from https://github.com/njh/W5500MacRaw, PR is pending)

How to use
----------
* esp8266/arduino (with lwip2)
* declare 'Wiznet5500lwIP ether(CSPIN)`
* call `ether.begin()` in arduino's `setup()`
* call `ether.loop()` in arduino's `loop()`

As for now, the network interface is working as DHCP client only

What's next
-----------
* allow dhcp server
* provide better network helpers (like arduino's `WiFi.status()`)
* enc28j60 similar driver

Help needed
-----------
Tested only with w5500, not on other w5x00 chips

Notes (esp8266 and w5500)
-------------------------
* the TCP/IP part of W5x00 chips is not used, the ethernet interface only is.
* Do not directly connect SPI CS(ChipSelect) to esp8266'SS(SlaveSelect) (GPIO15), use another pin (hacks using a transistor exist for using this pin).

Notes
----
* This arduino library is not specific to esp8266, it can work with other uC working with lwIP
