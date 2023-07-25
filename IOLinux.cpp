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

#include <unistd.h>

#include "Config.h"
#include "Globals.h"
#include "Debug.h"
#include "IO.h"

#include <SoapySDR/Device.hpp>

void CIO::initInt()
{
    LOGCONSOLE("Initializing I/O");
}

void CIO::startInt()
{
    LOGCONSOLE("Starting I/O");
}

void CIO::processInt()
{
    // TODO
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
