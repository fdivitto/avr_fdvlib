/*
** Invio di un messaggio normale:

1) A deve mandare il messaggio M a B
2) A manda il messaggio M in broadcast (con messageID e destinatario B nel messaggio stesso)
3) tutti i riceventi, eccetto B e quelli che hanno già ricevuto M, attendono per un tempo T1
4) i riceventi che avevano già ricevuto M, lo cancellano, senza attendere T1

CASO 1: B riceve il messaggio M

  5_1) B invia subito un ACK per M
  6_1) tutti i riceventi che sono in attesa T1 cancellano il messaggio M

CASO 2: B non riceve il messaggio

  5_2) tutti i riceventi che sono in attesa T1, dopo T1 e un tempo random, reinviano M
  6_2) si ritorna al passo #3



** Invio di un messaggio ACK:

1) B deve mandare un messaggio ACK ad A
2) B manda il messaggio ACK in broadcast (con messageID e destinatario A nel messaggio stesso)
3) tutti i riceventi, eccetto A e quelli che hanno già ricevuto ACK, attendono per un tempo random e reinviano ACK
*/

/*
*/


// 2010/2013 by Fabrizio Di Vittorio (fdivitto@tiscali.it)


// note: this driver is vulnerable to replay attack. Replay attack defense must be implemented at the upper layer.


#ifndef FDV_MRF24J40_H
#define FDV_MRF24J40_H

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdarg.h>

#include <avr/interrupt.h> 
#include <util/atomic.h>


// fdv includes
#include "../fdv_generic/fdv_memory.h"
#include "../fdv_generic/fdv_timesched.h"
#include "../fdv_generic/fdv_pin.h"
#include "../fdv_generic/fdv_spi.h"
#include "../fdv_generic/fdv_interrupt.h"
#include "../fdv_generic/fdv_random.h"
#include "../fdv_network/fdv_TCPIP.h"



namespace fdv
{

  
  class MRF24J40 : public ILinkLayer
  {

    private:
    
    static uint8_t const BIT0 = 0x01;
    static uint8_t const BIT1 = 0x02;
    static uint8_t const BIT2 = 0x04;
    static uint8_t const BIT3 = 0x08;
    static uint8_t const BIT4 = 0x10;
    static uint8_t const BIT5 = 0x20;
    static uint8_t const BIT6 = 0x40;
    static uint8_t const BIT7 = 0x80;
    
    // TXPEND: TX DATA PENDING REGISTER
    static uint16_t const REG_TXPEND = 0x21;
    //   MLIFS: Minimum Long Interframe Spacing bits
    static uint8_t const SHIFT_MLIFS = 2;
    
    // TXTIME: TX TURNAROUND TIME REGISTER
    static uint16_t const REG_TXTIME = 0x27;
    //   TURNTIME: Turnaround Time bits
    static uint8_t const SHIFT_TURNTIME = 4;
    
    // SOFTRST: SOFTWARE RESET REGISTER
    static uint16_t const REG_SOFTRST = 0x2A;
    //   RSTPWR : Power Management Reset
    static uint8_t const BIT_RSTPWR   = BIT2;
    //   RSTBB : Baseband Reset
    static uint8_t const BIT_RSTBB    = BIT1;
    //   RSTMAC : MAC Reset
    static uint8_t const BIT_RSTMAC   = BIT0;
    
    // PACON2: POWER AMPLIFIER CONTROL 2 REGISTER
    static uint16_t const REG_PACON2  = 0x18;
    //   FIFOEN: FIFO Enable  (1=enabled)
    static uint8_t const BIT_FIFOEN   = BIT7;
    //   TXONTS: Transmitter Enable On Time Symbol (4 bits)
    static uint8_t const SHIFT_TXONTS = 2;
    
    // TXSTBL: TX STABILIZATION REGISTER
    static uint16_t const REG_TXSTBL  = 0x2E;
    //   RFSTBL : VCO Stabilization Period (4 bits)
    static uint8_t const SHIFT_RFSTBL = 4;
    //   MSIFS : Minimum Short Interframe Spacing bits (4 bits)
    static uint8_t const SHIFT_MSIFS = 0;
    
    // RFCTL: RF MODE CONTROL REGISTER
    static uint16_t const REG_RFCTL = 0x36;
    //   RFRST: RF State Machine Reset bit
    static uint8_t const BIT_RFRST  = BIT2;
    
    // RXFLUSH: RECEIVE FIFO FLUSH REGISTER
    static uint16_t const REG_RXFLUSH = 0x0D;
    //   RXFLUSH: Reset Receive FIFO Address Pointer
    static uint8_t const BIT_RXFLUSH  = BIT0;
    
    // RXMCR: RECEIVE MAC CONTROL REGISTER
    static uint16_t const REG_RXMCR   = 0x00;
    //   PROMI: Promiscuous Mode (1=Receive all packet with good CRC)
    static uint8_t const BIT_PROMI    = BIT0;
    //   ERRPKT: Packet Error Mode (1=Accept all packages including CRC error)
    static uint8_t const BIT_ERRPKT   = BIT1;
    //   COORD: Coordinator (1=Set device as coordinator)
    static uint8_t const BIT_COORD    = BIT2;
    //   PANCOORD: PAN Coordinator (1=Set device as PAN coordinator)
    static uint8_t const BIT_PANCOORD = BIT3;
    //   NOACKRSP: Automatic Ack Response (1=Disable automatic ACK response)
    
    // SADRL: SHORT ADDRESS LOW BYTE REGISTER ... // SADRH: SHORT ADDRESS HIGH BYTE REGISTER
    static uint16_t const REG_SADRL = 0x03;
    static uint16_t const REG_SADRH = 0x04;
    
    // PANIDL: PAN ID LOW BYTE REGISTER ... // PANIDH: PAN ID HIGH BYTE REGISTER
    static uint16_t const REG_PANIDL = 0x01;
    static uint16_t const REG_PANIDH = 0x02;
    
    // EADR0: EXTENDED ADDRESS 0 REGISTER ... EADR7: EXTENDED ADDRESS 7 REGISTER
    static uint16_t const REG_EADR0 = 0x05;
    static uint16_t const REG_EADR1 = 0x06;
    static uint16_t const REG_EADR2 = 0x07;
    static uint16_t const REG_EADR3 = 0x08;
    static uint16_t const REG_EADR4 = 0x09;
    static uint16_t const REG_EADR5 = 0x0A;
    static uint16_t const REG_EADR6 = 0x0B;
    static uint16_t const REG_EADR7 = 0x0C;
    
    // RFCON0/RFCTRL0: RF CONTROL 0 REGISTER
    static uint16_t const REG_RFCON0   = 0x200;
    static uint16_t const REG_RFCTRL0  = REG_RFCON0;
    //   CHANNEL (4 bits)
    static uint8_t const SHIFT_CHANNEL = 4;
    //   RFOPT (4 bits)
    static uint8_t const SHIFT_RFOPT   = 0;
    
    // RFCON1: RF CONTROL 1 REGISTER (VCO Optimize Control)
    static uint16_t const REG_RFCON1 = 0x201;
    
    
    // RFCON2/RFCTRL2: RF CONTROL 2 REGISTER
    static uint16_t const REG_RFCON2  = 0x202;
    static uint16_t const REG_RFCTRL2 = REG_RFCON2;
    //   PLLEN : PLL enable
    static uint8_t const BIT_PLLEN    = BIT7;
    
    // RFCON3/RFCTRL3: RF CONTROL 3 REGISTER
    static uint16_t const REG_RFCON3  = 0x203;
    static uint16_t const REG_RFCTRL3 = REG_RFCON3;
    //   TXPWRL :  Large Scale Control for TX Power
    static uint8_t const VAL_TXPWRL_30DB = 3 << 6; // -30 dB
    static uint8_t const VAL_TXPWRL_20DB = 2 << 6; // -20 dB
    static uint8_t const VAL_TXPWRL_10DB = 1 << 6; // -10 dB
    static uint8_t const VAL_TXPWRL_00DB = 0 << 6; //   0 dB
    //   TXPWRS : Small Scale Control for TX Power
    static uint8_t const VAL_TXPWRS_6p3DB = 7 << 3; // -6.3 dB
    static uint8_t const VAL_TXPWRS_4p9DB = 6 << 3; // -4.9 dB
    static uint8_t const VAL_TXPWRS_3p7DB = 5 << 3; // -3.7 dB
    static uint8_t const VAL_TXPWRS_2p8DB = 4 << 3; // -2.8 dB
    static uint8_t const VAL_TXPWRS_1p9DB = 3 << 3; // -1.9 dB
    static uint8_t const VAL_TXPWRS_1p2DB = 2 << 3; // -1.2 dB
    static uint8_t const VAL_TXPWRS_0p5DB = 1 << 3; // -0.5 dB
    static uint8_t const VAL_TXPWRS_0p0DB = 0 << 3; //    0 dB
    
