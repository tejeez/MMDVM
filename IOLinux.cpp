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
#include <signal.h>
#include <unistd.h>

static const uint16_t DC_OFFSET = 2048U;
static const uint16_t LINUX_IO_BLOCK_SIZE = 96U;

void sighandler(int sig)
{
    (void)sig;
    m_running = 0;
}

static void setup_sighandler(void)
{
    // Setup a signal handler, so that I/O device can be
    // correctly shut down when the program is terminated.
    struct sigaction sigact;
    sigact.sa_handler = sighandler;
    ::sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    ::sigaction(SIGHUP, &sigact, NULL);
    ::sigaction(SIGINT, &sigact, NULL);
    ::sigaction(SIGQUIT, &sigact, NULL);
    ::sigaction(SIGTERM, &sigact, NULL);
    ::sigaction(SIGPIPE, &sigact, NULL);
}

void CIO::initInt()
{
    LOGCONSOLE("Initializing I/O");
    setup_sighandler();

#if defined(LINUX_IO_FILE)
    m_txFile = new std::ofstream("mmdvm_tx_iq_output.raw");
#elif defined(LINUX_IO_SOAPYSDR)

    SoapySDR::Kwargs device_args;
    SoapySDR::Kwargs rx_args;
    SoapySDR::Kwargs tx_args;
    size_t rx_channel = 0, tx_channel = 0;
    double samplerate = 0;
#if defined(LINUX_IO_LIMESDR)
    device_args["driver"] = "lime";
    rx_args["latency"] = "0";
    tx_args["latency"] = "0";
    samplerate = 240000;
#endif

    try {
        m_device = SoapySDR::Device::make(device_args);
        m_device->setSampleRate(SOAPY_SDR_RX, rx_channel, samplerate);
        m_device->setSampleRate(SOAPY_SDR_TX, tx_channel, samplerate);
        m_rxStream = m_device->setupStream(SOAPY_SDR_RX, "CF32", {rx_channel}, rx_args);
        m_txStream = m_device->setupStream(SOAPY_SDR_TX, "CF32", {tx_channel}, tx_args);
        LOGCONSOLE("SoapySDR device setup done");
    } catch (std::runtime_error &e) {
        LOGCONSOLE("Error setting up SoapySDR device: %s", e.what());
        m_running = 0;
    }
#endif
    m_dudc = new FDUDC(24, 125, 3, 40, 3, 40);
}

void CIO::exitInt()
{
    LOGCONSOLE("Closing I/O");
#if defined(LINUX_IO_SOAPYSDR)
    if (m_device != NULL)
        SoapySDR::Device::unmake(m_device);
#endif
}

void CIO::processIqBlock(std::vector<std::complex<float>> &buf)
{
    m_dudc->process(buf, [this](std::complex<float> rx_iq_sample) {
        const int32_t fm_deviation = 500000; // TODO

        std::complex<float> tx_iq_sample = { 0.0f, 0.0f };
        TSample fm_sample = { DC_OFFSET, MARK_NONE };
        if (m_txBuffer.getData() >= 1) {
            m_txBuffer.get(fm_sample);

            // Modulate TX FM
            m_phase += ((int32_t)fm_sample.sample - (int32_t)DC_OFFSET) * fm_deviation;
            float ph = m_phase * (float)(M_PI / 0x80000000UL);
            tx_iq_sample = { cosf(ph), sinf(ph) };
        }

        // Demodulate RX FM
        auto d = std::arg(rx_iq_sample * std::conj(m_prev_rx_iq_sample));
        m_prev_rx_iq_sample = rx_iq_sample;
        // Scale -pi...pi to 0...DC_OFFSET*2
        d = d * ((float)DC_OFFSET / (float)M_PI) + (float)DC_OFFSET;

        // TODO: delay slot flags

        fm_sample.sample = (uint16_t)d;
        m_rxBuffer.put(fm_sample);
        m_rssiBuffer.put(0U);
        return tx_iq_sample;
    });
}

void CIO::startInt()
{
    LOGCONSOLE("Starting I/O");
}

void CIO::processInt()
{
#if defined(LINUX_IO_FILE)
    // Read TX buffer and write RX buffer in blocks,
    // somewhat simulating SDR or sound-card I/O.
    std::vector<std::complex<float>> buf(LINUX_IO_BLOCK_SIZE);
    processIqBlock(buf);
    m_txFile->write((char*)buf.data(), buf.size() * sizeof(std::complex<float>));
    // Simulate I/O happening at roughly the correct rate.
    usleep(1000000 / 24000 * LINUX_IO_BLOCK_SIZE);
#endif
#if defined(LINUX_IO_SOAPYSDR)
    if (m_device == NULL || m_rxStream == NULL || m_txStream == NULL) {
        // Initialization has failed.
        return;
    }

    // TODO: handle streams
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
