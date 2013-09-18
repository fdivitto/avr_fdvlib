/*
# Created by Fabrizio Di Vittorio (fdivitto@gmail.com)
# Copyright (c) 2013 Fabrizio Di Vittorio.
# All rights reserved.
 
# GNU GPL LICENSE
#
# This module is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; latest version thereof,
# available at: <http://www.gnu.org/licenses/gpl.txt>.
#
# This module is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this module; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
*/



#ifndef FDV_ENC28J60_H_
#define FDV_ENC28J60_H_


#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdarg.h>

#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/atomic.h>


#include "../fdv_generic/fdv_pin.h"
#include "../fdv_generic/fdv_serial.h"
#include "../fdv_generic/fdv_timesched.h"
#include "../fdv_generic/fdv_spi.h"
#include "../fdv_generic/fdv_interrupt.h"
#include "../fdv_network/fdv_TCPIP.h"



namespace fdv
{





  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // ENC28J60

  class ENC28J60 : public ExtInterrupt::IExtInterruptCallable, public ILinkLayer
  {

  private:

    static uint8_t const BIT0  = 0x01;
    static uint8_t const BIT1  = 0x02;
    static uint8_t const BIT2  = 0x04;
    static uint8_t const BIT3  = 0x08;
    static uint8_t const BIT4  = 0x10;
    static uint8_t const BIT5  = 0x20;
    static uint8_t const BIT6  = 0x40;
    static uint8_t const BIT7  = 0x80;
    
    static uint16_t const BIT8  = 0x0100;
    static uint16_t const BIT9  = 0x0200;
    static uint16_t const BIT10 = 0x0400;
    static uint16_t const BIT11 = 0x0800;
    static uint16_t const BIT12 = 0x1000;
    static uint16_t const BIT13 = 0x2000;
    static uint16_t const BIT14 = 0x4000;
    static uint16_t const BIT15 = 0x8000;


    static uint8_t const BANK0 = 0b00 << 5;
    static uint8_t const BANK1 = 0b01 << 5;
    static uint8_t const BANK2 = 0b10 << 5;
    static uint8_t const BANK3 = 0b11 << 5;

    // ECON1: ETHERNET CONTROL REGISTER 1
    static uint8_t const ECON1 = 0x1F;
    //   BSEL: Bank Select bits (2 bit)
    static uint8_t const SHIFT_BSEL = 0;
    //   TXRST: Transmit Logic Reset bit
    static uint8_t const BIT_TXRST = BIT7;
    //   RXRST: Receive Logic Reset bit
    static uint8_t const BIT_RXRST = BIT6;
    //   DMAST: DMA Start and Busy Status bit
    static uint8_t const BIT_DMAST = BIT5;
    //   CSUMEN: DMA Checksum Enable bit
    static uint8_t const BIT_CSUMEN = BIT4;
    //   TXRTS: Transmit Request to Send bit
    static uint8_t const BIT_TXRTS = BIT3;
    //   RXEN: Receive Enable bit
    static uint8_t const BIT_RXEN = BIT2;

    // ECON2: ETHERNET CONTROL REGISTER 2
    static uint8_t const ECON2 =  0x1E;
    //   AUTOINC: Automatic Buffer Pointer Increment Enable bit
    static uint8_t const BIT_AUTOINC = BIT7;
    //   PKTDEC: Packet Decrement bit
    static uint8_t const BIT_PKTDEC = BIT6;
    //   PWRSV: Power Save Enable bit
    static uint8_t const BIT_PWRSV = BIT5;
    //   VRPS: Voltage Regulator Power Save Enable bit
    static uint8_t const BIT_VRPS = BIT3;

    // MAADR5 : MAC address byte 5 (15:8)
    static uint8_t const MAADR5 = BANK3 | 0x00;
    // MAADR6 : MAC address byte 6 (7:0)
    static uint8_t const MAADR6 = BANK3 | 0x01;
    // MAADR3 : MAC address byte 3 (31:24)
    static uint8_t const MAADR3 = BANK3 | 0x02;
    // MAADR4 : MAC address byte 4 (23:16)
    static uint8_t const MAADR4 = BANK3 | 0x03;
    // MAADR1 : MAC address byte 1 (47:40)
    static uint8_t const MAADR1 = BANK3 | 0x04;
    // MAADR2 : MAC address byte 2 (39:32)
    static uint8_t const MAADR2 = BANK3 | 0x05;

    // EREVID : Ethernet Revision ID
    static uint8_t const EREVID = BANK3 | 0x12;

    // MACON1: MAC CONTROL REGISTER 1
    static uint8_t const MACON1 = BANK2 | 0x00;
    //   TXPAUS: Pause Control Frame Transmission Enable bit
    static uint8_t const BIT_TXPAUS = BIT3;
    //   RXPAUS: Pause Control Frame Reception Enable bit
    static uint8_t const BIT_RXPAUS = BIT2;
    //   PASSALL: Pass All Received Frames Enable bit
    static uint8_t const BIT_PASSALL = BIT1;
    //   MARXEN: MAC Receive Enable bit
    static uint8_t const BIT_MARXEN = BIT0;

