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

#include <stdio.h>
#include <unistd.h>

void CIO::initInt()
{
    // TODO
}

void CIO::startInt()
{
    // TODO
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
    }\

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
