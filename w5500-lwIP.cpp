#include <IPAddress.h>

#include <lwip/netif.h>
#include <netif/etharp.h>
#include <lwip/dhcp.h>

#ifdef ESP8266
#include <user_interface.h>	// wifi_get_macaddr()
#endif

#include "w5500-lwIP.h"

template< class T > Wiznet5500lwIPQueue<T>::Wiznet5500lwIPQueue(int x) :
  size(x),//ctor
  values(new T[size]),
  front(0),
  back(0)
{
  m = xSemaphoreCreateMutex();
}

template< class T > bool Wiznet5500lwIPQueue<T>::isFull()
{
  if ((back + 1) % size == front)
    return 1;
  else
    return 0;
}

template< class T > bool Wiznet5500lwIPQueue<T>::enQueue(T x)
{
  bool b = 0;

  xSemaphoreTake(m, portMAX_DELAY);

  if (!Wiznet5500lwIPQueue<T>::isFull())
  {
    values[back] = x;
    back = (back + 1) % size;
    b = 1;
  }

  xSemaphoreGive(m);

  return b;
}

template< class T > bool Wiznet5500lwIPQueue<T>::isEmpty()
{
  if (back == front)//is empty
    return 1;
  else
    return 0; //is not empty
}

template< class T > T Wiznet5500lwIPQueue<T>::deQueue()
{
  T val = NULL;

  xSemaphoreTake(m, portMAX_DELAY);

  if (!Wiznet5500lwIPQueue<T>::isEmpty())
  {
    val = values[front];
    front = (front + 1) % size;
  }
  else
  {
    // Queue is empty
  }

  xSemaphoreGive(m);

  return val;

}

static void _startThread(void *ctx)
{
  while (42)
  {
    ((Wiznet5500lwIP*)ctx)->loop();
    //delay(1);
  }
}

static void _tcpcallback(void *ctx)
{
  Wiznet5500lwIP* ths = (Wiznet5500lwIP*)ctx;
  ths->lwiptcpip_callback();
}


template class Wiznet5500lwIPQueue<struct pbuf*>;
static SemaphoreHandle_t _W5500SPIMutex = xSemaphoreCreateMutex();


void Wiznet5500lwIP::lwiptcpip_callback()
{
  pbuf* p = _pbufqueue.deQueue();
  if (p == NULL)
    return;

  err_t err = _netif.input(p, &_netif);

  if (err != ERR_OK)
  {
    ESP_LOGW("w5500-lwip", "_netif.input (ret=%d)", err);
    pbuf_free(p);
    return;
  }
}

boolean Wiznet5500lwIP::begin (SPIparam(SPIClass& spi,) const uint8_t* macAddress, uint16_t mtu)
{
    uint8_t zeros[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    if (!macAddress)
        macAddress = zeros;

    ESP_LOGI("w5500-lwip", "NO_SYS=%d", NO_SYS);
        
    if (!Wiznet5500::begin(SPIparam(spi,) macAddress))
        return false;
    _mtu = mtu;

    switch (start_with_dhclient())
    {
    case ERR_OK:
      xTaskCreatePinnedToCore(
        _startThread,   // Function to implement the task
        "w5500-lwip",   // Name of the task
        10000,          // Stack size in words
        this,           // Task input parameter
        2,              // Priority of the task (higher priority task cause crash. I dont know why :/)
        NULL,           // Task handle.
        0);             // Core where the task should run
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

    _callbackmsg = tcpip_callbackmsg_new(_tcpcallback, this);
    
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
      ESP_LOGW("w5500-lwip", "Err tot_len ?");
#endif

    xSemaphoreTake(_W5500SPIMutex, portMAX_DELAY);
    uint16_t len = ths->sendFrame((const uint8_t*)pbuf->payload, pbuf->len);
    xSemaphoreGive(_W5500SPIMutex);

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
    //_netif.chksum_flags = NETIF_CHECKSUM_ENABLE_ALL;
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
  tcpip_trycallback(_callbackmsg);

  xSemaphoreTake(_W5500SPIMutex, portMAX_DELAY);
  uint16_t eth_data_count = readFrameSize();
  xSemaphoreGive(_W5500SPIMutex);
  if (!eth_data_count)
    return ERR_OK;

  /* Allocate pbuf from pool (avoid using heap in interrupts) */
  struct pbuf* p = pbuf_alloc(PBUF_RAW, eth_data_count, PBUF_POOL);
  if (!p || p->len < eth_data_count)
  {
    ESP_LOGW("w5500-lwip", "pbuf_alloc (pbuf=%p, len=%d)", p, (p) ? p->len : 0);

    if (p) pbuf_free(p);
    //discardFrame(eth_data_count);
    return ERR_BUF;
  }

  /* Copy ethernet frame into pbuf */
  xSemaphoreTake(_W5500SPIMutex, portMAX_DELAY);
  uint16_t len = readFrameData((uint8_t*)p->payload, eth_data_count);
  xSemaphoreGive(_W5500SPIMutex);
  if (len != eth_data_count)
  {
    // eth_data_count is given by readFrameSize()
    // and is supposed to be honoured by readFrameData()
    // todo: ensure this test is unneeded, remove the print
    //ESP_LOGW("w5500-lwip", "readFrameData (tot_len=%d, len=%d)", eth_data_count, len);
    pbuf_free(p);
    return ERR_BUF;
  }

  /* Put in a queue which is processed in lwip thread loop */
  if (!_pbufqueue.enQueue(p)) {
    /* queue is full -> packet loss */
    ESP_LOGW("w5500-lwip", "pbuf queue is full. Discard pbuf");
    pbuf_free(p);
    return ERR_BUF;
  }

  return ERR_OK;
}
