// 2012-2013 by Fabrizio Di Vittorio (fdivitto@tiscali.it)


#ifndef FDV_TCPIP_H_
#define FDV_TCPIP_H_


#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdarg.h>

#include "../fdv_generic/fdv_random.h"
#include "../fdv_generic/fdv_utility.h"
#include "../fdv_generic/fdv_memory.h"



#pragma GCC diagnostic ignored "-Wnarrowing"


namespace fdv
{


  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // a list of linked buffers
  
  struct DataList
  {
    DataList const* next; // NULL = no next
    void const*     data;
    uint16_t        length;

    DataList(DataList const* next_, void const* data_, uint16_t length_)
      : next(next_), data(data_), length(length_)
    {
    }

    uint16_t calcLength() const
    {
      uint16_t r = 0;
      DataList const* curr = this;
      while ( curr != NULL )
      {
        r += curr->length;
        curr = curr->next;
      }
      return r;
    }

    struct Enumerator
    {
      DataList const* next;
      uint8_t const*  data;
      uint16_t        length;
      explicit Enumerator(DataList const& datalist)
      {
        next = datalist.next; data = (uint8_t const*)datalist.data; length = datalist.length;
      }
      void moveNext() // doesn't check end of data (you have to count using calcLength)
      {
        if (length <= 1 && next != NULL)
        {
          data   = (uint8_t const*)next->data;
          length = next->length;
          next   = next->next;            
        }
        else
        {
          ++data;
          --length;
        }
      }
    };
        
    uint16_t calcInternetChecksum()
    {
      Enumerator enu(*this);
      uint16_t nbytes = calcLength();
      uint32_t sum = 0;
      while (nbytes > 1)
      {
        uint8_t lo = *enu.data; enu.moveNext();
        uint8_t hi = *enu.data; enu.moveNext();
        sum += (uint16_t)lo << 8 | hi;
        nbytes -= 2;
      }
      if (nbytes == 1)
      {
        uint8_t lo = *enu.data; enu.moveNext();
        sum += (uint16_t)lo << 8;
      }
      sum = (sum >> 16) + (sum & 0xffff);
      sum += (sum >> 16);
      return (uint16_t)~sum;      
    }

  };




  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // LinkAddress (a ethernet address)
  
  struct LinkAddress
  {
    
    LinkAddress()
    {
      memset(&value[0], 0, 6);
    }
    
    LinkAddress(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6)
    {
      value[0] = b1;
      value[1] = b2;
      value[2] = b3;
      value[3] = b4;
      value[4] = b5;
      value[5] = b6;
    }
    
    explicit LinkAddress(bool broadcast)
    {
      for (uint8_t i = 0; i != 6; ++i)
        value[i] = 0xFF;      
    }
    
    explicit LinkAddress(uint8_t const* value_)
    {
      memcpy(&value[0], value_, 6);
    }
    
    LinkAddress(LinkAddress const& value_)
    {
      memcpy(&value[0], &value_.value[0], 6);
    }
    
    bool operator== (LinkAddress const& rhs) const
    {
      return memcmp(&value[0], &rhs.value[0], 6) == 0;
    }      
    
    bool operator!= (LinkAddress const& rhs) const
    {
      return !(*this == rhs);
    }
    
    uint8_t operator[] (uint8_t index) const
    {
      return value[index];
    }      
    
    uint8_t* data()
    {
      return &value[0];
    }
    
    uint8_t const* data() const
    {
      return &value[0];
    }      
    
    // true is = (0,0,0,0,0,0)
    bool isNull() const
    {
      for (uint8_t i = 0; i != 6; ++i)
        if (value[i])
          return false;
      return true;
    }
    
    // true is = (0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF)
    bool isBroadcast() const
    {
      for (uint8_t i = 0; i != 6; ++i)
        if (value[i] != 0xFF)
          return false;
      return true;
    }
    
                   
  private:
    uint8_t value[6];
  };

  
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // frame used to send packet to link layer

  struct LinkLayerSendFrame
  {
    LinkAddress     srcAddress;
    LinkAddress     destAddress;
    uint16_t        type_length;
    DataList const* dataList;
    
    explicit LinkLayerSendFrame(LinkAddress const& srcAddress_, LinkAddress const& destAddress_, uint16_t type_length_, DataList const* dataList_)
      : srcAddress(srcAddress_), destAddress(destAddress_), type_length(type_length_), dataList(dataList_)
    {
    }
    
  };


  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // abstract frame used to receive packets from link layer
  
  struct LinkLayerReceiveFrame
  {
    LinkAddress destAddress;
    LinkAddress srcAddress;
    uint16_t    type_length;
    uint16_t    dataLength;  // may include additional padding

