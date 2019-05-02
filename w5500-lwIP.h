
#ifndef W5500LWIP_H
#define W5500LWIP_H

#include <lwip/init.h>
#include <lwip/netif.h>
#include "w5500.h"

#ifdef ESP32
#include <SPI.h>
#define SPIparam(x...) x
#else
#define SPIparam(x...)
#endif

#ifndef DEFAULT_MTU
#define DEFAULT_MTU 1500
#endif

class Wiznet5500lwIP: public Wiznet5500 {

public:

    Wiznet5500lwIP (int8_t cs = SS, int8_t intr = -1):
        Wiznet5500(cs),
        _mtu(DEFAULT_MTU),
        _default(false),
        _intrPin(intr)
    {
        memset(&_netif, 0, sizeof(_netif));
    }

    // start with dhcp client
    // default mac-address is inferred from esp8266's STA interface
    boolean begin (SPIparam(SPIClass& spi,) const uint8_t *macAddress = nullptr, uint16_t mtu = DEFAULT_MTU);

    const netif* getNetIf   () const { return &_netif; }

#if LWIP_VERSION_MAJOR == 1
    IPAddress    localIP    () const { return IPAddress(_netif.ip_addr.u_addr.ip4.addr); }
    IPAddress    subnetMask () const { return IPAddress(_netif.netmask.u_addr.ip4.addr); }
    IPAddress    gatewayIP  () const { return IPAddress(_netif.gw.u_addr.ip4.addr); }
#else
    IPAddress    localIP    () const { return IPAddress(ip4_addr_get_u32(&_netif.ip_addr)); }
    IPAddress    subnetMask () const { return IPAddress(ip4_addr_get_u32(&_netif.netmask)); }
    IPAddress    gatewayIP  () const { return IPAddress(ip4_addr_get_u32(&_netif.gw)); }
#endif

    void setDefault ();

    bool connected () { return !!ip4_addr_get_u32(&_netif.ip_addr); }

protected:

    netif _netif;
    uint16_t _mtu;
    bool _default;
    int8_t _intrPin;

    err_t start_with_dhclient ();
    err_t netif_init ();
    void  netif_status_callback ();

    static err_t netif_init_s (netif* netif);
    static err_t linkoutput_s (netif *netif, struct pbuf *p);
    static void  netif_status_callback_s (netif* netif);

    // called on a regular basis or on interrupt
    err_t handlePackets ();
};

#endif // W5500LWIP_H
