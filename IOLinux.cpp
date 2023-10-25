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

    double rxFreq = 434.0e6, txFreq = 434.0e6;

    int blockSize = 96;
    int resampNum = 1, resampDen = 1;
    int rxIfNum = 1, rxIfDen = 12;
    int txIfNum = 1, txIfDen = 12;

#if defined(LINUX_IO_LIMESDR)
    resampNum = 2;
    resampDen = 25;
    blockSize = 1024;
#endif

    m_buffer.resize(blockSize);
    m_dudc = new FDUDC(resampNum, resampDen, rxIfNum, rxIfDen, txIfNum, txIfDen, 11, 0.5f);
    double samplerate = 24000.0 * (double)resampDen / (double)resampNum;

#if defined(LINUX_IO_FILE)
    m_txFile = new std::ofstream("mmdvm_tx_iq_output.raw");
#elif defined(LINUX_IO_SOAPYSDR)
    m_latencyNs = (long long)std::round(1e9 / samplerate * (double)(blockSize * 3));

    SoapySDR::Kwargs device_args;
    SoapySDR::Kwargs rx_args;
    SoapySDR::Kwargs tx_args;
    size_t rx_channel = 0, tx_channel = 0;
#if defined(LINUX_IO_LIMESDR)
    device_args["driver"] = "lime";
    rx_args["latency"] = "0";
    tx_args["latency"] = "0";
#endif

    try {
        m_device = SoapySDR::Device::make(device_args);
        m_device->setSampleRate(SOAPY_SDR_RX, rx_channel, samplerate);
        m_device->setSampleRate(SOAPY_SDR_TX, tx_channel, samplerate);
        m_device->setFrequency(SOAPY_SDR_RX, rx_channel, rxFreq - samplerate * (double)rxIfNum / (double)rxIfDen);
        m_device->setFrequency(SOAPY_SDR_TX, tx_channel, txFreq - samplerate * (double)txIfNum / (double)txIfDen);
        m_device->setAntenna(SOAPY_SDR_RX, rx_channel, "LNAL");
        m_device->setAntenna(SOAPY_SDR_TX, tx_channel, "BAND1");
        m_device->setGain(SOAPY_SDR_RX, rx_channel, 50.0);
        m_device->setGain(SOAPY_SDR_TX, tx_channel, 30.0);
        m_rxStream = m_device->setupStream(SOAPY_SDR_RX, "CF32", {rx_channel}, rx_args);
        m_txStream = m_device->setupStream(SOAPY_SDR_TX, "CF32", {tx_channel}, tx_args);
        LOGCONSOLE("SoapySDR device setup done");
    } catch (std::runtime_error &e) {
        LOGCONSOLE("Error setting up SoapySDR device: %s", e.what());
        m_running = 0;
    }
#endif
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
        const float tx_amplitude = 0.7f;

        std::complex<float> tx_iq_sample = { 0.0f, 0.0f };
        TSample fm_sample = { DC_OFFSET, MARK_NONE };
        if (m_txBuffer.getData() >= 1) {
            m_txBuffer.get(fm_sample);

            // Modulate TX FM
            m_phase += ((int32_t)fm_sample.sample - (int32_t)DC_OFFSET) * fm_deviation;
            float ph = m_phase * (float)(M_PI / 0x80000000UL);
            tx_iq_sample = std::polar(tx_amplitude, ph);
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
    processIqBlock(m_buffer);
    m_txFile->write((char*)m_buffer.data(), m_buffer.size() * sizeof(std::complex<float>));
    // Simulate I/O happening at roughly the correct rate.
    usleep(1000000 / 24000 * m_buffer.size());
#endif
#if defined(LINUX_IO_SOAPYSDR)
    if (m_device == NULL || m_rxStream == NULL || m_txStream == NULL) {
        // Initialization has failed.
        return;
    }
    if (!m_streamsOn) {
        m_device->activateStream(m_rxStream);
        m_device->activateStream(m_txStream);
        m_streamsOn = true;
    }

    void *buffs[1] = { (void*)m_buffer.data() };
    int flags = 0;
    long long timeNs = 0;
    int ret = 0;
    ret = m_device->readStream(m_rxStream, buffs, m_buffer.size(), flags, timeNs);
    //LOGCONSOLE("RX: %d", ret);
    if (ret <= 0) {
        // TODO: report or handle errors
    }
    processIqBlock(m_buffer);
    timeNs += m_latencyNs;
    flags = SOAPY_SDR_HAS_TIME;
    ret = m_device->writeStream(m_txStream, buffs, m_buffer.size(), flags, timeNs);
    //LOGCONSOLE("TX: %d", ret);
    if (ret <= 0) {
        // TODO: report or handle errors
    }
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