    virtual void readReset() = 0;
    virtual uint8_t readByte() = 0;
    virtual uint16_t readWord() = 0;
    virtual void readBlock(void* dstBuffer, uint16_t length) = 0;
    
    LinkLayerReceiveFrame()
    {      
    }
    
    LinkLayerReceiveFrame(LinkAddress const& srcAddress_, LinkAddress const& destAddress_, uint16_t type_length_, uint16_t dataLength_)
      : destAddress(destAddress_), srcAddress(srcAddress_), type_length(type_length_), dataLength(dataLength_)
    {      
    }
    
    void bypass(uint16_t bytesToBypass)
    {
      while (bytesToBypass--)
        readByte();
    }
  };



  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // interface used by classes that need to receive packets (when recvFrame is called)
  
  struct ILinkLayerListener
  {
    virtual bool processLinkLayerFrame(LinkLayerReceiveFrame* frame) = 0;
  };


  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // interface used by link layer
  
  struct ILinkLayer
  {
    enum SendResult
    {
      SendFail,
      SendOK
    };

    virtual LinkAddress const& getAddress() const = 0;
    virtual void addListener(ILinkLayerListener* listener) = 0;
    virtual void recvFrame() = 0;
    virtual SendResult sendFrame(LinkLayerSendFrame const* frame) = 0;
  };


  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // IPAddress  (an IPv4 address)
  
  struct IPAddress
  {
    IPAddress()
    {        
      m_value[0] = m_value[1] = m_value[2] = m_value[3] = 0;
    }
    
    IPAddress(IPAddress const& ip)
    {      
      m_value[0] = ip.m_value[0];
      m_value[1] = ip.m_value[1];
      m_value[2] = ip.m_value[2];
      m_value[3] = ip.m_value[3];
    }
    
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
    {
      m_value[0] = a;
      m_value[1] = b;
      m_value[2] = c;
      m_value[3] = d;
    }
    
    bool isAllZero() const
    {
      return m_value[0] == 0x00 && m_value[1] == 0x00 && m_value[2] == 0x00 && m_value[3] == 0x00;
    }
    
    bool operator== (IPAddress const& rhs) const
    {
      return m_value[0] == rhs.m_value[0] && m_value[1] == rhs.m_value[1] && m_value[2] == rhs.m_value[2] && m_value[3] == rhs.m_value[3];
    }   
    
    bool operator!= (IPAddress const& rhs) const
    {
      return !(*this == rhs);
    }   
    
    IPAddress operator& (IPAddress const& rhs) const
    {
      return IPAddress(m_value[0] & rhs.m_value[0], m_value[1] & rhs.m_value[1], m_value[2] & rhs.m_value[2], m_value[3] & rhs.m_value[3]);
    }
    
    uint8_t const& operator[] (uint8_t index) const
    {
      return m_value[index];
    }

    uint8_t& operator[] (uint8_t index)
    {
      return m_value[index];
    }
    
    void operator= (IPAddress const& rhs)
    {
      m_value[0] = rhs.m_value[0];
      m_value[1] = rhs.m_value[1];
      m_value[2] = rhs.m_value[2];
      m_value[3] = rhs.m_value[3];      
    }      
    
    uint8_t const* data() const
    {
      return &m_value[0];
    }      

    uint8_t* data()
    {
      return &m_value[0];
    }
    
    // calc number of "1" bits
    // used in route finding algorithm
    int8_t calcRank()
    {
      uint8_t r = 0;
      for (uint8_t j = 0; j != 4; ++j)
        for (uint8_t i = 0; i != 8; ++i)
          if (m_value[j] & (1 << i))
            ++r;  
      return r;
    }
  
  private:
    uint8_t m_value[4];  
  };


  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Protocol_ARP (ARP - Address Resolution Protocol)

  // This ARP implementation is specific for IPv4 only
  class Protocol_ARP : public ILinkLayerListener
  {
    
  public:
  
    static uint8_t const  MAXINTERFACES = 2;   // maximum number of network interfaces
    
    static uint8_t const  MAXARPENTRIES = 5;
    
    static uint32_t const MAXCACHEITEMAGE = 300;  // maxium age of a cache item (in seconds)
    

    // the ARP table item
    struct Item
    {
      IPAddress   protocolAddress;
      LinkAddress hardwareAddress;
      uint32_t    creationTime;  // creation time in seconds

      Item()
      {
      }

      Item(IPAddress const& protocolAddress_, LinkAddress const& hardwareAddress_, uint32_t creationTime_)
        : protocolAddress(protocolAddress_), hardwareAddress(hardwareAddress_), creationTime(creationTime_)
      {
      }