    // RFCON6/RFCTRL6: RF CONTROL 6 REGISTER
    static uint16_t const REG_RFCON6  = 0x206;
    static uint16_t const REG_RFCTRL6 = REG_RFCON6;
    //   TXFIL : TX Filter Control
    static uint8_t const BIT_TXFIL    = BIT7;
    //   20MRECVR : 20MHz Clock Recovery Control
    static uint8_t const BIT_20MRECVR = BIT4;
    
    // RFCON7: RF CONTROL 7 REGISTER
    static uint16_t const REG_RFCON7 = 0x207;
    //   SLPCLKSEL : Sleep Clock Selection (2 bits)
    static uint8_t const VAL_SLPCLKSEL_100KHZ = 2 << 6; // 100kHz internal oscillator
    static uint8_t const VAL_SLPCLKSEL_32KHZ  = 1 << 6; // 32kHz external crystal oscillator
    
    // RFCON8/RFCTRL8: RF CONTROL 8 REGISTER
    static uint16_t const REG_RFCON8  = 0x208;
    static uint16_t const REG_RFCTRL8 = REG_RFCON8;
    //   RFVCO : VCO Control
    static uint8_t const BIT_RFVCO    = BIT4;
    
    // SLPCON1: SLEEP CLOCK CONTROL 1 REGISTER
    static uint16_t const REG_SLPCON1    = 0x220;
    //   CLKOUTEN : CLKOUT Pin Enable
    static uint8_t const BIT_CLKOUTEN    = BIT5;
    //   SLPCLKDIV : Sleep Clock Divisor (5 bits)
    static uint8_t const SHIFT_SLPCLKDIV = 0;
    
    
    // BBREG0: BASEBAND 0 REGISTER
    static uint16_t const REG_BBREG0 = 0x38;
    //   TURBO: Turbo Mode Enable bit (1 = Turbo mode (625 kbps), IEEE 802.15.4TM mode (250 kbps))
    static uint8_t const VAL_TURBO   = BIT0;
    
    
    // BBREG1: BASEBAND 1 REGISTER
    static uint16_t const REG_BBREG1  = 0x39;
    //    RXDECINV : RX Decode Inversion bit
    static uint8_t const BIT_RXDECINV = BIT2;  //  RX decode symbol sign inverted
    
    
    // BBREG2: BASEBAND 2 REGISTER
    static uint16_t const REG_BBREG2   = 0x3A;
    //    CCACSTH : Clear Channel Assessment (CCA) Carrier Sense (CS) Threshold
    static uint8_t const SHIFT_CCACSTH = 2;
    //    CCAMODE : Clear Channel Assessment (CCA) Mode
    static uint8_t const VAL_CCAMODE_1 = 2 << 6; // CCA Mode 1: Energy above threshold.
    static uint8_t const VAL_CCAMODE_2 = 1 << 6; // CCA Mode 2: Carrier sense only.
    static uint8_t const VAL_CCAMODE_3 = 3 << 6; // CCA Mode 3: Carrier sense with energy above threshold.
    
    
    // BBREG3: BASEBAND 3 REGISTER
    static uint16_t const REG_BBREG3 = 0x3B;
    //   PREVALIDTH: Preamble Search Energy Valid Threshold bits
    static uint8_t const VAL_PREVALIDTH_IEEE_802_15_4 = 13 << 4;  // IEEE 802.15.4TM (250 kbps) optimized value (default)
    static uint8_t const VAL_PREVALIDTH_TURBO         = 3 << 4;  // Turbo mode (625 kbps) optimized value
    //   PREDETTH: Preamble Search Energy Detection Threshold bits
    static uint8_t const SHIFT_PREDETTH = 1;
    
    
    // BBREG4: BASEBAND 4 REGISTER
    static uint16_t const REG_BBREG4 = 0x3C;
    //   CSTH: Carrier Sense Threshold bits
    static uint8_t const VAL_CSTH_IEEE_802_15_4 = 4 << 5;   // IEEE 802.15.4TM (250 kbps) optimized value (default)
    static uint8_t const VAL_CSTH_TURBO         = 2 << 5;   // Turbo mode (625 kbps) optimized value
    //   PRECNT: Preamble Counter Threshold bits
    static uint8_t const SHIFT_PRECNT = 2;
    
    
    // BBREG6: BASEBAND 6 REGISTER
    static uint16_t const REG_BBREG6   = 0x3E;
    //   RSSIMODE2 : 1=Calc RSSI for each received packet.
    static uint8_t const BIT_RSSIMODE2 = BIT6;
    
    // CCAEDTH/RSSITHCCA: ENERGY DETECTION THRESHOLD FOR CCA REGISTER
    static uint16_t const REG_CCAEDTH   = 0x3F;
    static uint16_t const REG_RSSITHCCA = REG_CCAEDTH;
    
    // TXNCON: TRANSMIT NORMAL FIFO CONTROL REGISTER
    static uint16_t const REG_TXNCON    = 0x1B;
    //   FPSTAT: Frame Pending Status bit
    static uint8_t const BIT_FPSTAT     = BIT4;
    //   TXNACKREQ: TX Normal FIFO Acknowledgement frame expected
    static uint16_t const BIT_TXNACKREQ = BIT2;
    //   TXNSECEN: TX Normal FIFO Security Enabled bit
    static uint8_t const BIT_TXNSECEN   = BIT1;
    //   TXNTRIG: Transmit Frame in TX Normal FIFO
    static uint16_t const BIT_TXNTRIG   = BIT0;
    
    // TXSTAT: TX MAC STATUS REGISTER
    static uint16_t const REG_TXSTAT    = 0x24;
    //   TXNSTAT : TX Normal FIFO Release Status
    static uint8_t const BIT_TXNSTAT    = BIT0;  // 1=failed  0=succeeded
    //   TXNRETRY: TX Normal FIFO Retry Times bits
    static uint8_t const SHIFT_TXNRETRY = 6;
    //   CCAFAIL: Clear Channel Assessment (CCA) Status of Last Transmission bit
    static uint8_t const BIT_CCAFAIL    = BIT5;
    
    // INTCON: INTERRUPT CONTROL REGISTER
    static uint16_t const REG_INTCON   = 0x32;
    //   SECIE : Security Key Request Interrupt Enable
    static uint8_t const BIT_SECIE     = BIT4;
    //   RXIE  : RX FIFO Reception Interrupt Enable
    static uint8_t const BIT_RXIE      = BIT3;
    //   TXNIE : TX Normal FIFO Transmission Interrupt Enable
    static uint8_t const BIT_TXNIE     = BIT0;
    //   HSYMTMRIE : Half Symbol Timer Interrupt
    static uint8_t const BIT_HSYMTMRIE = BIT5;
    
    // INTSTAT: INTERRUPT STATUS REGISTER
    static uint16_t const REG_INTSTAT  = 0x31;
    //   SECIF : Security Key Request Interrupt
    static uint8_t const BIT_SECIF     = BIT4;
    //   RXIF  : RX FIFO Reception Interrupt
    static uint8_t const BIT_RXIF      = BIT3;
    //   TXNIF  : TX Normal FIFO Release Interrupt
    static uint8_t const BIT_TXNIF     = BIT0;
    //   HSYMTMRIF : Half Symbol Timer Interrupt
    static uint8_t const BIT_HSYMTMRIF = BIT5;
    
    // TESTMODE: TEST MODE REGISTER
    static uint16_t const REG_TESTMODE      = 0x022F;
    //   TESTMODE : Test Mode bits
    static uint8_t const SHIFT_TESTMODE = 0;
    //   RSSIWAIT : RSSI State Machine Parameter bits
    static uint8_t const SHIFT_RSSIWAIT = 3;
    
    // TRISGPIO: GPIO PIN DIRECTION REGISTER
    static uint16_t const REG_TRISGPIO = 0x34;
    
    // GPIO: GPIO PORT REGISTER
    static uint16_t const REG_GPIO = 0x33;