    // MACON2: MAC CONTROL REGISTER 2 (documented only in first documentation revision!!)
    static uint8_t const MACON2 = BANK2 | 0x01;
    // MARST: MAC Reset bit
    static uint8_t const BIT_MARST   = BIT7;
    // RNDRST: MAC Random Number Generator Reset bit
    static uint8_t const BIT_RNDRST  = BIT6;
    // MARXRST: MAC Control Sublayer/Receive Logic Reset bit
    static uint8_t const BIT_MARXRST = BIT3;
    // RFUNRST: MAC Receive Function Reset bit
    static uint8_t const BIT_RFUNRST = BIT2;
    // MATXRST: MAC Control Sublayer/Transmit Logic Reset bit
    static uint8_t const BIT_MATXRST = BIT1;
    // TFUNRST: MAC Transmit Function Reset bit
    static uint8_t const BIT_TFUNRST = BIT0;

    // MACON3: MAC CONTROL REGISTER 3
    static uint8_t const MACON3 = BANK2 | 0x02;
    //   PADCFG2:PADCFG0: Automatic Pad and CRC Configuration bits
    static uint8_t const SHIFT_PADCFG = 5;
    //   TXCRCEN: Transmit CRC Enable bit
    static uint8_t const BIT_TXCRCEN = BIT4;
    //   PHDREN: Proprietary Header Enable bit
    static uint8_t const BIT_PHDREN = BIT3;
    //   HFRMEN: Huge Frame Enable bit
    static uint8_t const BIT_HFRMEN = BIT2;
    //   FRMLNEN: Frame Length Checking Enable bit
    static uint8_t const BIT_FRMLNEN = BIT1;
    //   FULDPX: MAC Full-Duplex Enable bit
    static uint8_t const BIT_FULDPX = BIT0;

    // MACON4: MAC CONTROL REGISTER 4
    static uint8_t const MACON4 = BANK2 | 0x03;
    //   DEFER: Defer Transmission Enable bit (applies to half duplex only)
    static uint8_t const BIT_DEFER = BIT6;
    //   BPEN: No Backoff During Backpressure Enable bit (applies to half duplex only)
    static uint8_t const BIT_BPEN = BIT5;
    //   NOBKOFF: No Backoff Enable bit (applies to half duplex only)
    static uint8_t const BIT_NOBKOFF = BIT4;

    // MAMXFLL: Maximum Frame Length Low Byte (MAMXFL<7:0>)
    static uint8_t const MAMXFLL = BANK2 | 0x0A;

    // MAMXFLH: Maximum Frame Length High Byte (MAMXFL<15:8>)
    static uint8_t const MAMXFLH = BANK2 | 0x0B;

    // MABBIPG: MAC BACK-TO-BACK INTER-PACKET GAP REGISTER
    static uint8_t const MABBIPG = BANK2 | 0x04;

    // MAIPGL: Non-Back-to-Back Inter-Packet Gap Low Byte (MAIPGL<6:0>)
    static uint8_t const MAIPGL = BANK2 | 0x06;

    // MAIPGH: Non-Back-to-Back Inter-Packet Gap High Byte (MAIPGH<6:0>)
    static uint8_t const MAIPGH = BANK2 | 0x07;

    // RX Start Low Byte (ERXST<7:0>)
    static uint8_t const ERXSTL = BANK0 | 0x08;

    // RX Start High Byte (ERXST<12:8>)
    static uint8_t const ERXSTH = BANK0 | 0x09;

    // ERXNDL: RX End Low Byte (ERXND<7:0>)
    static uint8_t const ERXNDL = BANK0 | 0x0A;

    // ERXNDH: RX End High Byte (ERXND<12:8>)
    static uint8_t const ERXNDH = BANK0 | 0x0B;

    // ERXRDPTL: RX RD Pointer Low Byte (ERXRDPT<7:0>)
    static uint8_t const ERXRDPTL = BANK0 | 0x0C;

    // ERXRDPTH: RX RD Pointer High Byte (ERXRDPT<12:8>)
    static uint8_t const ERXRDPTH = BANK0 | 0x0D;

    // ERXFCON: ETHERNET RECEIVE FILTER CONTROL REGISTER
    static uint8_t const ERXFCON = BANK1 | 0x18;
    //   UCEN: Unicast Filter Enable bit
    static uint8_t const BIT_UCEN = BIT7;
    //   ANDOR: AND/OR Filter Select bit
    static uint8_t const BIT_ANDOR = BIT6;
    //   CRCEN: Post-Filter CRC Check Enable bit
    static uint8_t const BIT_CRCEN = BIT5;
    //   PMEN: Pattern Match Filter Enable bit
    static uint8_t const BIT_PMEN = BIT4;
    //   MPEN: Magic Packeto? Filter Enable bit
    static uint8_t const BIT_MPEN = BIT3;
    //   HTEN: Hash Table Filter Enable bit
    static uint8_t const BIT_HTEN = BIT2;
    //   MCEN: Multicast Filter Enable bit
    static uint8_t const BIT_MCEN = BIT1;
    //   BCEN: Broadcast Filter Enable bit
    static uint8_t const BIT_BCEN = BIT0;