      bool operator== (Item const& rhs)
      {
        return protocolAddress == rhs.protocolAddress && hardwareAddress == rhs.hardwareAddress;
      }
      
      bool isValid()
      {
        return seconds() - creationTime < MAXCACHEITEMAGE;
      }
    };
    
    // ARP packet
    struct ARPPacket
    {
      uint16_t    hardwareAddressType;       // 0x0001 = Ethernet
      uint16_t    protocolAddressType;       // 0x0800 = IPv4
      uint8_t     hardwareAddressLength;     // 6 = Ethernet
      uint8_t     protocolAddressLength;     // 4 = IPv4
      uint16_t    operation;                 // 1 = request, 2 = reply
      LinkAddress sourceHardwareAddress;
      IPAddress   sourceProtocolAddress;
      LinkAddress targetHardwareAddress;     // all 0 = unknown
      IPAddress   targetProtocolAddress;
    };


  public:
  
    struct InterfaceEntry
    {
      ILinkLayer* interface;
      IPAddress   address;
      
      InterfaceEntry()
      {
      }
      
      InterfaceEntry(ILinkLayer* interface_, IPAddress const& address_)
        : interface(interface_), address(address_)
      {
      }
    };

  public:

    Protocol_ARP()
    {
    }
     
    void addInterface(ILinkLayer* interface, IPAddress const& address)
    {
      m_interfaces.push_back(InterfaceEntry(interface, address));
      interface->addListener(this);
    }
    
    Array<InterfaceEntry, MAXINTERFACES> const& interfaces() const
    {
      return m_interfaces;
    }
    
  private:

    bool processLinkLayerFrame(LinkLayerReceiveFrame* frame)
    {
      
      if (frame->type_length == 0x0806 && // check "type_length" ethernet field (0x0806)
          frame->readWord() == 0x0001 &&  // check Hardware Address Type of ARP field (0x0001 = Ethernet)
          frame->readWord() == 0x0800 &&  // check Protocol Address Type of ARP field (0x0800 = IPv4)
          frame->readByte() == 6 &&       // check Hardware Address Length of ARP field (6 = Ethernet)
          frame->readByte() == 4)         // check Protocol Address Length of ARP field (4 = IPv4)
      {
        #ifdef TCPVERBOSE
        serial.write_P(PSTR("ARP::processLinkLayerFrame")); cout << endl;
        #endif

        uint16_t operation = frame->readWord();

        LinkAddress sourceHardwareAddress;
        frame->readBlock(sourceHardwareAddress.data(), 6);

        IPAddress sourceProtocolAddress;
        frame->readBlock(&sourceProtocolAddress, 4);

        // bypass target hardware address
        frame->bypass(6);
        
        IPAddress targetProtocolAddress;
        frame->readBlock(&targetProtocolAddress, 4);

        uint8_t interfaceIndex = 0xFF;
        for (uint8_t i = 0; i != m_interfaces.size(); ++i)
          if (targetProtocolAddress == m_interfaces[i].address)
          {
            interfaceIndex = i;
            break;
          }
        if (interfaceIndex == 0xFF)
          return false; // this ARP is not for me!

        // save sender addresses
        addCacheTableItem(Item(sourceProtocolAddress, sourceHardwareAddress, seconds()));

        switch (operation)
        {
          case 0x0001:  // Request
          {
            // send reply
            sendPacket(interfaceIndex, 0x0002, sourceHardwareAddress, sourceProtocolAddress);
            break;
          }
          case 0x0002:  // Reply
            // nothing to do, address already saved
            break;
        }
        return true;
      }
      return false;
    }

  public:

    void addCacheTableItem(Item const& item)
    {
      for (uint8_t i = 0; i != m_table.size(); ++i)
      {
        if (m_table[i].protocolAddress == item.protocolAddress)
        {
          // update entry
          #ifdef TCPVERBOSE
          serial.write_P(PSTR("addCacheTableItem: entry updated ")); serial.writeIPv4(item.protocolAddress.data()); cout << endl;
          #endif
          m_table[i] = item;
          return;
        }
      }
      #ifdef TCPVERBOSE
      serial.write_P(PSTR("addCacheTableItem: entry added ")); serial.writeIPv4(item.protocolAddress.data()); cout << endl;
      #endif
      m_table.add(item);
    }

  private:

