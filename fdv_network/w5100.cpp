/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 *
 * * 2010 modified by Fabrizio Di Vittorio
 */

#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>

#include "w5100.h"

#include "../fdv_generic/fdv_timesched.h"
#include "../fdv_generic/fdv_spi.h"
#include "../fdv_generic/fdv_pin.h"

// W5100 controller instance
//W5100Class W5100;

#define TX_RX_MAX_BUF_SIZE 2048
#define TX_BUF 0x1100
#define RX_BUF (TX_BUF + TX_RX_MAX_BUF_SIZE)

#define TXBUF_BASE 0x4000
#define RXBUF_BASE 0x6000

void W5100Class::init()
{
  fdv::delay(300);

  //initSS();
  
  writeMR(1<<RST);
  writeTMSR(0x55);
  writeRMSR(0x55);

  for (int i=0; i<MAX_SOCK_NUM; i++) {
    SBASE[i] = TXBUF_BASE + SSIZE * i;
    RBASE[i] = RXBUF_BASE + RSIZE * i;
  }

}

uint16_t W5100Class::getTXFreeSize(SOCKET s)
{
  uint16_t val=0, val1=0;
  do {
    val1 = readSnTX_FSR(s);
    if (val1 != 0)
      val = readSnTX_FSR(s);
  }
  while (val != val1);
  return val;
}

uint16_t W5100Class::getRXReceivedSize(SOCKET s)
{
  uint16_t val=0,val1=0;
  do {
    val1 = readSnRX_RSR(s);
    if (val1 != 0)
      val = readSnRX_RSR(s);
  }
  while (val != val1);
  return val;
}


void W5100Class::send_data_processing(SOCKET s, uint8_t *data, uint16_t len)
{
  uint16_t ptr = readSnTX_WR(s);

  uint16_t offset = ptr & SMASK;
  uint16_t dstAddr = offset + SBASE[s];

  if (offset + len > SSIZE)
  {
    // Wrap around circular buffer
    uint16_t size = SSIZE - offset;
    write(dstAddr, data, size);
    write(SBASE[s], data + size, len - size);
  }
  else {
    write(dstAddr, data, len);
  }

  ptr += len;
  writeSnTX_WR(s, ptr);
}


void W5100Class::recv_data_processing(SOCKET s, uint8_t *data, uint16_t len, uint8_t peek)
{
  uint16_t ptr;
  ptr = readSnRX_RD(s);
  read_data(s, (uint8_t *)ptr, data, len);
  if (!peek)
  {
    ptr += len;
    writeSnRX_RD(s, ptr);
  }
}

void W5100Class::read_data(SOCKET s, volatile uint8_t *src, volatile uint8_t *dst, uint16_t len)
{
  uint16_t size;
  uint16_t src_mask;
  uint16_t src_ptr;

  src_mask = (uint16_t)src & RMASK;
  src_ptr = RBASE[s] + src_mask;

  if( (src_mask + len) > RSIZE )
  {
    size = RSIZE - src_mask;
    read(src_ptr, (uint8_t *)dst, size);
    dst += size;
    read(RBASE[s], (uint8_t *) dst, len - size);
  }
  else
    read(src_ptr, (uint8_t *) dst, len);
}


uint8_t W5100Class::write(uint16_t _addr, uint8_t _data)
{
  setSS();
  m_spi->write(0xF0);
  m_spi->write(_addr >> 8);
  m_spi->write(_addr & 0xFF);
  m_spi->write(_data);
  resetSS();
  return 1;
}

uint16_t W5100Class::write(uint16_t _addr, uint8_t *_buf, uint16_t _len)
{
  for (uint16_t i=0; i<_len; i++)
  {
    setSS();
    m_spi->write(0xF0);
    m_spi->write(_addr >> 8);
    m_spi->write(_addr & 0xFF);
    _addr++;
    m_spi->write(_buf[i]);
    resetSS();
  }
  return _len;
}

uint8_t W5100Class::read(uint16_t _addr)
{
  uint8_t _data;
  setSS();
  m_spi->write(0x0F);
  m_spi->write(_addr >> 8);
  m_spi->write(_addr & 0xFF);
  _data = m_spi->read();
  resetSS();
  return _data;
}

uint16_t W5100Class::read(uint16_t _addr, uint8_t *_buf, uint16_t _len)
{
  for (uint16_t i=0; i<_len; i++)
  {
    setSS();
    m_spi->write(0x0F);
    m_spi->write(_addr >> 8);
    m_spi->write(_addr & 0xFF);
    _addr++;
    _buf[i] = m_spi->read();
    resetSS();
  }
  return _len;
}

void W5100Class::execCmdSn(SOCKET s, SockCMD _cmd)
{
  // Send command to socket
  writeSnCR(s, _cmd);
  // Wait for command to complete
  while (readSnCR(s))
    ;
}
