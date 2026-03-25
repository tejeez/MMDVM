/*
 *   Copyright (C) 2023,2026 by Tatu Peltola OH2EAT
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if defined(LINUX)

#include "Config.h"
#include "Globals.h"
#include "IO.h"

#include <cassert>
#include <cstdio>
#include <unistd.h>

int CIO::getRxFd()
{
  return m_rxFd;
}

void CIO::initInt()
{
  // TODO
}

void CIO::startInt()
{
  // TODO
}

#define MAX_RX_PACKET_SIZE 8192U
void CIO::receive()
{
  uint8_t packet[MAX_RX_PACKET_SIZE];

  ssize_t packetLen = ::read(m_rxFd, packet, MAX_RX_PACKET_SIZE);
  if (packetLen < 16) {
    return;
  }

  ssize_t pos = 8;
  uint64_t sampleCount = 0;
  for (int i = 0; i < 8; i++) {
    sampleCount |= packet[pos++] << (8 * i);
  }

  while (pos + 3 < packetLen) {
    m_rxBuffer.put(TRxSample {
      .count = sampleCount,
      .sample = ((uint16_t)packet[pos  ]) | (((uint16_t)packet[pos+1]) << 8U),
      .rssi   = ((uint16_t)packet[pos+2]) | (((uint16_t)packet[pos+3]) << 8U),
    });
    pos += 4;
    sampleCount++;
  }
}

void CIO::transmit()
{
  ssize_t ret = ::write(m_txFd, m_txPacket, m_txPacketLen);
  if (ret > 0) {
    m_txPacketLen = 0;
  }
}

uint16_t CIO::getRxAvailable() const
{
  return m_rxBuffer.getData();
}

void CIO::getRxSampleAndRssiInt(TSample& sample, uint16_t& rssi)
{
  TRxSample bufferSample = { 0U, 0U, 0U };
  if (m_rxBuffer.get(bufferSample)) {

    sample.sample = bufferSample.sample;
    rssi = bufferSample.rssi;

    // Get control flags from a separate buffer based on sample count
    uint64_t c = m_controlBuffer[bufferSample.count & (CONTROL_BUFFER_SIZE - 1)];
    if ((c & ~0xFFULL) == (bufferSample.count & ~0xFFULL)) {
      sample.control = c & 0xFFU;
    } else {
      sample.control = MARK_NONE;
    }

    m_lastRxSampleProcessed = bufferSample.count;
  }
}


// How far ahead from latest processed RX sample
// the first TX sample after a pause should be timed.
static const uint64_t TX_BEGIN_AHEAD = 360UL; // 15ms

void CIO::putTxSampleInt(TSample sample)
{
  if (m_txSampleCounter - m_lastRxSampleProcessed >= 1ULL << 63U) {
    // TX sample counter is behind RX sample counter, i.e. in the past.
    // This may happen for the first TX sample after a pause in TX.
    // Forward TX counter txBeginAhead samples to the future.
    m_txSampleCounter = m_lastRxSampleProcessed + TX_BEGIN_AHEAD;
  }

  if (m_txPacketLen == 0) {
    // Add header
    for (int i = 0; i < 8; i++) {
      m_txPacket[m_txPacketLen++] = 0;
    }
    uint64_t v = m_txSampleCounter;
    for (int i = 0; i < 8; i++) {
      m_txPacket[m_txPacketLen++] = v;
      v >>= 8;
    }
  }

  if (m_txPacketLen + 4 <= MAX_TX_PACKET_SIZE) {
    m_txPacket[m_txPacketLen++] = sample.sample;
    m_txPacket[m_txPacketLen++] = sample.sample >> 8;
    m_txPacket[m_txPacketLen++] = 0;
    m_txPacket[m_txPacketLen++] = 0;

    m_controlBuffer[m_txSampleCounter & (CONTROL_BUFFER_SIZE - 1)] =
      (m_txSampleCounter & ~0xFFULL) | (sample.control & 0xFFULL);

    m_txSampleCounter++;
  }
}

uint16_t CIO::getSpace() const
{
  // Simulate given TX buffer size by limiting how far ahead
  // from latest processed RX sample we let TX samples to be produced.
  uint64_t ahead = m_txSampleCounter - m_lastRxSampleProcessed;
  if (ahead >= 1ULL << 63U) {
    // Wrapped around, meaning m_txSampleCounter is behind m_lastRxSampleProcessed.
    // putTxSampleInt will move it to m_lastRxSampleProcessed + TX_BEGIN_AHEAD.
    ahead = TX_BEGIN_AHEAD;
  }

  uint16_t space = (ahead >= (uint64_t)TX_RINGBUFFER_SIZE)
    ? 0
    : TX_RINGBUFFER_SIZE - ahead;

  uint16_t maxForPacket = (MAX_TX_PACKET_SIZE - m_txPacketLen) / 4U;
  return space < maxForPacket ? space : maxForPacket;
}

bool CIO::hasTXOverflow()
{
  return m_txPacketLen + 4 > MAX_TX_PACKET_SIZE;
}

bool CIO::hasEmptyTXBufferInt()
{
  return m_txSampleCounter - m_lastRxSampleProcessed >= 1ULL << 63U;
}




bool CIO::getCOSInt()
{
  return false;
}

void CIO::delayInt(unsigned int dly)
{
  ::usleep(dly * 1000U);
}

uint8_t CIO::getCPU() const
{
  return 3U;
}

void CIO::getUDID(uint8_t* buffer)
{
  ::memset(buffer, 0, 16U);
}


struct OutputPins {
  bool LED, PTT, COS;
  bool DStar, DMR, YSF, P25, NXDN, POCSAG, M17, FM;
};
struct OutputPins m_outputPins = {};

#define PRINT_PIN(name)\
  if (m_outputPins.name) {\
    ::fprintf(stderr, "\033[7m" #name "\033[0m ");\
  } else {\
    ::fprintf(stderr, "\033[2m" #name "\033[0m ");\
  }

static void printOutputPins(void)
{
  ::fprintf(stderr, "\r");
  PRINT_PIN(LED)
  PRINT_PIN(PTT)
  PRINT_PIN(COS)

  PRINT_PIN(DStar)
  PRINT_PIN(DMR)
  PRINT_PIN(YSF)
  PRINT_PIN(P25)
  PRINT_PIN(NXDN)
  PRINT_PIN(POCSAG)
  PRINT_PIN(M17)
  PRINT_PIN(FM)
}

#define CIO_SET_PIN(name)\
void CIO::set ## name ## Int(bool on) {\
  m_outputPins.name = on;\
  printOutputPins();\
}

CIO_SET_PIN(LED)
CIO_SET_PIN(PTT)
CIO_SET_PIN(COS)

CIO_SET_PIN(DStar)
CIO_SET_PIN(DMR)
CIO_SET_PIN(YSF)
CIO_SET_PIN(P25)
CIO_SET_PIN(NXDN)
CIO_SET_PIN(POCSAG)
CIO_SET_PIN(M17)
CIO_SET_PIN(FM)

#endif