    // targetHardwareAddress can be (0,0,0,0,0,0) if unknown
    bool sendPacket(uint8_t interfaceIndex, uint16_t operation, LinkAddress const& targetHardwareAddress, IPAddress const& targetProtocolAddress)
    {
      InterfaceEntry& interfaceEntry = m_interfaces[interfaceIndex];
      ARPPacket packet;
      packet.hardwareAddressType   = Utility::htons(0x0001);
      packet.protocolAddressType   = Utility::htons(0x0800);
      packet.hardwareAddressLength = 6;
      packet.protocolAddressLength = 4;
      packet.operation             = Utility::htons(operation);
      packet.sourceHardwareAddress = interfaceEntry.interface->getAddress();
      packet.sourceProtocolAddress = interfaceEntry.address;
      packet.targetHardwareAddress = targetHardwareAddress;
      packet.targetProtocolAddress = targetProtocolAddress;
      DataList frameData(NULL, &packet, sizeof(ARPPacket));
      LinkLayerSendFrame frame(interfaceEntry.interface->getAddress(), 
                               targetHardwareAddress.isNull()? LinkAddress(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF) : targetHardwareAddress,  // if targetHardwareAddress=(0,0,0,0,0,0), it is broadcast
                               0x0806,      // this is ARP
                               &frameData);        
//cout << "sendPacket int=" << interfaceIndex << endl;
      return interfaceEntry.interface->sendFrame(&frame) == ILinkLayer::SendOK;
    }

  public:

    void clearCache()
    {
      m_table.clear();
    }

    LinkAddress const* getHardwareAddressFromCache(IPAddress const& targetProtocolAddress)
    {
      for (uint8_t i = 0; i != m_table.size(); ++i)
        if (m_table[i].isValid() && m_table[i].protocolAddress == targetProtocolAddress)
          return &m_table[i].hardwareAddress;
      return NULL; // not found
    }

    // return "NULL" on fail. When fail send an ARP request in broadcast, but doesn't wait for it, so you should loop getHardwareAddress multiple times
    LinkAddress const* getHardwareAddress(uint8_t interfaceIndex, IPAddress const& targetProtocolAddress)
    {
      #ifdef TCPVERBOSE
      serial.write_P(PSTR("getHardwareAddress() of ")); serial.writeIPv4(targetProtocolAddress.data()); cout << endl;
      #endif
      
      // check cache first
      LinkAddress const* r = getHardwareAddressFromCache(targetProtocolAddress);

      if (r == NULL)
      {
        // not found, send request message
        #ifdef TCPVERBOSE
        serial.write_P(PSTR("getHardwareAddress(): not found, send ARP request")); cout << endl;
        #endif
        sendPacket(interfaceIndex, 0x0001, LinkAddress(), targetProtocolAddress);
      }
      
      return r;
    }
    
  private:

    CircularBuffer<Item, MAXARPENTRIES>  m_table;           // the ARP table (actually a circular buffer)
    Array<InterfaceEntry, MAXINTERFACES> m_interfaces;      // link layer interfaces

  };


  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Protocol_IP (IP - Internet Protocol)
  // Does Not support datagrams fragmentation (for both RX and TX)
  // Does Not support multicast and broadcast for TX
  // Does Not check checksum on received packets

  class Protocol_IP : public ILinkLayerListener
  {

  public:
  
    static uint8_t const  MAXROUTEENTRIES = 3; // routing table size: other dest + local + one free

  private:

    static uint8_t const  MAXLISTENERS    = 3; 
    static uint32_t const ARPTRYTIMEOUT   = 1500;
    
    struct RouteEntry
    {
      IPAddress destination;
      IPAddress netmask;
      IPAddress gateway;
      uint8_t   interfaceIndex;  
      
      RouteEntry()
      {        
      }
      
      RouteEntry(IPAddress const& destination_, IPAddress const& netmask_, IPAddress const& gateway_, uint8_t interfaceIndex_)
        : destination(destination_), netmask(netmask_), gateway(gateway_), interfaceIndex(interfaceIndex_)
      {          
      }
    };
        

  public:

    struct Datagram
    {
      uint8_t   protocol;
      IPAddress sourceAddress;
      IPAddress destAddress;
      void*     data;
      uint16_t  dataLength;
    };


    // interface used by classes that need to receive packets
    struct IListener
    {
      virtual bool processIPDatagram(Datagram* datagram) = 0;
    };


  public:

    explicit Protocol_IP(bool routingEnabled)
      : m_ARP(NULL), m_datagramIdent(0), m_routingEnabled(routingEnabled)
    {            
    }

    
    void setARP(Protocol_ARP* ARP)
    {
      m_ARP = ARP;
      for (uint8_t i = 0; i != m_ARP->interfaces().size(); ++i)
        m_ARP->interfaces()[i].interface->addListener(this);
    }
    
    
    void addRoute(IPAddress const& destination, IPAddress const& netmask, IPAddress const& gateway, uint8_t interfaceIndex)
    {
      m_routingTable.push_back(RouteEntry(destination, netmask, gateway, interfaceIndex));      
    }
    