    // HSYMTMRL: HALF SYMBOL TIMER LOW BYTE REGISTER
    static uint16_t const REG_HSYMTMRL = 0x28;
    
    
    // HSYMTMRH: HALF SYMBOL TIMER HIGH BYTE REGISTER
    static uint16_t const REG_HSYMTMRH = 0x29;
    
    
    // SLPCON0: SLEEP CLOCK CONTROL 0 REGISTER
    static uint16_t const REG_SLPCON0 = 0x211;
    //   INTEDGE : Interrupt Edge Polarity
    static uint8_t const BIT_INTEDGE  = BIT1;  // 1=rising edge  0=falling edge
    
    
    // RXSR: RX MAC STATUS REGISTER (ADDRESS: 0x30)
    static uint16_t const REG_RXSR     = 0x30;
    //    UPSECERR: MIC Error in Upper Layer Security Mode bit
    static uint8_t const BIT_UPSECERR  = BIT6;   // 1 = MIC error occurred. Write ‘1’ to clear
    //    SECDECERR: Security Decryption Error
    static uint8_t const BIT_SECDECERR = BIT2;  // 1 = Security decryption error occurred
    
    
    // SECISR: undocumented but used in MRF24J40.c to test decryption
    static uint16_t const REG_SECISR = 0x2F;
    
    
    // SECCON0: SECURITY CONTROL 0 REGISTER
    static uint16_t const REG_SECCON0    = 0x2C;
    //   SECIGNORE : RX Security Decryption Ignore bit
    static uint8_t const BIT_SECIGNORE   = BIT7;
    //   SECSTART: RX Security Decryption Start bit
    static uint8_t const BIT_SECSTART    = BIT6;
    //   RXCIPHER: RX FIFO Security Suite Select bits
    static uint8_t const SHIFT_RXCIPHER  = 3;
    //   TXNCIPHER: TX Normal FIFO Security Suite Select bits
    static uint8_t const SHIFT_TXNCIPHER = 0;
    //   Security suite
    static uint8_t const CIPHER_NONE            = 0;
    static uint8_t const CIPHER_AES_CTR         = 1;
    static uint8_t const CIPHER_AES_CCM_128     = 2;
    static uint8_t const CIPHER_AES_CCM_64      = 3;
    static uint8_t const CIPHER_AES_CCM_32      = 4;
    static uint8_t const CIPHER_AES_CBC_MAC_128 = 5;
    static uint8_t const CIPHER_AES_CBC_MAC_64  = 6;
    static uint8_t const CIPHER_AES_CBC_MAC_32  = 7;


    // SECCON1: SECURITY CONTROL 1 REGISTER
    static uint16_t const REG_SECCON1 = 0x2D;
    // DISDEC: Disable Decryption Function bit (1 = Will not generate a security interrupt if security enabled bit is set in the MAC header)
    static uint8_t const BIT_DISDEC   = BIT1;
    // DISENC: Disable Encryption Function bit (1 = Will not encrypt packet if transmit security is enabled)
    static uint8_t const BIT_DISENC   = BIT0;
    
    
    // TX Normal FIFO address (128 bytes)
    static uint16_t const MEM_TXN_FIFO    = 0x0000;
    
    // TX Beacon FIFO address (128 bytes)
    static uint16_t const MEM_TXB_FIFO    = 0x0080;
    
    // TX GTS1 FIFO address (128 bytes)
    static uint16_t const MEM_TXGTS1_FIFO = 0x0100;
    
    // TX GTS2 FIFO address (128 bytes)
    static uint16_t const MEM_TXGTS2_FIFO = 0x0180;
    
    // RX FIFO address (128 bytes)
    static uint16_t const MEM_RX_FIFO     = 0x0300;

    // TX Security Key FIFO address (16 bytes)
    static uint16_t const MEM_TXSEC_FIFO  = 0x0280;

    // RX Security Key FIFO address (16 bytes)
    static uint16_t const MEM_RXSEC_FIFO  = 0x02B0;
    
    
    // Frame Control Field (16 bit, see 802.15.4-2003)
    //   Frame Type
    static uint16_t const FRAMECTRL_FRAMETYPE_BEACON = 0; // Beacon
    static uint16_t const FRAMECTRL_FRAMETYPE_DATA   = 1; // Data
    static uint16_t const FRAMECTRL_FRAMETYPE_ACK    = 2; // Acknowledgment
    static uint16_t const FRAMECTRL_FRAMETYPE_CMD    = 3; // MAC command
    //   Security Enabled
    static uint16_t const FRAMECTRL_SECENABLED       = BIT3;
    //   Frame Pending
    static uint16_t const FRAMECTRL_FRAMEPENDING     = BIT4;
    //   Ack request
    static uint16_t const FRAMECTRL_ACKREQUEST       = BIT5;
    //   PANID compression (when src panid = dst panid, only dest must be present), also named "Intra PAN"
    static uint16_t const FRAMECTRL_PANIDCOMP        = BIT6;
    //   Dest Addressing Mode
    static uint16_t const FRAMECTRL_DESTADDRMODE_MASK  = (uint16_t)3 << 10;
    static uint16_t const FRAMECTRL_DESTADDRMODE_NONE  = (uint16_t)0 << 10;  // PAN identifier and address fields are not present
    static uint16_t const FRAMECTRL_DESTADDRMODE_SHORT = (uint16_t)2 << 10;  // Address field contains a 16-bit short address
    static uint16_t const FRAMECTRL_DESTADDRMODE_LONG  = (uint16_t)3 << 10;  // Address field contains a 64-bit extended address
    //   Source Addressing Mode
    static uint16_t const FRAMECTRL_SRCADDRMODE_MASK   = (uint16_t)3 << 14;
    static uint16_t const FRAMECTRL_SRCADDRMODE_NONE   = (uint16_t)0 << 14;  // PAN identifier and address fields are not present
    static uint16_t const FRAMECTRL_SRCADDRMODE_SHORT  = (uint16_t)2 << 14;  // Address field contains a 16-bit short address
    static uint16_t const FRAMECTRL_SRCADDRMODE_LONG   = (uint16_t)3 << 14;  // Address field contains a 64-bit extended address
    
    
    
  public:
    
    
    struct Channel
    {
      enum
      {
        CHANNEL11    = 0,  // 2.405 GHz
        CHANNEL12    = 1,  // 2.410 GHz
        CHANNEL13    = 2,  // 2.415 GHz
        CHANNEL14    = 3,  // 2.420 GHz
        CHANNEL15    = 4,  // 2.425 GHz
        CHANNEL16    = 5,  // 2.430 GHz
        CHANNEL17    = 6,  // 2.435 GHz
        CHANNEL18    = 7,  // 2.440 GHz
        CHANNEL19    = 8,  // 2.445 GHz
        CHANNEL20    = 9,  // 2.450 GHz
        CHANNEL21    = 10, // 2.455 GHz
        CHANNEL22    = 11, // 2.460 GHz
        CHANNEL23    = 12, // 2.465 GHz
        CHANNEL24    = 13, // 2.470 GHz
        CHANNEL25    = 14, // 2.475 GHz
        CHANNEL26    = 15  // 2.480 GHz
      };
      Channel() : value(CHANNEL11) {}
      Channel(uint8_t value_) : value(value_) {}
      bool operator==(Channel const& rhs) { return value == rhs.value; }
      bool operator<=(Channel const& rhs) { return value <= rhs.value; }
      Channel operator++() { return ++value; }
      uint8_t         value;
      typedef uint8_t Type;
      static Channel const getMinValue() { return Channel(CHANNEL11); }
      static Channel const getMaxValue() { return Channel(CHANNEL26); }
      static const uint8_t CHANNELSCOUNT = 16;
	  };	  
	  
    
	  // maximum payload size
    static uint8_t const MAXPAYLOAD  = 108;
		  
	  
  private:
    
    // MRF24J40 general purpose timer value (each value = 8us), 0 = disabled
	  static uint16_t const TIMERTOP    = 0;    // es. 12500 = 12500*8us = 100000us = 100ms (10 per second)


    // maximum number of link layer listeners
    static uint8_t const MAXLISTENERS = 3;


    // maximum number of resends in case of no software ack received
    static uint8_t const MAXSENDTRIES = 2;
    
    
    // maximum number of already received messages buffer (circular buffer)
    static uint8_t const MAXALREADYRECEIVEDMESSAGES = 10;
    
    
  private:


    enum AckType
    {
      ACK_NONE,      // no ack required
      ACK_SOFTWARE   // software ack
    };


    // max command values: 0..15
    enum CMD
    {
      // this is a normal message, not a command
      CMD_NONE               = 0,

      // send change channel command (broadcast)
      // params:
      //    1 byte = new channel
      CMD_CHANGECHANNEL      = 1,
    };


