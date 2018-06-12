
// TODO: remove all Serial.print

#include <IPAddress.h>

#include <lwip/netif.h>
#include <netif/etharp.h>
#include <lwip/dhcp.h>

#ifdef ESP8266
#include <NetDump.h>
#include <user_interface.h>
#endif

#include "w5500-lwIP.h"

boolean Wiznet5500lwIP::begin(SPIClass &spi, const uint8_t* macAddress, uint16_t mtu)
{
    uint8_t zeros[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    if (!macAddress)
        macAddress = zeros;
        
    if (!Wiznet5500::begin(spi, macAddress))
        return false;
    _mtu = mtu;

    switch (start_with_dhclient())
    {
    case ERR_OK:
        return true;

    case ERR_IF:
        return false;

    default:
        netif_remove(&_netif);
        return false;
    }
}

/**
* @ingroup lwip_nosys
* Forwards a received packet for input processing with
* ethernet_input() or ip_input() depending on netif flags.
* Don't call directly, pass to netif_add() and call
* netif->input().
* Only works if the netif driver correctly sets
* NETIF_FLAG_ETHARP and/or NETIF_FLAG_ETHERNET flag!
*/
err_t
netif_input(struct pbuf *p, struct netif *inp)
{
#if LWIP_ETHERNET
  if (inp->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) {
    return ethernet_input(p, inp);
  }
  else
#endif /* LWIP_ETHERNET */
    return ip_input(p, inp);
}

err_t Wiznet5500lwIP::start_with_dhclient ()
{
    ip4_addr_t ip, mask, gw;
    
    ip4_addr_set_zero(&ip);
    ip4_addr_set_zero(&mask);
    ip4_addr_set_zero(&gw);
    
    _netif.hwaddr_len = sizeof _mac_address;
    memcpy(_netif.hwaddr, _mac_address, sizeof _mac_address);
    
    if (!netif_add(&_netif, &ip, &mask, &gw, this, netif_init_s, ethernet_input))// netif_input))
        return ERR_IF;

    _netif.flags |= NETIF_FLAG_UP;

    return dhcp_start(&_netif);
}

err_t Wiznet5500lwIP::linkoutput_s (netif *netif, struct pbuf *pbuf)
{
    Wiznet5500lwIP* ths = (Wiznet5500lwIP*)netif->state;
    
    /*
    if (pbuf->len != pbuf->tot_len || pbuf->next)
        Serial.println("ERRTOT\r\n");

    uint16_t len = ths->sendFrame((const uint8_t*)pbuf->payload, pbuf->len);
    */

    struct pbuf *p = pbuf;

    //if (p->len != p->tot_len)
    //  Serial.printf("Chained : p->len=%d p->tot_len=%d\r\n", p->len, p->tot_len);

    do 
    {
      uint16_t len = ths->sendFrame((const uint8_t*)p->payload, p->len);
      if (len != p->len)
      {
        Serial.printf("SendFrame() : wrong buffer length p->len=%d, returned=%d\r\n", p->len, len);
        //return ERR_MEM;
      }

      p = pbuf->next;
    } while (p);
    
    return ERR_OK;


#ifdef ESP8266
    if (phy_capture)
        phy_capture(ths->_netif.num, (const char*)pbuf->payload, pbuf->len, /*out*/1, /*success*/len == pbuf->len);
#endif
    
    //return len == pbuf->len? ERR_OK: ERR_MEM;
}

err_t Wiznet5500lwIP::netif_init_s (struct netif* netif)
{
    return ((Wiznet5500lwIP*)netif->state)->netif_init();
}

err_t Wiznet5500lwIP::netif_init ()
{
    uint8_t zeros[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    if (memcmp(_mac_address, zeros, 6) == 0)
    {
#ifdef ESP8266
        if (!wifi_get_macaddr(STATION_IF, (uint8*)zeros))
            Serial.println("can u see me?");
        // I understand this is cheating
#endif
        zeros[3] += _netif.num;
        memcpy(_mac_address, zeros, 6);
        memcpy(_netif.hwaddr, zeros, 6);
    }

    _netif.name[0] = 'e';
    _netif.name[1] = '0' + _netif.num;
    _netif.mtu = _mtu;
    _netif.chksum_flags = NETIF_CHECKSUM_ENABLE_ALL;
    _netif.flags = 
          NETIF_FLAG_ETHARP
        | NETIF_FLAG_IGMP
        | NETIF_FLAG_BROADCAST 
        | NETIF_FLAG_LINK_UP;

    _netif.output = etharp_output;
    _netif.linkoutput = linkoutput_s;
    
    return ERR_OK;
}

err_t Wiznet5500lwIP::loop ()
{
    uint16_t tot_len = readFrameSize();
    if (!tot_len)
        return ERR_OK;
    
    struct pbuf *pbuf = pbuf_alloc(PBUF_RAW, tot_len, PBUF_POOL);
    if (!pbuf)
    {
      Serial.printf("pbuf_alloc() : Error tot_len=%d\r\n", tot_len);
      discardFrame(tot_len);
      return ERR_BUF;
    }

    struct pbuf *p = pbuf;
    uint16_t l = tot_len;

    //if (p->len != p->tot_len)
    //  Serial.printf("Chained : p->len=%d p->tot_len=%d\r\n", p->len, p->tot_len);

    do 
    {
      uint16_t len = readFrameData((uint8_t*)p->payload, p->len);

      if (len != p->len)
      {
        if (len > 0)
          Serial.printf("readFrameData() : wrong buffer length p->len=%d, returned=%d\r\n", p->len, len);

        discardFrame(tot_len);
        pbuf_free(pbuf);

        return ERR_BUF;
      }

      l = l - len;
      p = p->next;
    } while (p && l > 0);
    
    err_t err = _netif.input(pbuf, &_netif);

    if (err != ERR_OK)
      pbuf_free(pbuf);

    return err;



    /*
    if (pbuf->next || pbuf->tot_len != pbuf->len)
    {
        Serial.println("CHAINED?\r\n");
        discardFrame(tot_len);
        return ERR_BUF;
    }

    uint16_t len = readFrameData((uint8_t*)pbuf->payload, tot_len);
    if (!len)
    {
        Serial.println("read error?\r\n");
        return ERR_BUF;
    }

    if (pbuf->len != tot_len)
    {
        Serial.println("pbuf too large?\r\n");
        pbuf->len = tot_len;
    }

    err_t err = _netif.input(pbuf, &_netif);

#ifdef ESP8266
    if (phy_capture)
        phy_capture(_netif.num, (const char*)pbuf->payload, tot_len, 0, err == ERR_OK);
#endif

    if (err != ERR_OK)
        pbuf_free(pbuf);

    return err;

    */
}
