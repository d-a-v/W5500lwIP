#ifndef W5500LWIP_H
#define W5500LWIP_H

#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include "w5500.h"

#ifdef ESP32
#include <SPI.h>
#define SPIparam(x...) x
#else
#define SPIparam(x...)
#endif

template< class T > class Wiznet5500lwIPQueue
{
public:
  Wiznet5500lwIPQueue(int = 10);
  ~Wiznet5500lwIPQueue()
  {
    delete[] values;
  }
  bool enQueue(T);
  T deQueue();
  bool isEmpty();
  bool isFull();
private:
  SemaphoreHandle_t m;
  int size;
  T *values;
  int front;
  int back;
};

class Wiznet5500lwIP: public Wiznet5500 {

public:

    Wiznet5500lwIP (int8_t cs=SS): Wiznet5500(cs)
    {
    }

    // start with dhcp client
    // default mac-address is inferred(+modified) from esp8266's STA one
    boolean begin (SPIparam(SPIClass& spi,) const uint8_t *macAddress = NULL, uint16_t mtu = 1500);

    const netif* getNetIf   () const { return &_netif; }
    IPAddress    localIP    () const { return IPAddress(_netif.ip_addr.u_addr.ip4.addr); }
    IPAddress    subnetMask () const { return IPAddress(_netif.netmask.u_addr.ip4.addr); }
    IPAddress    gatewayIP  () const { return IPAddress(_netif.gw.u_addr.ip4.addr); }

    void lwiptcpip_callback();
    err_t loop();

protected:
    netif _netif;
    uint16_t _mtu;
    
    err_t start_with_dhclient ();
    err_t netif_init ();

    static err_t netif_init_s (netif* netif);
    static err_t linkoutput_s (netif *netif, struct pbuf *p);

    tcpip_callback_msg *_callbackmsg;
    Wiznet5500lwIPQueue<struct pbuf*> _pbufqueue;
};

#endif // W5500LWIP_H