    static uint32_t const CMDDATASIZE = 2;
	
  
    // additional structure to contain commands data and info
    struct CMDExtraData
    {
		  CMD      command;                // CMD_NONE = normal message, otherwise a command (not a 802.15.4 command!). Inserted as first byte of frame payload
		  uint8_t  extraData[CMDDATASIZE]; // 
		  uint8_t  LQI;                    // Link Quality Indication (0=bad quality, 255=max quality)
		  uint8_t  RSSI;                   // Received Signal Strength Indicator (0=-100dBm, 255=-20dBm)      
    };


    struct MRFRcvFrame
    {
      uint8_t  destAddress;
      uint8_t  srcAddress;
      uint16_t type_length;
      uint8_t  dataLength;  // may include additional padding      
      uint8_t* payload;
    };


    // specialized for MRF24J40 link layer receive frame
    struct RcvFrame : LinkLayerReceiveFrame
    {
		  uint8_t* payload;              
      
      RcvFrame()
        : payload(NULL), m_readPos(0)
      {          
        dataLength = 0;
      }
      
      RcvFrame(LinkAddress const& srcAddress_, LinkAddress const& destAddress_, uint16_t type_length_, uint8_t* payload_, uint16_t dataLength_)
        : LinkLayerReceiveFrame(srcAddress_, destAddress_, type_length_, dataLength_), payload(payload_)
      {          
      }
      
      void readReset()
      {
        m_readPos = 0;
      }
      
      uint8_t readByte()
      {
        return payload[m_readPos++];
      }
      
      // assume big-endian (that is the network byte order)
      uint16_t readWord()
      {
        uint16_t r = ((uint16_t)payload[m_readPos] << 8) | payload[m_readPos + 1];
        m_readPos += 2;
        return r;
      }
      
      void readBlock(void* dstBuffer, uint16_t length)
      {
        memcpy(dstBuffer, &payload[m_readPos], length);
        m_readPos += length;
      }
      
    private:   
       
      uint16_t m_readPos;
    };


    struct MRFSendFrame
    {
      uint8_t         srcAddress;
      uint8_t         destAddress;
      uint16_t        type_length;
      DataList const* dataList;
    
      explicit MRFSendFrame(uint8_t srcAddress_, uint8_t destAddress_, uint16_t type_length_, DataList const* dataList_)
        : srcAddress(srcAddress_), destAddress(destAddress_), type_length(type_length_), dataList(dataList_)
      {
      }    
    };
    
    
    // identifies an already received message
    struct ReceivedMessageID
    {
      uint8_t  sourceAddress;
      uint16_t messageID;
      
      explicit ReceivedMessageID(uint8_t sourceAddress_, uint16_t messageID_)
        : sourceAddress(sourceAddress_), messageID(messageID_)
      {          
      }
      
      ReceivedMessageID()
        : sourceAddress(0), messageID(0)
      {
      }
      
      bool operator == (ReceivedMessageID const& rhs) const
      {
        return sourceAddress == rhs.sourceAddress && messageID == rhs.messageID;
      }
    };


	public:

	
    enum DeviceType
	  {
		  MRF24J40MA,
		  MRF24J40MB
	  };
	  
	  
	  enum Speed
	  {
		  SPEED_250_kbps,
		  SPEED_625_kbps
	  };
	  		  
    
    // This driver uses 1 byte for address (allowing up to 255 devices in the same PANID, 0xFF reserved for broadcast)
    // "address" is the LinkLayer address. Only the first byte is used, so it must be used to differentiate among devices.
    // channel: 0..15 (channel_11..channel_26) : channel can be changed using setChannel
    MRF24J40(HardwareSPIMaster* spi, bool sharedSPI,
	           uint16_t PANID, LinkAddress const* address, Channel channel, 
			       DeviceType deviceType, Speed speed, PGM_P key)
      : m_SPI(spi),
        m_sharedSPI(sharedSPI),
        m_channel(channel),
        m_deviceType(deviceType),
        m_PANID(PANID),
        m_address(*address),
        m_speed(speed)
    {		  
      Random::reseed(m_address[0]);
      prepareKey(key);
      reset();
    }


    void reset()
    {
      MutexLock lock(m_SPI->mutex(), m_sharedSPI);

      // status
      m_available      = false;
      m_frameReceived  = false;
      m_frameSent      = false;
      m_messageID      = Random::nextUInt16();

      // Reset Power Module, Baseband and MAC
      setReg(REG_SOFTRST, BIT_RSTPWR | BIT_RSTBB | BIT_RSTMAC);
        
      // Enable FIFO and set "Transmitter Enable On Time Symbol"
      setReg(REG_PACON2, BIT_FIFOEN | (6 << SHIFT_TXONTS));
        
      // Set VCO Stabilization Period
      setReg(REG_TXSTBL, 9 << SHIFT_RFSTBL);
        
      // Set RFOPT
      setReg(REG_RFCON0, (3 << SHIFT_RFOPT));

      // Set VCO Optimize Control
      setReg(REG_RFCON1, 2);
        
      // enable RF-PLL
      setReg(REG_RFCON2, BIT_PLLEN);

      // enable TX filter control and 20Mhz Clock Recovery Control
      setReg(REG_RFCON6, BIT_TXFIL | BIT_20MRECVR);
        
      // sleep clock (100kHz internal oscillator)
      setReg(REG_RFCON7, VAL_SLPCLKSEL_100KHZ);
        
      // enable VCO
      setReg(REG_RFCON8, BIT_RFVCO);
        
      // initialize CLKOUT pin enable and clock divisor (div by 1)
      setReg(REG_SLPCON1, BIT_CLKOUTEN | (1 << SHIFT_SLPCLKDIV));
        
      // set CCA mode 1
      setReg(REG_BBREG2, VAL_CCAMODE_1);

      // set energy detection threshold for CCA
      //setReg(REG_CCAEDTH, 10);    // fine tune!!
      setReg(REG_CCAEDTH, 0X60);    // fine tune!!

      // set RSSI mode (calculate for each received packet)
      setReg(REG_BBREG6, BIT_RSSIMODE2);

      // setup messages filter
      setPromiscuousMode_nolock(false);
        
      // Set channel
      setReg(REG_RFCON0, (m_channel.value << SHIFT_CHANNEL) | (getReg(REG_RFCON0) & 0x0F));
        
      // TX power (0dB = max)
      setReg(REG_RFCON3, VAL_TXPWRL_00DB | VAL_TXPWRS_0p0DB);

      // reset RF
      resetRF_nolock();

      if (m_deviceType == MRF24J40MB)
      {
        // PA/LNA (Power Amplifier / Low Noise Amplifier) state machine enable
        setReg(REG_TESTMODE, (7 << SHIFT_TESTMODE) | (1 << SHIFT_RSSIWAIT));
      }
        
      // flush RX fifo
      setReg(REG_RXFLUSH, BIT_RXFLUSH);
        
      // SECCON1, enable encryption
      setReg(REG_SECCON1, BIT_DISDEC | BIT_DISENC); // DISDEC=1 and DISENC=1
        
      // short MAC address
      setReg(REG_SADRL, m_address[0]);
      setReg(REG_SADRH, 0xFF);
        
      // long MAC address (used only when encryption is enabled)
      // this MRF24J40 driver uses only one byte of the LinkAddress address (that is 6 bytes).
      setReg(REG_EADR0, m_address[0]);
      setReg(REG_EADR1, 0xFF);
      setReg(REG_EADR2, 0xFF);
      setReg(REG_EADR3, 0xFF);
      setReg(REG_EADR4, 0xFF);
      setReg(REG_EADR5, 0xFF);
      setReg(REG_EADR6, 0xFF);
      setReg(REG_EADR7, 0xFF);
        
      // PAN ID
      setReg(REG_PANIDL, 0xFF & m_PANID);
      setReg(REG_PANIDH, (m_PANID >> 8));
        
      // verify PAN ID
      if ((0xFF & m_PANID) != getReg(REG_PANIDL) || (m_PANID >> 8) != getReg(REG_PANIDH))
        return;

      if (m_speed == SPEED_625_kbps)
      {
        // Turbo mode
        setReg(REG_BBREG0, VAL_TURBO);
        setReg(REG_BBREG3, VAL_PREVALIDTH_TURBO | (0x4 << SHIFT_PREDETTH));
        setReg(REG_BBREG4, VAL_CSTH_TURBO | (7 << SHIFT_PRECNT));
        resetRF();
      }

      m_available = true;
    }
    

    bool isAvailable()
    {
      return m_available;
    }


    void addListener(ILinkLayerListener* listener)
    {
      m_listeners.push_back(listener);
    }