    void addListener(IListener* listener)
    {
      m_listeners.push_back(listener);
    }

  
    // return 0xFF on fail  
    uint8_t findInterfaceForAddress(IPAddress const& destAddress, IPAddress* effectiveDestination)
    {
      uint8_t bestRouteIndex = 0xFF;
      int8_t bestRouteRank   = -1;
      for (uint8_t i = 0; i != m_routingTable.size(); ++i)
      {
        //cout << "rout.netmask & destAddress = "; serial.writeIPv4((m_routingTable[i].netmask & destAddress).data()); cout << endl;
        //cout << "rout.dest = "; serial.writeIPv4(m_routingTable[i].destination.data()); cout << endl;
        if ((m_routingTable[i].netmask & destAddress) == m_routingTable[i].destination)
        {
          int8_t rank = m_routingTable[i].netmask.calcRank();
          if (rank > bestRouteRank)
          {
            // found a better route
            bestRouteIndex = i;
            bestRouteRank  = rank;
          }
        }
      }
      //cout << "use route " << bestRouteIndex << endl;
      if (bestRouteIndex == 0xFF)
        return 0xFF; // no route  
        
      uint8_t interfaceIndex = m_routingTable[bestRouteIndex].interfaceIndex;
      if (effectiveDestination != NULL) 
      {
        // Am I the gateway?
        if (m_routingTable[bestRouteIndex].gateway != m_ARP->interfaces()[interfaceIndex].address)
          *effectiveDestination = m_routingTable[bestRouteIndex].gateway;  // no, effective destination is the gateway
        else
          *effectiveDestination = destAddress; // yes, don't need the gateway
      }
      return interfaceIndex;   
    }


    // if srcAddress=0.0.0.0 then it is automatically selected from used interface
    bool send(IPAddress const& srcAddress, IPAddress const& destAddress, uint8_t protocol, DataList const& data)
    {
      #ifdef TCPVERBOSE
      serial.write_P(PSTR("IPsend to ")); serial.writeIPv4(destAddress.data()); cout << endl;
      #endif
      
      IPAddress effectiveDestAddress; // this IP address is used only in order to get effective hardware address, not as effective destination IP address
      uint8_t   interfaceIndex = findInterfaceForAddress(destAddress, &effectiveDestAddress);
      if (interfaceIndex == 0xFF)
        return false; // no route, fail

      // find destination hardware address
      LinkAddress const* destHardwareAddress = m_ARP->getHardwareAddress(interfaceIndex, effectiveDestAddress);
      if (destHardwareAddress == NULL)  // still not available? Try until ARPTRYTIMEOUT timeouts
      {
        return false;
        // todo: test
        /*
        TimeOut timeOut(ARPTRYTIMEOUT);
        while ((destHardwareAddress = m_ARP->getHardwareAddressFromCache(effectiveDestAddress)) == NULL && !timeOut)
          receive();
          */
      }
                        
      IPAddress sourceAddress = srcAddress.isAllZero()? m_ARP->interfaces()[interfaceIndex].address : srcAddress;  // is the source IP auto calculated?
            
      if (destHardwareAddress == NULL)
      {
        #ifdef TCPVERBOSE
        serial.write_P(PSTR("IPsend: fail!")); cout << endl;
        #endif        
        return false; // failed to get destination hardware address
      }        

      // IP header
      uint8_t IPHeader[20];
      //   VER (4) | HLEN (5 = 20 bytes)
      IPHeader[0] = 0x45;
      //   TOS (0)
      IPHeader[1] = 0x00;
      //   Total Length
      uint16_t totalLength = 20 + data.calcLength();
      IPHeader[2] = totalLength >> 8;
      IPHeader[3] = totalLength & 0xFF;
      //   Identification
      IPHeader[4] = m_datagramIdent >> 8;
      IPHeader[5] = m_datagramIdent & 0xFF;
      m_datagramIdent++;
      //   Flags (0) | Fragment offset
      IPHeader[6] = 0x00;
      IPHeader[7] = 0x00;
      //   TTL - Time To Live
      IPHeader[8] = 64;
      //   Protocol
      IPHeader[9] = protocol;
      //   Header checksum
      IPHeader[10] = 0;
      IPHeader[11] = 0;
      //   Source IP address
      IPHeader[12] = sourceAddress[0];
      IPHeader[13] = sourceAddress[1];
      IPHeader[14] = sourceAddress[2];
      IPHeader[15] = sourceAddress[3];
      //   Destination IP address
      IPHeader[16] = destAddress[0];
      IPHeader[17] = destAddress[1];
      IPHeader[18] = destAddress[2];
      IPHeader[19] = destAddress[3];
      // calculate checksum
      uint16_t checksum = DataList(NULL, &IPHeader[0], 20).calcInternetChecksum();
      IPHeader[10] = checksum >> 8;
      IPHeader[11] = checksum & 0xFF;

      // link layer
      DataList dataList(&data, &IPHeader[0], 20);
      ILinkLayer* interface = m_ARP->interfaces()[interfaceIndex].interface;
      LinkLayerSendFrame frame(interface->getAddress(), *destHardwareAddress, 0x0800, &dataList);
                                                                          
      return interface->sendFrame(&frame) == ILinkLayer::SendOK;
    }