    // EIE: ETHERNET INTERRUPT ENABLE REGISTER
    static uint8_t const EIE = 0x1B;
    //   INTIE: Global INT Interrupt Enable bit
    static uint8_t const BIT_INTIE = BIT7;
    //   PKTIE: Receive Packet Pending Interrupt Enable bit
    static uint8_t const BIT_PKTIE = BIT6;
    //   DMAIE: DMA Interrupt Enable bit
    static uint8_t const BIT_DMAIE = BIT5;
    //   LINKIE: Link Status Change Interrupt Enable bit
    static uint8_t const BIT_LINKIE = BIT4;
    //   TXIE: Transmit Enable bit
    static uint8_t const BIT_TXIE = BIT3;
    //   TXERIE: Transmit Error Interrupt Enable bit
    static uint8_t const BIT_TXERIE = BIT1;
    //   RXERIE: Receive Error Interrupt Enable bit
    static uint8_t const BIT_RXERIE = BIT0;

    // EIR: ETHERNET INTERRUPT REQUEST (FLAG) REGISTER
    static uint8_t const EIR = 0x1C;
    //   PKTIF: Receive Packet Pending Interrupt Flag bit
    static uint8_t const BIT_PKTIF = BIT6;
    //   DMAIF: DMA Interrupt Flag bit
    static uint8_t const BIT_DMAIF = BIT5;
    //   LINKIF: Link Change Interrupt Flag bit
    static uint8_t const BIT_LINKIF = BIT4;
    //   TXIF: Transmit Interrupt Flag bit
    static uint8_t const BIT_TXIF = BIT3;
    //   TXERIF: Transmit Error Interrupt Flag bit
    static uint8_t const BIT_TXERIF = BIT1;
    //   RXERIF: Receive Error Interrupt Flag bit
    static uint8_t const BIT_RXERIF = BIT0;

    // ESTAT: ETHERNET STATUS REGISTER
    static uint8_t const ESTAT = 0x1D;
    //   INT: INT Interrupt Flag bit
    static uint8_t const BIT_INT = BIT7;
    //   BUFER: Ethernet Buffer Error Status bit
    static uint8_t const BIT_BUFER = BIT6;
    //   LATECOL: Late Collision Error bit
    static uint8_t const BIT_LATECOL = BIT4;
    //   RXBUSY: Receive Busy bit
    static uint8_t const BIT_RXBUSY = BIT2;
    //   TXABRT: Transmit Abort Error bit
    static uint8_t const BIT_TXABRT = BIT1;
    //   CLKRDY: Clock Ready bit(1)
    static uint8_t const BIT_CLKRDY = BIT0;

    // ERDPTL: Read Pointer Low Byte ERDPT<7:0>)
    static uint8_t const ERDPTL = BANK0 | 0x00;

    // ERDPTH: Read Pointer High Byte (ERDPT<12:8>)
    static uint8_t const ERDPTH = BANK0 | 0x01;

    // EWRPTL: Write Pointer Low Byte (EWRPT<7:0>)
    static uint8_t const EWRPTL = BANK0 | 0x02;

    // EWRPTH: Write Pointer High Byte (EWRPT<12:8>)
    static uint8_t const EWRPTH = BANK0 | 0x03;

    // ETXSTL: TX Start Low Byte (ETXST<7:0>)
    static uint8_t const ETXSTL = BANK0 | 0x04;

    // ETXSTH: TX Start High Byte (ETXST<12:8>)
    static uint8_t const ETXSTH = BANK0 | 0x05;

    // ETXNDL: TX End Low Byte (ETXND<7:0>)
    static uint8_t const ETXNDL = BANK0 | 0x06;

    // ETXNDH: TX End High Byte (ETXND<12:8>)
    static uint8_t const ETXNDH = BANK0 | 0x07;

    // EPKTCNT: Ethernet Packet Count
    static uint8_t const EPKTCNT = BANK1 | 0x19;

    // MIREGADR: MII Register Address (MIREGADR<4:0>)
    static uint8_t const MIREGADR = BANK2 | 0x14;

    // MICMD: MII COMMAND REGISTER
    static uint8_t const MICMD = BANK2 | 0x12;
    //   MIISCAN: MII Scan Enable bit
    static uint8_t const BIT_MIISCAN = BIT1;
    //   MIIRD: MII Read Enable bit
    static uint8_t const BIT_MIIRD = BIT0;

    // MISTAT: MII STATUS REGISTER
    static uint8_t const MISTAT = BANK3 | 0x0A;
    //   NVALID: MII Management Read Data Not Valid bit
    static uint8_t const BIT_NVALID = BIT2;
    //   SCAN: MII Management Scan Operation bit
    static uint8_t const BIT_SCAN = BIT1;
    //   BUSY: MII Management Busy bit
    static uint8_t const BIT_BUSY = BIT0;

    // MIRDL: MII Read Data Low Byte (MIRD<7:0>)
    static uint8_t const MIRDL = BANK2 | 0x18;

    // MIRDH: MII Read Data High Byte(MIRD<15:8>)
    static uint8_t const MIRDH = BANK2 | 0x19;

    // MIWRL: MII Write Data Low Byte (MIWR<7:0>)
    static uint8_t const MIWRL = BANK2 | 0x16;