  private:


    void checkInterrupt()
    {
      uint8_t intstat = getReg(REG_INTSTAT);

      // Frame sent
      if (intstat & BIT_TXNIF)
      {
        m_frameSent = true;
      }

      // Frame received
      if (intstat & BIT_RXIF)
      {
        m_frameReceived = true;
      }
    }
    
		  
    void resetRF_nolock()
    {
      setReg(REG_RFCTL, BIT_RFRST); // reset on
      delayMicroseconds(192);
      setReg(REG_RFCTL, 0);         // reset off
      delayMicroseconds(192);
    }


    void setPromiscuousMode_nolock(bool value)
    {
      // setup receive MAC control register
      if (value)
      setReg(REG_RXMCR, BIT_PROMI);
      else
      setReg(REG_RXMCR, 0);
    }

      
  public:
	
  
    void setPromiscuousMode(bool value)
    {
      MutexLock lock(m_SPI->mutex(), m_sharedSPI);
      setPromiscuousMode_nolock(value);
    }
        

    void resetRF()
    {
      MutexLock lock(m_SPI->mutex(), m_sharedSPI);
      resetRF_nolock();
    }


    #ifdef VERBOSE
    void dumpRegisters()
    {
      // short registers
      for (uint16_t r = 0x00; r <= 0x3F; ++r)
        cout << r << " = " << getReg_noIRQ(r) << endl;
      // long registers
      for (uint16_t r = 0x200; r <= 0x24C; ++r)
        cout << r << " = " << getReg_noIRQ(r) << endl;
    }
    #endif

	  
	  // change local channel
	  void setChannel(Channel channel)
	  {
      if (channel == m_channel)
        return;
      MutexLock lock(m_SPI->mutex(), m_sharedSPI);
      m_channel = channel;
      // Set channel
      setReg(REG_RFCON0, (channel.value << SHIFT_CHANNEL) | (getReg(REG_RFCON0) & 0x0F));
      resetRF();
	  }
	  
	  
	  Channel getChannel()
	  {
		  return m_channel;
	  }


    LinkAddress const& getAddress() const
    {
      return m_address;
    }
	  
	  
  private:
  
    	  	  
	  void sendCMD_CHANGECHANNEL(Channel channel)
	  {
      uint8_t data[1] = {channel.value};
      DataList datalist(NULL, &data[0], sizeof(data));
      MRFSendFrame frame(m_address[0], 0xFF, 0x0000, &datalist);  // destination is broadcast
      directSendFrame(&frame, ACK_NONE, ++m_messageID, false, CMD_CHANGECHANNEL);
	  }		  

    
  public:
  
    
    void sendCMD_CHANGECHANNEL_ALL(Channel channel)
    {
      Channel prevChannel = m_channel;
      for (Channel::Type c = Channel::CHANNEL11; c <= Channel::CHANNEL26; ++c)
      {
        setChannel(c);
        sendCMD_CHANGECHANNEL(channel);
      }
      // restore current channel
      setChannel(prevChannel);
    }
    
    
  private:


	  // if frame is a CMD process it.
	  bool processCMD(MRFRcvFrame* frame, CMDExtraData* extra)
	  {
			switch (extra->command)
			{        
				case CMD_CHANGECHANNEL:
          if (frame->dataLength == 1)
          {
	  				setChannel(frame->payload[0]);						  
          }
          return true;

        default:
          return true;  // unknown command, return "processed" anyway
			}				  
	  }

	  
  private:
  

    // can send also ACK messages
    SendResult directSendFrame(MRFSendFrame const* frame, AckType ackType, uint16_t messageID, bool isACK = false, CMD command = CMD_NONE, uint8_t const* extraData = NULL, uint8_t extraDataLen = 0, bool flooded = false)
    {
      
      uint8_t const payloadlen = (frame->dataList? frame->dataList->calcLength() : 0);
      
      if (payloadlen > MAXPAYLOAD)
      {
        return SendFail; // packet too long
      }        
      
      // cannot request ack when sending broadcast messages
      ackType = (frame->destAddress == 0xFF? ACK_NONE : ackType);
      
      #ifdef VERBOSE
      cout << "send msg " << (uint16_t)messageID << endl;
      #endif   
            
      // 2 = FCF    1 = sequence number    2 = dest PANID    2 = destination address    2 = source address      
      uint8_t const hdrlen = 2 + 1 + 2 + 2 + 2;
      
      // +2 is for message-id, +2 is for checksum field (non standard), +2 is for protocol (type_length) field, +1 actual destination
      uint8_t const framelen = hdrlen + 2 + 2 + 2 + 1 + payloadlen + (command == CMD_NONE? 0 : CMDDATASIZE);

      // calculate FCF (frame control field)      
	    uint16_t const FCF = FRAMECTRL_PANIDCOMP |
	                         FRAMECTRL_DESTADDRMODE_SHORT |
	                         FRAMECTRL_SRCADDRMODE_SHORT |
                           FRAMECTRL_FRAMETYPE_DATA;

      // atomic block
      {
        MutexLock lock(m_SPI->mutex(), m_sharedSPI);
  
        uint16_t wpos = MEM_TXN_FIFO;
        
        // write header length        
        writeLongAddress(wpos++, hdrlen);
        
        // write frame length
        writeLongAddress(wpos++, framelen);
        
        // write Frame control field	    
  		  writeLongAddress(wpos++, FCF & 0xFF); // low byte      
  		  writeLongAddress(wpos++, (FCF >> 8) & 0xFF);   // high byte      
  		  
        // Sequence number
        // 0    (1 bit) : ACK (this is an ACK message)
        // 1    (2 bit) : not used
        // 2..4 (3 bit) : command, values 0..7
        // 5    (1 bit) : 1 = requested soft ack
        // 6    (1 bit) : 0 = direct message   1 = flooded (resent by another device)
        // 7    (1 bit) : not used
        uint8_t const seqnum = isACK? ((1 << 0) | (flooded? 1 << 6 : 0)) :
                                      (((uint8_t)command << 2) | (ackType == ACK_SOFTWARE? 1 << 5 : 0) | (flooded? 1 << 6 : 0));
        writeLongAddress(wpos++, seqnum);
        
        // Destination PANID
  	    writeLongAddress(wpos++, m_PANID & 0xFF); // low byte      
  	    writeLongAddress(wpos++, (m_PANID >> 8) & 0xFF); // high byte      
        
        // Destination address (broadcast)
        writeLongAddress(wpos++, 0xFF);
        writeLongAddress(wpos++, 0xFF);
        
        // Source address
        writeLongAddress(wpos++, frame->srcAddress);
        writeLongAddress(wpos++, 0xFF);
  		
  			// command extra data
        if (command != CMD_NONE)
        {
          for (uint8_t i = 0; i != CMDDATASIZE; ++i)
            writeLongAddress(wpos++, (i < extraDataLen? extraData[i] : 0));          
        }
  			  
        // write actual destination
        writeLongAddress(wpos++, frame->destAddress);
   
        // write message id (16 bit)
        writeLongAddress(wpos++, messageID & 0xFF);         // low byte
        writeLongAddress(wpos++, (messageID >> 8) & 0xFF);  // high byte                
        
        // write Checksum (not 802.15.4 standard)
        if (isACK)
        {
          writeLongAddress(wpos++, 0);
          writeLongAddress(wpos++, 0);
        }
        else
        {
          uint16_t const checksum = calcChecksum(frame, command, messageID);
          writeLongAddress(wpos++, checksum & 0xFF);        // low byte
          writeLongAddress(wpos++, (checksum >> 8) & 0xFF); // high byte
        }  
          
        // write upper protocol (not 802.15.4 standard)
        if (isACK)
        {
          writeLongAddress(wpos++, 0);
          writeLongAddress(wpos++, 0);
        }
        else
        {
          writeLongAddress(wpos++, frame->type_length & 0xFF);        // low byte
          writeLongAddress(wpos++, (frame->type_length >> 8) & 0xFF); // high byte
        }
  
        // Payload
        if (payloadlen > 0)
        {
          if (isACK)
          {
            // ACK is not encrypted
            DataList const* data = frame->dataList;
            while (data != NULL)
            {
              uint8_t const* bw = (uint8_t const*)data->data;
              for (uint8_t i = 0; i != data->length; ++i)
                writeLongAddress(wpos++, *bw++);
              data = data->next;
            }
          }
          else
          {
            EncodeInfo encodeInfo(m_keyz, m_keyw, messageID);
            DataList const* data = frame->dataList;
            while (data != NULL)
            {
              uint8_t const* bw = (uint8_t const*)data->data;
              for (uint8_t i = 0; i != data->length; ++i)
                writeLongAddress(wpos++, encodeByte(encodeInfo, *bw++));
              data = data->next;
            }
          }
        }          
          
      } // end of Atomic block

      // Check transmission status
      while (true)
      {
        // Send
        setReg_noIRQ(REG_TXNCON, BIT_TXNTRIG);

        TimeOut timeOut(15);  // wait up to 15ms
        while (!m_frameSent && !timeOut)
          checkInterrupt();          
        uint8_t txstat = getReg_noIRQ(REG_TXSTAT);
        m_frameSent    = false;
			  if ((txstat & BIT_TXNSTAT) == 0)
        {
          if (!flooded && !isACK && ackType == ACK_SOFTWARE && !recvSoftACK(m_address[0], messageID, 650))
          {
            // no soft-ack
            return SendFail;
          }            
				  return SendOK;
        }
      }

    }