    bool processLinkLayerFrame(LinkLayerReceiveFrame* frame)
    {
      if (frame->type_length == 0x0800)
      {
        Datagram datagram;

        // VER | HLEN
        uint8_t b = frame->readByte();
        if ( ((b >> 4) & 0x0F) != 4)
          return false; // unsupported IP version
        uint16_t headerLength = static_cast<uint16_t>(b & 0x0F) * 4;  // header length in bytes
        if (headerLength < 20 || headerLength > frame->dataLength)
        {
          #ifdef TCPVERBOSE
          serial.write_P(PSTR("Protocol_IP, invalid headerLength")); cout << endl;
          #endif
          return false; // invalid headerLength                   
        }
        // TOS (bypass)
        frame->readByte();
        // Total Length
        uint16_t totalLength = frame->readWord();
        if (totalLength < headerLength || totalLength > frame->dataLength)  // fdv1
        {
          #ifdef TCPVERBOSE
          serial.write_P(PSTR("Protocol_IP, invalid totalLength")); cout << endl;
          #endif
          return false; // invalid totalLength
        }          
        // Identification (bypass)
        frame->readWord();
        // Flags (bypass) | Fragment offset (bypass)
        frame->readWord();
        // TTL - time To Live (bypass)
        frame->readByte();
        // Protocol
        datagram.protocol = frame->readByte();
        // Checksum (bypass)
        frame->readWord();
        // Source IP address
        for (uint8_t i = 0; i != 4; ++i)
          datagram.sourceAddress[i] = frame->readByte();
        // Destination IP address
        for (uint8_t i = 0; i != 4; ++i)
          datagram.destAddress[i] = frame->readByte();
        // bypass other header fields
        for (uint8_t i = 20; i != headerLength; ++i)
          frame->readByte();

        // data
        datagram.dataLength = totalLength - headerLength;
        if (getFreeMem() - 200 < datagram.dataLength)
          return false; // cannot allocate
        SimpleBuffer<uint8_t> dataBuffer(datagram.dataLength);
        if (dataBuffer.get() == NULL)
            return false; // cannot allocate
        datagram.data = dataBuffer.get();
        frame->readBlock(datagram.data, datagram.dataLength);

        // is this for me?
        bool rightDest = false;
        for (uint8_t i = 0; i != m_ARP->interfaces().size(); ++i)
          if (m_ARP->interfaces()[i].address == datagram.destAddress)
          {
            rightDest = true;
            break;
          }

        if (rightDest)
        {          
          // add address to ARP
          m_ARP->addCacheTableItem(Protocol_ARP::Item(datagram.sourceAddress, frame->srcAddress, seconds()));
          
          // send to listeners
          for (uint8_t i = 0; i != m_listeners.size(); ++i)
            if (m_listeners[i]->processIPDatagram(&datagram))
              break; // message processed   
        }
        else if (m_routingEnabled)
        {
          // perform routing
          send(datagram.sourceAddress, datagram.destAddress, datagram.protocol, DataList(NULL, datagram.data, datagram.dataLength));
        }

        return true;
      }
      else
        return false; // not IP
    }

    void receive()
    {
      for (uint8_t i = 0; i != m_ARP->interfaces().size(); ++i)
        m_ARP->interfaces()[i].interface->recvFrame();
    }      
    
    Array<Protocol_ARP::InterfaceEntry, Protocol_ARP::MAXINTERFACES> const& interfaces() const
    {
      return m_ARP->interfaces();
    }   
    
    void routingEnabled(bool value)
    {
      m_routingEnabled = value;
    }  
    
    bool routingEnabled() const
    {
      return m_routingEnabled;
    }
    

  private:

    Protocol_ARP*                      m_ARP;            // ARP protocol
    uint16_t                           m_datagramIdent;  // counter to identify each outgoing datagram
    Array<IListener*, MAXLISTENERS>    m_listeners;      // upper layer listeners
    Array<RouteEntry, MAXROUTEENTRIES> m_routingTable;   // routing table    
    bool                               m_routingEnabled; // routing enabled
  };


  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Protocol_ICMP (ICMP - Internet Control Message Protocol)
  // Supports only pings
  // Doesn't check checksum on received packets