    // MIWRH: MII Write Data High Byte (MIWR<15:8>)
    static uint8_t const MIWRH = BANK2 | 0x17;


    // ECOCON: CLOCK OUTPUT CONTROL REGISTER
    static uint8_t const ECOCON = BANK3 | 0x15;
    //    CLKOUT outputs main clock divided by 8 (3.125 MHz)
    static uint8_t const ECOCON_VAL_CLKOUTDIV8 = 0b101;
    //    CLKOUT outputs main clock divided by 4 (6.25 MHz)
    static uint8_t const ECOCON_VAL_CLKOUTDIV4 = 0b100;
    //    CLKOUT outputs main clock divided by 3 (8.333333 MHz)
    static uint8_t const ECOCON_VAL_CLKOUTDIV3 = 0b011;
    //    CLKOUT outputs main clock divided by 2 (12.5 MHz)
    static uint8_t const ECOCON_VAL_CLKOUTDIV2 = 0b010;
    //    CLKOUT outputs main clock divided by 1 (25 MHz)
    static uint8_t const ECOCON_VAL_CLKOUTDIV1 = 0b001;
    //    CLKOUT is disabled, the pin is driven low
    static uint8_t const ECOCON_VAL_CLKOUTDISABLED = 0b000;    


    // PHCON1: PHY CONTROL REGISTER 1 (use getPHYReg/setPHYReg)
    static uint8_t const PHCON1 = 0x00;
    //   PRST: PHY Software Reset bit
    static uint16_t const BIT_PRST = BIT15;
    //   PLOOPBK: PHY Loopback bit
    static uint16_t const BIT_PLOOPBK = BIT14;
    //   PPWRSV: PHY Power-Down bit
    static uint16_t const BIT_PPWRSV = BIT11;
    //   PDPXMD: PHY Duplex Mode bit
    static uint16_t const BIT_PDPXMD = BIT8;

    // PHCON2: PHY CONTROL REGISTER 2 (use getPHYReg/setPHYReg)
    static uint8_t const PHCON2 = 0x10;
    //   FRCLNK: PHY Force Linkup bit
    static uint16_t const BIT_FRCLNK = BIT14;
    //   TXDIS: Twisted-Pair Transmitter Disable bit
    static uint16_t const BIT_TXDIS = BIT13;
    //   JABBER: Jabber Correction Disable bit
    static uint16_t const BIT_JABBER = BIT10;
    //   HDLDIS: PHY Half-Duplex Loopback Disable bit
    static uint16_t const BIT_HDLDIS = BIT8;

    // PHSTAT1: PHYSICAL LAYER STATUS REGISTER 1 (use getPHYReg/setPHYReg)
    static uint8_t const PHSTAT1 = 0x01;
    //   PFDPX: PHY Full-Duplex Capable bit
    static uint16_t const BIT_PFDPX = BIT12;
    //   PHDPX: PHY Half-Duplex Capable bit
    static uint16_t const BIT_PHDPX = BIT11;
    //   LLSTAT: PHY Latching Link Status bit
    static uint16_t const BIT_LLSTAT = BIT2;
    //   JBSTAT: PHY Latching Jabber Status bit
    static uint16_t const BIT_JBSTAT = BIT1;

    // PHSTAT2: PHYSICAL LAYER STATUS REGISTER 2
    static uint8_t const PHSTAT2 = 0x11;
    //   TXSTAT: PHY Transmit Status bit
    static uint16_t const BIT_TXSTAT = BIT13;
    //   RXSTAT: PHY Receive Status bit
    static uint16_t const BIT_RXSTAT = BIT12;
    //   COLSTAT: PHY Collision Status bit
    static uint16_t const BIT_COLSTAT = BIT11;
    //   LSTAT: PHY Link Status bit (non-latching)
    static uint16_t const BIT_LSTAT = BIT10;
    //   DPXSTAT: PHY Duplex Status bit
    static uint16_t const BIT_DPXSTAT = BIT9;
    //   PLRITY: Polarity Status bit
    static uint16_t const BIT_PLRITY = BIT5;

    // PHID1: PHY Identifier 1
    static uint8_t const PHID1 = 0x02;

    // PHID2: PHY Identifier 2
    static uint8_t const PHID2 = 0x03;

    // PHIE: PHY INTERRUPT ENABLE REGISTER
    static uint8_t const PHIE = 0x12;
    //   PLNKIE: PHY Link Change Interrupt Enable bit
    static uint16_t const BIT_PLNKIE = BIT4;
    //   PGEIE: PHY Global Interrupt Enable bit
    static uint16_t const BIT_PGEIE = BIT1;

    // PHIR: PHY INTERRUPT REQUEST (FLAG) REGISTER
    static uint8_t const PHIR = 0x13;
    //   PLNKIF: PHY Link Change Interrupt Flag bit
    static uint16_t const BIT_PLNKIF = BIT4;
    //   PGIF: PHY Global Interrupt Flag bit
    static uint16_t const BIT_PGIF = BIT2;