    SendResult sendFrame(LinkLayerSendFrame const* frame)
    {
      MRFSendFrame directFrame(frame->srcAddress[0], frame->destAddress[0], frame->type_length, frame->dataList);
      for (uint8_t i = 0; i != MAXSENDTRIES; ++i)
      {
        // each try has a different message-id, to allow flooding
        if (directSendFrame(&directFrame, ACK_SOFTWARE, ++m_messageID) == SendOK)
          return SendOK;
      }
      return SendFail;
    }
    
    
    struct StopAndRestartRX
    {
      StopAndRestartRX(MRF24J40& mac_) : mac(mac_)
      {
        // stop receiving packets (suggested in a note of "3.11.1 RECEPTION MODES")
        mac.setReg(REG_BBREG1, BIT_RXDECINV);
      }
      ~StopAndRestartRX()
      {
        // restart receiving packets (suggested in a note of "3.11.1 RECEPTION MODES")
        mac.setReg(REG_BBREG1, 0);
        // Flush FIFO
        mac.setReg(REG_RXFLUSH, BIT_RXFLUSH);
      }
      MRF24J40& mac;
    };
    
    
    void recvFrame()
    {

      checkInterrupt();
      if (!m_frameReceived)
        return;

      uint16_t checksum = 0;

      MRFRcvFrame                frame;
      SimpleBuffer<CMDExtraData> extra;          // container for "extra" data
      SimpleBuffer<uint8_t>      payloadBuffer;  // container for payload
      
      CMD command        = CMD_NONE;
      
      uint16_t messageID = 0;
      AckType acktype    = ACK_NONE;
      uint8_t seqnum     = 0;
      bool isACK         = false;
      
      #if defined(MRFDEBUG_ACCEPTONLYFLOODEDMESSAGES)
      bool isFlooded     = false;
      #endif
      
      {
        MutexLock lock(m_SPI->mutex(), m_sharedSPI);

        // stop receiving packets (suggested in a note of "3.11.1 RECEPTION MODES")
        StopAndRestartRX stopAndStartRX(*this);

        m_frameReceived = false;

        for (uint8_t tries = 0; ; ++tries)
        {          
      
          uint16_t rpos = MEM_RX_FIFO;
      	  
          // Frame Length
          uint8_t frameLength = readLongAddress(rpos++);        

          // Frame Control Field
	        uint16_t FCF = readLongAddress(rpos) | ((uint16_t)readLongAddress(rpos+1) << 8);
		      rpos += 2;        
          if ((FCF & FRAMECTRL_SECENABLED) || 
              ((FCF & FRAMECTRL_SRCADDRMODE_MASK) == FRAMECTRL_SRCADDRMODE_LONG) || 
              ((FCF & FRAMECTRL_DESTADDRMODE_MASK) == FRAMECTRL_DESTADDRMODE_LONG) ||
              ((FCF & 7) != FRAMECTRL_FRAMETYPE_DATA))
          {
            return; // we don't use security and long addresses
          }            

          // Sequence number
          // 0    (1 bit) : ACK (this is an ACK message)
          // 1    (2 bit) : not used
          // 2..4 (3 bit) : command, values 0..7
          // 5    (1 bit) : 1 = requested soft ack
          // 6    (1 bit) : 0 = direct message   1 = flooded (resent by another device)
          // 7    (1 bit) : not used
          seqnum    = readLongAddress(rpos++);
          command   = (CMD)((seqnum >> 2) & 7);
          acktype   = (seqnum & (1 << 5)) ? ACK_SOFTWARE : ACK_NONE;
          isACK     = seqnum & (1 << 0);
          #if defined(MRFDEBUG_ACCEPTONLYFLOODEDMESSAGES)
          isFlooded = seqnum & (1 << 6);
          #endif
      
          #ifdef MRFDEBUG_ACCEPTONLYFLOODEDMESSAGES
          if (!isFlooded)
            return; // discard if this is not flooded (for debug purposes only)
          #endif

          // Destination PANID (ignore)
	        rpos += 2;
      
          // Destination address (ignore, always broadcast)
          rpos += 2;
      
          // Source address
          frame.srcAddress = readLongAddress(rpos);
          rpos += 2;

          // Check source address
          if (frame.srcAddress == m_address[0])
          {
            return; // from my-self (maybe broadcast replication)
          }            

          if (command != CMD_NONE)
          {
            // this is a command, read extra structure
            extra.reset(1);
            extra.get()->command = command;

            for (uint8_t i = 0; i != CMDDATASIZE; ++i)
              extra.get()->extraData[i] = readLongAddress(rpos++);            
          }
        
          // actual destination
          frame.destAddress = readLongAddress(rpos++);

          // read message-id
          messageID = readLongAddress(rpos) | ((uint16_t)readLongAddress(rpos + 1) << 8);
          rpos += 2;        
        
          // read non-standard Checksum
          checksum = readLongAddress(rpos) | ((uint16_t)readLongAddress(rpos + 1) << 8);
          rpos += 2;        
        
          // read non-standard protocol (type_length)
          frame.type_length = readLongAddress(rpos) | ((uint16_t)readLongAddress(rpos + 1) << 8);
          rpos += 2;

          // Payload
		      // subtract:
		      //    12 = FCF(2) + SEQ(1) + PANID(2) + DEST(1) + MESSAGEID(2) + NON_STD_CHECKSUM(2) + NON_STD_PROTOCOL(2)
		      //    if command:
          //      CMDDATA  = extra command data
		      //    short address:
		      //      4 = SHORT_DEST_ADDR(2) + SHORT_SRC_ADDR(2)
		      //    2 = CRC(2)
	        frame.dataLength = frameLength - 12 - 4 - (command != CMD_NONE? CMDDATASIZE : 0) - 2;
          if (frame.dataLength > MAXPAYLOAD || getFreeMem() - 200 < frame.dataLength)
          {
            return; // too large, discard
          }            
          payloadBuffer.reset(frame.dataLength);
          frame.payload = payloadBuffer.get();
          
          // decode payload
          if (isACK)
          {
            // ACK, do not decrypt
            for (uint8_t i = 0; i != frame.dataLength; ++i)
              frame.payload[i] = readLongAddress(rpos++);
          }
          else
          {            
            // message, decrypt
            EncodeInfo encodeInfo(m_keyz, m_keyw, messageID);
            for (uint8_t i = 0; i != frame.dataLength; ++i)
              frame.payload[i] = encodeByte(encodeInfo, readLongAddress(rpos++));
          }            
          
          // CRC (ignore)
	        rpos += 2;
      
          if (extra.get())
          {
            // LQI
            extra.get()->LQI = readLongAddress(rpos++);        
            // RSSI
            extra.get()->RSSI = readLongAddress(rpos++);          
          }
        
          // checksum is 0x0000 when message is ACK
          uint16_t const chk = (isACK? 0 : calcChecksum(&frame, extra.get(), messageID));
          if (chk != checksum)
          {
            if (tries == 2)
            {
              return; // discard, wrong checksum
            }  
          }  
          else
            break;            
        }  // end of tries loop
        
      }    // end of RX stop
    
      #ifdef VERBOSE
      cout << "recv msg " << (uint16_t)messageID << endl;
      #endif

      // already received?
      if (m_receivedMessages.indexOf(ReceivedMessageID(frame.srcAddress, messageID)) != -1)
      {
        return; // yes, already received, discard.
      }        
        
      // add this message to already received list
      m_receivedMessages.add(ReceivedMessageID(frame.srcAddress, messageID));
    
      // not for me, flood if necessary
      if (frame.destAddress != m_address[0] && frame.destAddress != 0xFF)
      {
        // if not ACK and this packet requires an ACK then wait for receiver ACK
        uint16_t ackMsgID;
        if (!isACK && acktype == ACK_SOFTWARE && recvSoftACK(frame.srcAddress, messageID, 60, &ackMsgID)) // wait ACK directed to source of this message
        {
          // ACK received, replicate ACK
          delay_ms( 0 + Random::nextUInt16(0, 60) ); // wait random time
          sendSoftACK(frame.destAddress, frame.srcAddress, messageID, ackMsgID);
          return; // discard
        }
        // ACK not received or not necessary, maybe actual recipient didn't receive the message. Resend the message.
        #ifdef VERBOSE
        serial.write_P(PSTR("recvFrame: perform flood")); cout << endl;
        #endif
        delay_ms( 0 + Random::nextUInt16(0, 60) ); // wait random time
        DataList data(NULL, frame.payload, frame.dataLength);
        MRFSendFrame outFrame(frame.srcAddress, frame.destAddress, frame.type_length, &data);
        directSendFrame(&outFrame, acktype, messageID, isACK, command, (extra.get()? extra.get()->extraData : NULL), (extra.get()? CMDDATASIZE : 0), true);
    		return;
      }    
    
      // Check if this is an ACK
      if (isACK) // is an ACK?
      {
        // discard
        return;
      }
    
      // send software ack
      if (frame.destAddress == m_address[0] && acktype == ACK_SOFTWARE && !isACK)
        sendSoftACK(frame.destAddress, frame.srcAddress, messageID, ++m_messageID);

      // resend broadcast messages
      // This is the way broadcast message uses to global flooding
      if (frame.destAddress == 0xFF)
      {
        delay_ms( 0 + Random::nextUInt16(0, 60) ); // wait random time
        DataList data(NULL, frame.payload, frame.dataLength);
        MRFSendFrame outFrame(frame.srcAddress, 0xFF, frame.type_length, &data);
        directSendFrame(&outFrame, ACK_NONE, messageID, false, command, (extra.get()? extra.get()->extraData : NULL), (extra.get()? CMDDATASIZE : 0), true);
      }

      if ((frame.destAddress == m_address[0] || frame.destAddress == 0xFF) && extra.get() && processCMD(&frame, extra.get()))
      {
        return; // command processed
      }
      
      if (frame.destAddress == m_address[0] || frame.destAddress == 0xFF)
      {
        RcvFrame rframe(LinkAddress(frame.srcAddress, m_address[1], m_address[2], m_address[3], m_address[4], m_address[5]),
                        LinkAddress(frame.destAddress, m_address[1], m_address[2], m_address[3], m_address[4], m_address[5]),
                        frame.type_length,
                        frame.payload,
                        frame.dataLength); 
        for (uint8_t i = 0; i != m_listeners.size(); ++i)
        {
          rframe.readReset();
          if (m_listeners[i]->processLinkLayerFrame(&rframe))
            break; // message processed
        }      
      }        
    }
    