  class Protocol_ICMP : public Protocol_IP::IListener
  {
    
    static uint32_t const MAXECHOREPLYTIME = 4000;


  public:

    explicit Protocol_ICMP(Protocol_IP* ip)
      : m_IP(ip)
    {
      m_IP->addListener(this);
    }
        
    bool processIPDatagram(Protocol_IP::Datagram* datagram)
    {
      if (datagram->protocol == 0x01 && datagram->dataLength >= 8)
      {
        uint8_t* databuf = static_cast<uint8_t*>(datagram->data);
        if (databuf[0] == 8 && databuf[1] == 0)       // received Echo request
        {
          // set ICMP type = 0 (echo reply), code = 0
          databuf[0] = 0;
          databuf[1] = 0;
          // clear checksum
          databuf[2] = 0;
          databuf[3] = 0;
          // calculate and set new checksum
          uint16_t checksum = DataList(NULL, datagram->data, datagram->dataLength).calcInternetChecksum();
          databuf[2] = checksum >> 8;
          databuf[3] = checksum & 0xFF;
          // send back datagram
          m_IP->send(datagram->destAddress, datagram->sourceAddress, 0x01, DataList(NULL, datagram->data, datagram->dataLength));
          return true;
        }
        else if (databuf[0] == 0 && databuf[1] == 0)  // received Echo reply
        {
          m_receivedID = ((uint16_t)databuf[4] << 8) | databuf[5];
          return true;
        }
      }
      return false;
    }

    // send Echo Request and wait for Echo Reply
    // return "measured" echo time in milliseconds. ret 0xFFFFFFFF on timeout
    uint32_t ping(IPAddress const& dest)
    {
      uint32_t t1 = millis();
      
      // prepare Echo Request
      uint16_t id  = Random::nextUInt16(0, 0xFFFF);
      m_receivedID = ~id; // just to make it different
      uint8_t data[] =
      {
        8,         // type = 8, Echo Request
        0,         // code = 0
        0,         // checksum high
        0,         // checksum low
        id >> 8,   // identifier high
        id & 0xFF, // identifier low
        0,         // sequence number high
        0          // sequence number low 
      };
      // calculate and set checksum
      uint16_t checksum = DataList(NULL, &data[0], sizeof(data)).calcInternetChecksum();
      data[2] = checksum >> 8;
      data[3] = checksum & 0xFF;      
      // send Echo Request
      m_IP->send(IPAddress(0, 0, 0, 0), dest, 0x01, DataList(NULL, &data[0], sizeof(data)));
      
      // wait for reply
      TimeOut timeOut(MAXECHOREPLYTIME);
      while (m_receivedID != id && !timeOut)
        m_IP->receive();
      
      return (m_receivedID == id? millis() - t1 : 0xFFFFFFFF);
    }
    

  private:

    Protocol_IP* m_IP;  // IP layer
    uint16_t     m_receivedID;
    
  };


  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Protocol_UDP (User Datagram Protocol)
  
  class Protocol_UDP : public Protocol_IP::IListener
  {
  
  public:
  
    static uint8_t const MAXLISTENERS = 2;

  
  public:

    struct PseudoHeader
    {
      IPAddress sourceAddress;
      IPAddress destAddress;
      uint8_t   zeros;
      uint8_t   protocol;
      uint16_t  length;        
    };

    struct Datagram
    {
      uint16_t  sourcePort;
      uint16_t  destPort;
      uint16_t  dataLength;
      uint8_t*  data;
    };        

    // interface used by classes that need to receive packets
    struct IListener
    {
      virtual bool processUDPDatagram(IPAddress const& sourceAddress, Datagram* datagram) = 0;
    };
  
  
  public:
  
    explicit Protocol_UDP(Protocol_IP* ip)
      : m_IP(ip), m_lastSrcUsedPort(49151)
    {
      m_IP->addListener(this);
    }
    
    
    void addListener(IListener* listener)
    {
      m_listeners.push_back(listener);
    }