    // PHLCON: PHY MODULE LED CONTROL REGISTER
    static uint8_t const PHLCON = 0x14;
    //   LACFG3:LACFG0: LEDA Configuration bits
    static uint16_t const SHIFT_LACFG = BIT8;
    //   LBCFG3:LBCFG0: LEDB Configuration bits
    static uint16_t const SHIFT_LBCFG = BIT4;
    //   LFRQ1:LFRQ0: LED Pulse Stretch Time Configuration bits
    static uint16_t const SHIFT_LFRQ = BIT2;
    //   STRCH: LED Pulse Stretching Enable bit
    static uint16_t const BIT_STRCH = BIT1;
    



  public:

    // RX and TX buffer specifications
    static uint16_t const RXBUFFERLENGTH = 6592;
    static uint16_t const RXBUFFERSTART  = 0x0000;
    static uint16_t const RXBUFFEREND    = RXBUFFERSTART + RXBUFFERLENGTH;
    static uint16_t const RXTXBUFFERGAP  = 2;
    static uint16_t const TXBUFFERSTART  = RXBUFFEREND + RXTXBUFFERGAP;

    static uint16_t const MAXFRAMELENGTH = 1518;

    static uint8_t const MAXLISTENERS = 5;

    enum Mode
    {
      HalfDuplex,
      FullDuplex
    };


    // specialized for ENC28J60 link layer receive frame
    struct RcvFrame : LinkLayerReceiveFrame
    {
      uint16_t status;      // status word (see Table 7-3 - Receive status vectors, bits 16:31)
      uint16_t dataPos;     // position in RX buffer of frame data

      RcvFrame(ENC28J60* parent)
        : LinkLayerReceiveFrame(), m_parent(parent), m_currIndex(0)
      {
      }

      void readReset()
      {
        m_currIndex = 0;
      }

      uint8_t readByte(uint16_t index)
      {
        uint8_t r;
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
          m_parent->beginReadMemory(dataPos + index);
          r = m_parent->readByte();
          m_parent->endReadMemory();
        }
        return r;
      }

      uint8_t readByte()
      {
        return readByte(m_currIndex++);
      }

      // assume big-endian (that is the network byte order)
      uint16_t readWord(uint16_t index)
      {
        uint16_t r;
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
          m_parent->beginReadMemory(dataPos + index);
          uint8_t lo = m_parent->readByte();
          uint8_t hi = m_parent->readByte();
          r = ((uint16_t)hi << 8) | lo;
          m_parent->endReadMemory();
        }
        return r;        
      }

      // assume big-endian (that is the network byte order)
      uint16_t readWord()
      {
        uint16_t i = m_currIndex;
        m_currIndex += 2;
        return readWord(i);
      }