  public:

    uint8_t channelAssessment()
    {    

      if (MRF24J40MB == m_deviceType)
      {
        setReg(REG_TESTMODE, 0x08);
        setReg(REG_TRISGPIO, 0x0F); 
        setReg(REG_GPIO, 0x04);    
      }
   
      setReg(REG_BBREG6, 0x80);
   
      uint8_t RSSIcheck = getReg(REG_BBREG6);
      while ((RSSIcheck & 0x01) != 0x01)
      {
        RSSIcheck = getReg(REG_BBREG6);
      }
   
      RSSIcheck = getReg(0x210);
   
      setReg(REG_BBREG6, BIT_RSSIMODE2);
   
      if (MRF24J40MB == m_deviceType)
      {
        setReg(REG_TRISGPIO, 0x00);
        setReg(REG_GPIO, 0);
        setReg(REG_TESTMODE, 0x0F);
      }
   
      return RSSIcheck;
    }

    struct RSSIStats
    {
      uint8_t min;
      uint8_t avg;
      uint8_t max;
    };

    // stats must be stats[Channel::CHANNELSCOUNT]
    void getChannelsRSSIStats(RSSIStats* stats, uint32_t millisPerChannel)
    {
      Channel prevChannel = m_channel;
      for (Channel ch = Channel::getMinValue(); ch <= Channel::getMaxValue(); ++ch)
      {
        setChannel(ch);
        stats[ch.value].min = 255;
        stats[ch.value].avg = 0;
        stats[ch.value].max = 0;
        uint32_t avg = 0;
        uint32_t count = 0;
        TimeOut timeout(millisPerChannel);
        while (!timeout)
        {
          uint8_t rssi = channelAssessment();
          if (rssi < stats[ch.value].min)
            stats[ch.value].min = rssi;
          if (rssi > stats[ch.value].max)
            stats[ch.value].max = rssi;
          avg += rssi;
          ++count;
        }
        stats[ch.value].avg = avg / count;
      }
      setChannel(prevChannel);
    }
 
    
  private:
    