    bool processIPDatagram(Protocol_IP::Datagram* datagram)
    {
      #ifdef TCPVERBOSE
      serial.write_P(PSTR("processIPDatagram from ")); serial.writeIPv4(datagram->sourceAddress.data()); cout << endl;
      serial.write_P(PSTR("  prot = ")); cout << (uint16_t)datagram->protocol << endl;
      //cout << "  len  = " << datagram->dataLength << endl;
      #endif
      if (datagram->protocol == 0x11 && datagram->dataLength >= 8)
      {
        uint8_t* databuf = static_cast<uint8_t*>(datagram->data);
        
        Datagram UDPDatagram;
        UDPDatagram.sourcePort = (uint16_t)databuf[0] << 8 | databuf[1];
        UDPDatagram.destPort   = (uint16_t)databuf[2] << 8 | databuf[3];
        UDPDatagram.dataLength = ((uint16_t)databuf[4] << 8 | databuf[5]) - 8;
        UDPDatagram.data       = &databuf[8];              
        
        for (uint8_t i = 0; i != m_listeners.size(); ++i)
          if (m_listeners[i]->processUDPDatagram(datagram->sourceAddress, &UDPDatagram))
            break;
          
        return true;
      }
      return false;
    }   
    
    
    bool send(uint16_t destPort, IPAddress const& destAddress, DataList const& data)
    {
      m_lastSrcUsedPort = (m_lastSrcUsedPort == 65535? 49152 : m_lastSrcUsedPort + 1);
      return send(m_lastSrcUsedPort, destPort, destAddress, data);
    }
    
    bool send(uint16_t srcPort, uint16_t destPort, IPAddress const& destAddress, DataList const& data)
    {
      
      #ifdef TCPVERBOSE
      serial.write_P(PSTR("UDP Send to ")); serial.writeIPv4(destAddress.data()); cout << endl;
      serial.write_P(PSTR("mem      = ")); cout << (uint16_t)getFreeMem() << endl;
      #endif

      PseudoHeader pseudoHeader;
     
      uint8_t interfaceIndex = m_IP->findInterfaceForAddress(destAddress, NULL); 
      
      pseudoHeader.sourceAddress = m_IP->interfaces()[interfaceIndex].address;
      pseudoHeader.destAddress   = destAddress;
      pseudoHeader.zeros         = 0x00;
      pseudoHeader.protocol      = 0x11;
      pseudoHeader.length        = Utility::htons(8 + data.calcLength());
      
      uint16_t dataLength = data.calcLength();
      
      uint8_t udphead[8] =
      {
        srcPort >> 8,
        srcPort & 0xFF,
        destPort >> 8,
        destPort & 0xFF,
        (8 + dataLength) >> 8,
        (8 + dataLength) & 0xFF,
        0,
        0
      };      
      
      DataList datagram(&data, &udphead[0], 8);
      
      // calc checksum      
      uint16_t checksum = DataList(&datagram, &pseudoHeader, sizeof(PseudoHeader)).calcInternetChecksum();
      udphead[6] = checksum >> 8;
      udphead[7] = checksum & 0xFF;
      
      return m_IP->send(IPAddress(0, 0, 0, 0), destAddress, 0x11, datagram);
    }     
    
    
    void receive()
    {
      m_IP->receive();
    }
    
    
  
  private:
  
    Protocol_IP*                    m_IP;              // IP layer
    Array<IListener*, MAXLISTENERS> m_listeners;       // upper layer listeners
    uint16_t                        m_lastSrcUsedPort; // next port to use for sending
    
  };



  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // StackTCPIP (aggregates Protocol_ARP, Protocol_IP, Protocol_ICMP, Protocol_UDP)
  
  class StackTCPIP
  {
    
  public:
    
    StackTCPIP(IPAddress const& IP, IPAddress const& subnet, IPAddress const& gateway, ILinkLayer* interface, bool routingEnabled)
      : m_IP(routingEnabled),
        m_ICMP(&m_IP),
        m_UDP(&m_IP)
    {      
      m_ARP.addInterface(interface, IP);
      m_IP.setARP(&m_ARP);
      m_IP.addRoute(IP & subnet, subnet, IP, 0);  // myself
      m_IP.addRoute(IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0), gateway, 0);   // default gateway
    }
    
    explicit StackTCPIP(bool routingEnabled)
      : m_IP(routingEnabled),
        m_ICMP(&m_IP),
        m_UDP(&m_IP)
    {      
    }
    
    void yield()
    {
      m_UDP.receive();
    }
    
    Protocol_UDP& UDP()
    {
      return m_UDP;
    }
    
    Protocol_ICMP& ICMP()
    {
      return m_ICMP;
    }
    
    Protocol_IP& IP()
    {
      return m_IP;
    }
    
    Protocol_ARP& ARP()
    {
      return m_ARP;
    }
  
  private:  
  
    Protocol_ARP  m_ARP;
    Protocol_IP   m_IP;
    Protocol_ICMP m_ICMP;
    Protocol_UDP  m_UDP;
  };




} // namespace fdv




#endif /* FDV_TCPIP_H_ */