
// TODO:
// remove all Serial.print
// unchain pbufs

#include <IPAddress.h>

#include <lwip/netif.h>
#include <lwip/etharp.h>
#include <lwip/dhcp.h>

#ifdef ESP8266
#include <user_interface.h>	// wifi_get_macaddr()
#endif

#include "w5500-lwIP.h"

boolean Wiznet5500lwIP::begin (SPIparam(SPIClass& spi,) const uint8_t* macAddress, uint16_t mtu)
{
    uint8_t zeros[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    if (!macAddress)
        macAddress = zeros;
        
    if (!Wiznet5500::begin(SPIparam(spi,) macAddress))
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

err_t Wiznet5500lwIP::start_with_dhclient ()
{
    ip4_addr_t ip, mask, gw;
    
    ip4_addr_set_zero(&ip);
    ip4_addr_set_zero(&mask);
    ip4_addr_set_zero(&gw);
    
    _netif.hwaddr_len = sizeof _mac_address;
    memcpy(_netif.hwaddr, _mac_address, sizeof _mac_address);
    
    if (!netif_add(&_netif, &ip, &mask, &gw, this, netif_init_s, ethernet_input))
        return ERR_IF;

    _netif.flags |= NETIF_FLAG_UP;

    return dhcp_start(&_netif);
}

err_t Wiznet5500lwIP::linkoutput_s (netif *netif, struct pbuf *pbuf)
{
    Wiznet5500lwIP* ths = (Wiznet5500lwIP*)netif->state;
    
#ifdef ESP8266
    if (pbuf->len != pbuf->tot_len || pbuf->next)
        Serial.println("ERRTOT\r\n");
#endif

    uint16_t len = ths->sendFrame((const uint8_t*)pbuf->payload, pbuf->len);

#if defined(ESP8266) && PHY_HAS_CAPTURE
    if (phy_capture)
        phy_capture(ths->_netif.num, (const char*)pbuf->payload, pbuf->len, /*out*/1, /*success*/len == pbuf->len);
#endif
    
    return len == pbuf->len? ERR_OK: ERR_MEM;
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

        // make a new mac-address from the esp's wifi sta one
        // I understand this is cheating with an official mac-address
        wifi_get_macaddr(STATION_IF, (uint8*)zeros);

#else

        // https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines
        // TODO api to get a mac-address from user
        // TODO ESP32: get wifi mac address like with esp8266 above
        zeros[0] = 0xEE;

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

    // lwIP's doc: This function typically first resolves the hardware
    // address, then sends the packet.  For ethernet physical layer, this is
    // usually lwIP's etharp_output()
    _netif.output = etharp_output;
    
    // lwIP's doc: This function outputs the pbuf as-is on the link medium
    // (this must points to the raw ethernet driver, meaning: us)
    _netif.linkoutput = linkoutput_s;
    
    return ERR_OK;
}

err_t Wiznet5500lwIP::loop ()
{
    uint16_t tot_len = readFrameSize();
    if (!tot_len)
        return ERR_OK;
    
    // from doc: use PBUF_RAM for TX, PBUF_POOL from RX
    // however:
    // PBUF_POOL can return chained pbuf (not in one piece)
    // and WiznetDriver does not have the proper API to deal with that
    // so in the meantime, we use PBUF_RAM instead which are currently
    // guarantying to deliver a continuous chunk of memory.
    // TODO: tweak the wiznet driver to allow copying partial chunk
    //       of received data and use PBUF_POOL.
    pbuf* pbuf = pbuf_alloc(PBUF_RAW, tot_len, PBUF_RAM);
    if (!pbuf || pbuf->len < tot_len)
    {
        if (pbuf)
            pbuf_free(pbuf);
        discardFrame(tot_len);
        return ERR_BUF;
    }

    uint16_t len = readFrameData((uint8_t*)pbuf->payload, tot_len);
    if (len != tot_len)
    {
        // tot_len is given by readFrameSize()
        // and is supposed to be honoured by readFrameData()
        // todo: ensure this test is unneeded, remove the print
        Serial.println("read error?\r\n");
        pbuf_free(pbuf);
        return ERR_BUF;
    }

    err_t err = _netif.input(pbuf, &_netif);

#if defined(ESP8266) && PHY_HAS_CAPTURE
    if (phy_capture)
        phy_capture(_netif.num, (const char*)pbuf->payload, tot_len, /*out*/0, /*success*/err == ERR_OK);
#endif

    if (err != ERR_OK)
    {
        pbuf_free(pbuf);
        return err;
    }
    // (else) allocated pbuf is now on lwIP's responsibility

    return ERR_OK;
}
