
#ifndef W5500LWIP_H
#define W5500LWIP_H

#include <w5500.h>
#include <lwip/netif.h>

class Wiznet5500lwIP: public Wiznet5500 {

public:

    Wiznet5500lwIP (int8_t cs=SS): Wiznet5500(cs) { }

    // start with dhcp client
    // default mac-address is inferred(+modified) from esp8266's STA one
    boolean begin (const uint8_t *macAddress = NULL, uint16_t mtu = 1500);
    
    // to be called regularly
    err_t loop ();

    const netif* getNetIf   () const { return &_netif; }
    IPAddress    localIP    () const { return IPAddress(ip4_addr_get_u32(&_netif.ip_addr)); }
    IPAddress    subnetMask () const { return IPAddress(ip4_addr_get_u32(&_netif.netmask)); }
    IPAddress    gatewayIP  () const { return IPAddress(ip4_addr_get_u32(&_netif.gw)); }

protected:

    netif _netif;
    uint16_t _mtu;
    
    err_t start_with_dhclient ();
    err_t netif_init ();

    static err_t netif_init_s (netif* netif);
    static err_t linkoutput_s (netif *netif, struct pbuf *p);
};

#endif // W5500LWIP_H