      void readBlock(uint16_t index, void* dstBuffer, uint16_t length)
      {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
          m_parent->beginReadMemory(dataPos + index);
          m_parent->readBlock(dstBuffer, length);
          m_parent->endReadMemory();
        }
      }

      void readBlock(void* dstBuffer, uint16_t length)
      {
        readBlock(m_currIndex, dstBuffer, length);
        m_currIndex += length;
      }

      // releases ENC24J60 RX buffer for this frame
      void releaseFrame()
      {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
          // update ERXRDPT (release memory in RX buffer)
          m_parent->setReg(ERXRDPTL, m_parent->m_nextRXPacketPtr & 0xFF);
          m_parent->setReg(ERXRDPTH, (m_parent->m_nextRXPacketPtr >> 8) & 0xFF);
        }
      }

    private:

      ENC28J60* m_parent;
      uint16_t  m_currIndex;
    };




  public:

    ENC28J60(Pin const* interruptPin, HardwareSPIMaster* spi, LinkAddress const& address, Mode mode)
    {      
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {

        // configuration
        m_spi          = spi;
        m_interruptPin = interruptPin;
        m_address      = address;
        m_mode         = mode;

        // status
        m_available       = false;
        m_frameReceived   = 0;
        m_frameSent       = false;
        m_nextRXPacketPtr = RXBUFFERSTART;
        //m_linkUp          = false;

        // pins
        interruptPin->modeInput();
        interruptPin->writeLow();

        // system reset (SPI reset)
        _delay_us(150);
        CMD_systemReset();
        delay_ms(50);  // fix errata silicon 2 (rev. B7)
        
        // setup RX buffer Start and End
        setReg(ERXSTL, RXBUFFERSTART & 0xFF);
        setReg(ERXSTH, (RXBUFFERSTART >> 8) & 0xFF);
        setReg(ERXNDL, RXBUFFEREND & 0xFF);
        setReg(ERXNDH, (RXBUFFEREND >> 8) & 0xFF);
        
        // setup RX start reading pointer
        setReg(ERXRDPTL, RXBUFFERSTART & 0xFF);
        setReg(ERXRDPTH, (RXBUFFERSTART >> 8) & 0xFF);
        
        // setup receive filters
        //   - if unicast
        //   - OR multicast
        //   - OR broadcast
        //   - AND CRC check ok
        setReg(ERXFCON, BIT_UCEN | BIT_CRCEN | BIT_MCEN | BIT_BCEN);

        // wait for clock is ready
        while ((getReg(ESTAT) & BIT_CLKRDY) == 0)
          delay_ms(1);
          
        // set MAC address (init step 9)
        setReg(MAADR1, m_address[0]);
        setReg(MAADR2, m_address[1]);
        setReg(MAADR3, m_address[2]);
        setReg(MAADR4, m_address[3]);
        setReg(MAADR5, m_address[4]);
        setReg(MAADR6, m_address[5]);

        // verify MAC address
        if (getReg(MAADR1) != m_address[0] || getReg(MAADR2) != m_address[1] || getReg(MAADR3) != m_address[2] || getReg(MAADR4) != m_address[3] || getReg(MAADR5) != m_address[4] || getReg(MAADR6) != m_address[5])
          return; // FAIL!!
        
        // setup MACON1 register (init step 1)
        //   - enable MAC to receive frames
        //   - TXPAUS and RXPAUS needed by fullduplex
        setReg(MACON1, BIT_MARXEN | BIT_TXPAUS | BIT_RXPAUS);

        // setup MACON2 (end reset, undocumented!!)
        //setReg(MACON2, 0x00);
        //delay_ms(100);

        // setup MACON3 register (init step 2)
        //   - zero pad to 64 bytes and auto CRC
        //   - frame length check
        //   - enable/disable full duplex
        setReg(MACON3, (0b001 << SHIFT_PADCFG) | BIT_TXCRCEN | (m_mode == FullDuplex? BIT_FULDPX : 0));

        // setup MACON4 register (init step 3)
        //   - just for half-duplex IEEE 802.3 conformance
        setReg(MACON4, BIT_DEFER);

        // setup maximum frame length (init step 4)
        setReg(MAMXFLL, MAXFRAMELENGTH & 0xFF);
        setReg(MAMXFLH, (MAXFRAMELENGTH >> 8) & 0xFF);

        // setup back-to-back inter-packet gap (init step 5)
        setReg(MABBIPG, m_mode == FullDuplex? 0x15 : 0x12);

        // setup non-back-to-back inter-packet gap (init step 6)
        setReg(MAIPGL, 0x12);
        if (m_mode == HalfDuplex)
          setReg(MAIPGH, 0x0C);

        // disable clock out pin
        setReg(ECOCON, ECOCON_VAL_CLKOUTDISABLED);

        // wait PHY reset ends (actually necessary?)
        while (getPHYReg(PHCON1) & BIT_PRST);

        // setup full or half duplex in PHY
        setPHYReg(PHCON1, (m_mode == FullDuplex? BIT_PDPXMD : 0));

        // disable lookback
        setPHYReg(PHCON2, BIT_HDLDIS);

        // set autoincrement for read and write buffers
        bitFieldSet(ECON2, BIT_AUTOINC);

        // enable interrupts
        //   - global interrupt enabled
        //   - Receive Packet Pending
        //   - transmission has ended
        //   - PHY link change
        setReg(EIE, BIT_INTIE | BIT_PKTIE | BIT_TXIE | BIT_LINKIE);

        // enable PHY interrupt
        //setPHYReg(PHIE, BIT_PLNKIE | BIT_PGEIE);

        // setup interrupt handler
        ExtInterrupt::attach(interruptPin->EXT_INT, this, ExtInterrupt::EXTINT_FALLING);

        // enable RX
        bitFieldSet(ECON1, BIT_RXEN);

        delay_ms(100);

        m_available = true;
      }      
    }


    bool isAvailable()
    {
      return m_available;
    }


    uint8_t getRevisionID()
    {
      return getReg_noIRQ(EREVID);
    }

    
    void addListener(ILinkLayerListener* listener)
    {
      m_listeners.push_back(listener);
    }

    
  private:

    void extInterrupt()
    {

      uint16_t vvv = m_interruptPin->read();
      cout << "int " << (uint16_t)getReg(EIR) << " " << vvv << endl;

      if (!m_available)
        return;
      
      // disable interrupts
      bitFieldClear(EIE, BIT_INTIE);

      while (true)
      {
        uint8_t eir = getReg(EIR);

        if (eir & BIT_PKTIF)
        {
          // packet received
          ++m_frameReceived;
          //m_frameReceived = getReg(EPKTCNT);

          // decrease packets count
          bitFieldSet(ECON2, BIT_PKTDEC); // this set also EIR.PKTIF=0 when EPKTCNT=0
        }
        else if (eir & BIT_TXIF)
        {
          // packet transmitted
          m_frameSent = true;

          // clear flag
          bitFieldClear(EIR, BIT_TXIF);
        }/*
        else if (eir & BIT_LINKIF)
        {
          // link change
          getPHYReg(PHIR);  // reading PHIR will clear PLINKIF
          m_linkUp = getPHYReg(PHSTAT2) & BIT_LSTAT;
        }*/
        else
          break;
      }

      // enable interrupts
      bitFieldSet(EIE, BIT_INTIE);
    }


  public:

    ILinkLayer::SendResult sendFrame(LinkLayerSendFrame const* frame)
    {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        m_frameSent = false;

        beginWriteMemory(TXBUFFERSTART);

        // write Packet Control Byte
        writeByte(0x00);

        // write destination address
        writeBlock(frame->destAddress.data(), 6);

        // write source address
        writeBlock(frame->srcAddress.data(), 6);

        // write Type/Length field. Store MSB first (bigendian)
        writeByte((frame->type_length >> 8) & 0xFF);
        writeByte(frame->type_length & 0xFF);

        // write data
        DataList const* data = frame->dataList;
        while (data != NULL)
        {
          writeBlock(data->data, data->length);
          data = data->next;
        }        

        endWriteMemory();

        // set Start position in buffer memory
        setReg(ETXSTL, TXBUFFERSTART & 0xFF);
        setReg(ETXSTH, (TXBUFFERSTART >> 8) & 0xFF);

        // set End position in buffer memory
        uint16_t bufend = TXBUFFERSTART + 1 + 6 + 6 + 2 + frame->dataList->calcLength() - 1;
        setReg(ETXNDL, bufend & 0xFF);
        setReg(ETXNDH, (bufend >> 8) & 0xFF);

        // clear TX interrupt flag
        bitFieldClear(EIR, BIT_TXIF);

        // start transmission
        bitFieldSet(ECON1, BIT_TXRTS);
      }

      // wait TX
      while (!m_frameSent);

      // get result
      if (getReg_noIRQ(ESTAT) & BIT_TXABRT)
        return SendFail;
      else
        return SendOK;
    }


    // applications must call frame->release() to release RX buffer memory
    bool recvFrame(RcvFrame* frame)
    {
      if (m_frameReceived == 0)
        return false;
      
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        uint16_t RXPtr = m_nextRXPacketPtr;
        beginReadMemory(m_nextRXPacketPtr);
        
        // read next packet pointer
        uint8_t lo = readByte();
        uint8_t hi = readByte();
        m_nextRXPacketPtr = lo | ((uint16_t)hi << 8);

        // read status
        //   read Received Byte Count
        lo = readByte();
        hi = readByte();
        uint16_t byteCount = lo | ((uint16_t)hi << 8);
        //   read up 16 bit of status
        lo = readByte();
        hi = readByte();
        frame->status = lo | ((uint16_t)hi << 8);

        // read destination address
        readBlock(frame->destAddress.data(), 6);

        // read source address
        readBlock(frame->srcAddress.data(), 6);

        // read Type/Length field (bigendian)
        hi = readByte();
        lo = readByte();
        frame->type_length = ((uint16_t)hi << 8) | lo;

        // store data position and length
        frame->dataPos = RXPtr + 20;
        frame->dataLength = byteCount - 6 - 6 - 2 - 4; // decrement by dst_addr, src_addr, type/length, CRC32
        
        endReadMemory();

        --m_frameReceived;
      }

      //cout << "frame->dataPos = " << frame->dataPos << endl;

      for (uint8_t i = 0; i != m_listeners.size(); ++i)
      {
        frame->readReset();
        if (m_listeners[i]->processLinkLayerFrame(frame))
          return false; // message processed
      }

      return true;
    }


    // called instead of recvFrame with parameters in order to just receve and process frames using listeners
    void recvFrame()
    {
      RcvFrame frame(this);
      if (!recvFrame(&frame))  // actually processed as "listener"
        frame.releaseFrame();  // if no one processed the frame, release it
    }


    LinkAddress const& getAddress() const
    {
      return m_address;
    }


    bool linkUp()
    {
      //return m_linkUp;
      return getPHYReg_noIRQ(PHSTAT2) & BIT_LSTAT;
    }


  private:
    
    
    // native SPI command: RCR
    uint8_t CMD_readControlRegister(uint8_t reg)
    {
      m_spi->select();
      m_spi->write((0b000 << 5) | reg);
      uint8_t valueToReturn = m_spi->read();
      m_spi->deselect();
      return valueToReturn;
    }

    // native SPI command: BFS
    // only set "1" bits
    void CMD_bitFieldSet(uint8_t reg, uint8_t value)
    {
      m_spi->select();
      m_spi->write((0b100 << 5) | reg);
      m_spi->write(value);
      m_spi->deselect();
    }

    // native SPI command: BFC
    // only clear "1" bits
    void CMD_bitFieldClear(uint8_t reg, uint8_t value)
    {
      m_spi->select();
      m_spi->write((0b101 << 5) | reg);
      m_spi->write(value);
      m_spi->deselect();
    }

    // native SPI command: WCR
    void CMD_writeControlRegister(uint8_t reg, uint8_t value)
    {
      m_spi->select();
      m_spi->write((0b010 << 5) | reg);
      m_spi->write(value);
      m_spi->deselect();
    }

    // native SPI command: SRC
    void CMD_systemReset()
    {
      m_spi->select();
      m_spi->write(0xFF);
      m_spi->deselect();
    }

    // native SPI command: RBM - begin
    // if address=0xFFFF, do not set, just continue from last reading position
    void beginReadMemory(uint16_t address)
    {
      if (address != 0xFFFF)
      {
        // check wrap (in this case it must be done manually)
        if (address > RXBUFFEREND)
        {
          //cout << "wrap: " << address << endl;
          address = RXBUFFERSTART + (address - RXBUFFEREND) - 1;
        }
        //cout << "address:" << address << endl;
        // set reading position
        setReg(ERDPTL, address & 0xFF);
        setReg(ERDPTH, (address >> 8) & 0xFF);
      }
      m_spi->select();
      m_spi->write(0b00111010);
    }

    // native SPI command: RBM - continue
    // assume AUTOINC is enabled
    uint8_t readByte()
    {
      return m_spi->read();
    }

    // native SPI command: RBM - continue
    // assume AUTOINC is enabled
    void readBlock(void* buffer, uint16_t length)
    {
      uint8_t* buf = static_cast<uint8_t*>(buffer);
      while (length--)
        *buf++ = m_spi->read();
    }

    // native SPI command: RBM - end
    void endReadMemory()
    {
      m_spi->deselect();
    }

    // native SPI command: WBM
    // if address=0xFFFF, do not set, just continue from last writing position
    void beginWriteMemory(uint16_t address)
    {
      if (address != 0xFFFF)
      {
        // set writing position
        setReg(EWRPTL, address & 0xFF);
        setReg(EWRPTH, (address >> 8) & 0xFF);
      }
      m_spi->select();
      m_spi->write(0b01111010);
    }

    // native SPI command: WBM - continue
    // assume AUTOINC is enabled
    void writeByte(uint8_t value)
    {
      m_spi->write(value);
    }

    // native SPI command: WBM - continue
    // assume AUTOINC is enabled
    void writeBlock(void const* buffer, uint16_t length)
    {
      uint8_t const* buf = static_cast<uint8_t const*>(buffer);
      while (length--)
        m_spi->write(*buf++);
    }

    // native SPI command: WBM - end
    void endWriteMemory()
    {
      m_spi->deselect();
    }

    // bank: 0..3
    void selectBank(uint8_t bank)
    {
      CMD_bitFieldClear(ECON1, 0b11 << SHIFT_BSEL);
      CMD_bitFieldSet(ECON1, bank << SHIFT_BSEL);
    }

    // 7 6 5 4 3 2 1 0
    // * * * = bank (0..3)
    //       * * * * * = register
    void setReg(uint8_t reg, uint8_t value)
    {
      uint8_t rr = reg & 0b11111;
      if (rr < 0x1B)  // registers >=0x1B doesn't require bank change
      {
        // select bank
        selectBank(reg >> 5);
      }
      // set register
      CMD_writeControlRegister(rr, value);
    }
    
    
    void setReg_noIRQ(uint8_t reg, uint8_t value)
    {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        setReg(reg, value);
      }
    }
    
    
    // 7 6 5 4 3 2 1 0
    // * * * = bank (0..3)
    //       * * * * * = register
    uint8_t getReg(uint8_t reg)
    {
      uint8_t rr = reg & 0b11111;
      if (rr < 0x1B)  // registers >=0x1B doesn't require bank change
      {
        // select bank
        selectBank(reg >> 5);
      }
      // set register
      return CMD_readControlRegister(rr);
    }
    
    
    uint8_t getReg_noIRQ(uint8_t reg)
    {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        return getReg(reg);
      }
      return 0; // avoid compiler warning
    }


    uint16_t getPHYReg(uint8_t reg)
    {
      setReg(MIREGADR, reg);
      bitFieldSet(MICMD, BIT_MIIRD);
      delayMicroseconds(11);
      while (getReg(MISTAT) & BIT_BUSY);
      bitFieldClear(MICMD, BIT_MIIRD);
      return getReg(MIRDL) | ((uint16_t)getReg(MIRDH) << 8);
    }
    
    
    uint16_t getPHYReg_noIRQ(uint8_t reg)
    {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        return getPHYReg(reg);
      }
      return 0; // avoid compiler warning
    }


    void setPHYReg(uint8_t reg, uint16_t value)
    {
      setReg(MIREGADR, reg);
      setReg(MIWRL, value & 0xFF);
      setReg(MIWRH, (value >> 8) & 0xFF);
      delayMicroseconds(11);
      while (getReg(MISTAT) & BIT_BUSY);
    }


    void bitFieldSet(uint8_t reg, uint8_t value)
    {
      uint8_t rr = reg & 0b11111;
      if (rr < 0x1B)  // registers >=0x1B doesn't require bank change
      {
        // select bank
        selectBank(reg >> 5);
      }
      CMD_bitFieldSet(rr, value);
    }

    void bitFieldClear(uint8_t reg, uint8_t value)
    {
      uint8_t rr = reg & 0b11111;
      if (rr < 0x1B)  // registers >=0x1B doesn't require bank change
      {
        // select bank
        selectBank(reg >> 5);
      }
      CMD_bitFieldClear(rr, value);
    }

    
  private:

    // configuration
    HardwareSPIMaster* m_spi;
    Pin const*         m_interruptPin;
    bool               m_available;
    LinkAddress        m_address;    // the MAC address
    Mode               m_mode;
    Array<ILinkLayerListener*, MAXLISTENERS> m_listeners; // upper layer listeners
    
    // status
    uint8_t volatile   m_frameReceived; // number of frames ready
    bool volatile      m_frameSent;
    uint16_t           m_nextRXPacketPtr;
    //bool volatile      m_linkUp;        // true=linkup false=linkdown

  };

  

} // namespace fdv




#endif /* FDV_ENC28J60_H_ */