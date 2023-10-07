/*
 *   Copyright (C) 2023 by Tatu Peltola OH2EAT
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
#include "Debug.h"
#include "IO.h"

#include <stdio.h>
#include <unistd.h>

#include <SoapySDR/Device.hpp>

static const uint16_t DC_OFFSET = 2048U;
static const uint16_t LINUX_IO_BLOCK_SIZE = 2U;

void CIO::initInt()
{
    LOGCONSOLE("Initializing I/O");
#if defined(LINUX_IO_FILE)
    tx_output_fd = new std::ofstream("mmdvm_tx_output.raw");
#endif
}

void CIO::startInt()
{
    LOGCONSOLE("Starting I/O");
}

void CIO::processInt()
{
    // TODO
#if defined(LINUX_IO_FILE)
    // Read TX buffer and write RX buffer in blocks,
    // somewhat simulating SDR or sound-card I/O.
    uint16_t tx_output_buf[LINUX_IO_BLOCK_SIZE * 2];
    for (size_t i = 0; i < LINUX_IO_BLOCK_SIZE; i++) {
        TSample sample = {DC_OFFSET, MARK_NONE};
        m_txBuffer.get(sample);
        tx_output_buf[i * 2    ] = sample.sample;
        tx_output_buf[i * 2 + 1] = sample.control;
        // Transfer marks into the RX buffer
        // but use silence as the signal for now.
        sample.sample = DC_OFFSET;
        m_rxBuffer.put(sample);
        m_rssiBuffer.put(0U);
    }

    tx_output_fd->write((char*)tx_output_buf, sizeof(tx_output_buf));
    // Simulate I/O happening at roughly the correct rate.
    usleep(1000000 / 24000 * LINUX_IO_BLOCK_SIZE);
#endif
}

bool CIO::getCOSInt()
{
    return false;
}

static const char *on_off(bool on)
{
    return on ? "ON" : "off";
}

void CIO::setLEDInt(bool on)
{
    LOGCONSOLE("LED led %s", on_off(on));
}

void CIO::setPTTInt(bool on)
{
    LOGCONSOLE("PTT led %s", on_off(on));
}

void CIO::setCOSInt(bool on)
{
    LOGCONSOLE("COS led %s", on_off(on));
}

uint8_t CIO::getCPU() const
{
    return 3U;
}

void CIO::getUDID(uint8_t* buffer)
{
    memset(buffer, 0, 16U);
}

void CIO::delayInt(unsigned int dly)
{
    usleep(dly * 1000U);
}

#endif