    static uint8_t const ACKPADDINGSIZE = 0; // todo: fine tune!
    
    
    void sendSoftACK(uint8_t srcAddress, uint8_t dstAddress, uint16_t messageID, uint16_t ackMsgID)
    {

      #ifdef VERBOSE
      cout << "send ack " << (uint16_t)ackMsgID << ":" << (uint16_t)messageID << endl;
      #endif
      
      // 2 = FCF    1 = sequence number    2 = dest PANID    2 = destination address    2 = source address
      uint8_t const hdrlen = 2 + 1 + 2 + 2 + 2;
      
      // +2 is for message-id, +2 checksum (0x000), +2 protocol (0x000), +2 reply message-id, +1 is for actual destination
      uint8_t const framelen = hdrlen + 2 + 2 + 2 + 2 + 1 + ACKPADDINGSIZE;

      // calculate FCF (frame control field)
      uint16_t const FCF = FRAMECTRL_PANIDCOMP |
                           FRAMECTRL_DESTADDRMODE_SHORT |
                           FRAMECTRL_SRCADDRMODE_SHORT |
                           FRAMECTRL_FRAMETYPE_DATA;

      {
        MutexLock lock(m_SPI->mutex(), m_sharedSPI);

        uint16_t wpos = MEM_TXN_FIFO;
          
        // write header length
        writeLongAddress(wpos++, hdrlen);
          
        // write frame length
        writeLongAddress(wpos++, framelen);
          
        // write Frame control field
        writeLongAddress(wpos++, FCF & 0xFF);          // low byte
        writeLongAddress(wpos++, (FCF >> 8) & 0xFF);   // high byte
          
        // Sequence number
        writeLongAddress(wpos++, 0xFF); // soft-ack marker
          
        // Destination PANID
        writeLongAddress(wpos++, m_PANID & 0xFF);        // low byte
        writeLongAddress(wpos++, (m_PANID >> 8) & 0xFF); // high byte
          
        // Destination address (broadcast)
        writeLongAddress(wpos++, 0xFF);
        writeLongAddress(wpos++, 0xFF);
          
        // Source address
        writeLongAddress(wpos++, srcAddress);
        writeLongAddress(wpos++, 0xFF);
  
        // write actual destination
        writeLongAddress(wpos++, dstAddress);
          
        // write this message id (16 bit)
        writeLongAddress(wpos++, ackMsgID & 0xFF);         // low byte
        writeLongAddress(wpos++, (ackMsgID >> 8) & 0xFF);  // high byte
      
        // write non-standard checksum (zero)
        writeLongAddress(wpos++, 0);
        writeLongAddress(wpos++, 0);
        
        // write non-standard protocol (zero)
        writeLongAddress(wpos++, 0);
        writeLongAddress(wpos++, 0);      

        //// payload

        // write reply message id (16 bit)
        writeLongAddress(wpos++, messageID & 0xFF);         // low byte
        writeLongAddress(wpos++, (messageID >> 8) & 0xFF);  // high byte
      
        // write padding
        for (uint8_t i = 0; i != ACKPADDINGSIZE; ++i)
          writeLongAddress(wpos++, 0xAA);
      }

      // Check transmission status
      while (true)
      {
        // Send
        setReg_noIRQ(REG_TXNCON, BIT_TXNTRIG);

        TimeOut timeOut(15);  // wait up to 15ms
        while (!m_frameSent && !timeOut)
          checkInterrupt();
        uint8_t txstat = getReg_noIRQ(REG_TXSTAT);
        m_frameSent = false;
        if ((txstat & BIT_TXNSTAT) == 0)
          return;

      }
    }
    
    
    bool recvSoftACK(uint8_t waiterAddress, uint16_t messageID, uint32_t maxTimeOut, uint16_t* ackMsgID = NULL)
    {
      TimeOut timeOut(maxTimeOut);
      while (!timeOut)
      {
        checkInterrupt();
        if (!m_frameReceived)
          continue;

        {
          MutexLock lock(m_SPI->mutex(), m_sharedSPI);

          // to execute code at startup of atomic-block and at cleanup
          StopAndRestartRX stopAndStartRX(*this);
  
          m_frameReceived = false;
  
          uint16_t rpos = MEM_RX_FIFO;
            
          // Frame Length
          readLongAddress(rpos++);
  
          // Frame Control Field
          uint16_t FCF = readLongAddress(rpos) | ((uint16_t)readLongAddress(rpos + 1) << 8);
          rpos += 2;
          // check security, long addresses
          if ((FCF & FRAMECTRL_SECENABLED) ||
              ((FCF & FRAMECTRL_SRCADDRMODE_MASK) == FRAMECTRL_SRCADDRMODE_LONG) || 
              ((FCF & FRAMECTRL_DESTADDRMODE_MASK) == FRAMECTRL_DESTADDRMODE_LONG) ||
              ((FCF & 7) != FRAMECTRL_FRAMETYPE_DATA))
          {
            continue;
          }          

          // Sequence number  
          uint8_t const seqnum = readLongAddress(rpos++);
          bool const isACK     = seqnum & (1 << 0);
          #ifdef MRFDEBUG_ACCEPTONLYFLOODEDMESSAGES
          bool const isFlooded = seqnum & (1 << 6);
          #endif
          
          // Check sequence number
          if (!isACK) // isn't an ACK?
          {
            continue; // no, discard
          }          

          #ifdef MRFDEBUG_ACCEPTONLYFLOODEDMESSAGES
          if (!isFlooded)
            continue; // discard if this is not flooded (for debug purposes only)
          #endif

          // Destination PANID (ignore)
          rpos += 2;
            
          // Destination address
          rpos += 2;
            
          // Source address
          uint8_t const srcAddress = readLongAddress(rpos);
          rpos += 2;

          // Check source address
          if (srcAddress == m_address[0])
          {
            continue; // from my-self (maybe broadcast replication)
          }          

          // actual destination
          uint8_t const destAddress = readLongAddress(rpos++);
  
          // ACK message id
          if (ackMsgID)
            *ackMsgID = readLongAddress(rpos) | ((uint16_t)readLongAddress(rpos + 1) << 8);
          rpos += 2;

          // checksum (ignore), always 0 for ACKs
          rpos += 2;
          
          // protocol (ignore), always 0 for ACKs
          rpos += 2;
  
          // read message-id
          uint16_t const rmessageID = readLongAddress(rpos) | ((uint16_t)readLongAddress(rpos + 1) << 8);
          rpos += 2;
          
          #ifdef VERBOSE
          cout << "rcv ack XXX:" << (uint16_t)rmessageID << endl;
          #endif

          if (rmessageID == messageID && destAddress == waiterAddress)
          {
            return true;
          }
        } // mutex end
      }        
      return false;
    }
    
    
    uint16_t calcChecksum(MRFRcvFrame const* frame, CMDExtraData const* extra, uint16_t messageID)
    {
      uint16_t checksum = 0;
      for (uint8_t i = 0; i != frame->dataLength; ++i)
        checksum += frame->payload[i];
      checksum += (extra == NULL? 0 : extra->command);
      checksum += frame->dataLength;
      checksum += frame->srcAddress;
      checksum += frame->destAddress;
      checksum += frame->type_length;
      checksum += messageID;
      return checksum;
    }
    
    
    uint16_t calcChecksum(MRFSendFrame const* frame, CMD command, uint16_t messageID)
    {
      uint16_t checksum = 0;   
      uint16_t len = 0;
      DataList const* data = frame->dataList;
      while (data != NULL)
      {
        uint8_t const* bw = (uint8_t const*)data->data;
        for (uint8_t i = 0; i != data->length; ++i, ++len)
          checksum += *bw++;
        data = data->next;
      }        
      checksum += command;
      checksum += len;
      checksum += frame->srcAddress;
      checksum += frame->destAddress;      
      checksum += frame->type_length;
      checksum += messageID;
      return checksum;
    }
    
    
    void prepareKey(PGM_P key)
    {
      m_keyz = 0;
      m_keyw = 0;
      while (true)
      {
        uint8_t b = pgm_read_byte(key++);
        if (b == 0)
          break;
        m_keyz += b;
        b = pgm_read_byte(key++);
        if (b == 0)
          break;
        m_keyw += b;      
      }
    }


    struct EncodeInfo
    {
      uint32_t z;
      uint32_t w;
      union
      {
        uint8_t  keystreamA[4];
        uint32_t keystreamV;
      };      
      uint8_t  keystreamPos;  
      
      EncodeInfo(uint32_t keyz, uint32_t keyw, uint8_t dynamicPass)
        : z(keyz * dynamicPass), w(keyw * dynamicPass), keystreamV(0), keystreamPos(4)
      {        
      }
    };
    
    
    uint8_t encodeByte(EncodeInfo& encodeInfo, uint8_t data)
    {
      if (encodeInfo.keystreamPos == 4)
      {
        encodeInfo.z = 36969 * (encodeInfo.z & 65535) + (encodeInfo.z >> 16);
        encodeInfo.w = 18000 * (encodeInfo.w & 65535) + (encodeInfo.w >> 16);
        encodeInfo.keystreamV = (encodeInfo.z << 16) + encodeInfo.w;          
        encodeInfo.keystreamPos = 0;
      }
      return data ^ encodeInfo.keystreamA[encodeInfo.keystreamPos++];
    }
    

    void setReg(uint16_t address, uint8_t value)
    {
      if (address <= 0x3F)
        writeShortAddress(address, value);
      else
        writeLongAddress(address, value);
    }
	
	
	  void setReg_noIRQ(uint16_t address, uint8_t value)
	  {
      MutexLock lock(m_SPI->mutex(), m_sharedSPI);
  	  setReg(address, value);
	  }		  
    
    
    uint8_t getReg(uint16_t address)
    {
      return (address <= 0x3F)? readShortAddress(address) : readLongAddress(address);
    }
	
	
	  uint8_t getReg_noIRQ(uint16_t address)
	  {
      MutexLock lock(m_SPI->mutex(), m_sharedSPI);
  	  return getReg(address);
	  }		  
    

    void writeLongAddress(uint16_t address, uint8_t value)
    {
      m_SPI->select();
      m_SPI->write(((uint8_t)(address >> 3) & 0x7f) | 0x80);
      m_SPI->write(((uint8_t)(address << 5) & 0xe0) | 0x10);
      m_SPI->write(value);
      m_SPI->deselect();
    }
    
    
    uint8_t readLongAddress(uint16_t address)
    {
      m_SPI->select();
      m_SPI->write(((uint8_t)(address >> 3) & 0x7f) | 0x80);
      m_SPI->write(((uint8_t)(address << 5) & 0xe0));
      uint8_t valueToReturn = m_SPI->read();
      m_SPI->deselect();
      return valueToReturn;      
    }
    
    
    void writeShortAddress(uint8_t address, uint8_t value)
    {
      uint8_t const dataToSend = (address << 1) | 0x01;      
      m_SPI->select();
      m_SPI->write(dataToSend);
      m_SPI->write(value);
      m_SPI->deselect();
    }
    
    
    uint8_t readShortAddress(uint8_t address)
    {
      uint8_t const dataToSend = address << 1;      
      m_SPI->select();
      m_SPI->write(dataToSend);
      uint8_t valueToReturn = m_SPI->read();
      m_SPI->deselect();
      return valueToReturn;
    }
    
    
  private:        

    // configuration
    HardwareSPIMaster* m_SPI;
    bool               m_sharedSPI;
  	Channel            m_channel;
    DeviceType         m_deviceType;
    uint16_t           m_PANID;
    LinkAddress const& m_address; // we use only first byte of this address
    Speed              m_speed;
    Array<ILinkLayerListener*, MAXLISTENERS> m_listeners; // upper layer listeners
    uint32_t           m_keyz;
    uint32_t           m_keyw;
    
	  // status
    bool     m_available;	  
    bool     m_frameSent;
    bool     m_frameReceived;
    uint16_t m_messageID;
    CircularBuffer<ReceivedMessageID, MAXALREADYRECEIVEDMESSAGES> m_receivedMessages;
  };
  
  

  
} // end of fdv namespace


#endif  // FDV_MRF24J40_H
