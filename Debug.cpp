/*
 *   Copyright (C) 2023 by Tatu Peltola OH2EAT
 *   Copyright (C) 2019 by Patrick Maier DK5MP
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

#include <time.h>
#include <sys/time.h>
#include <cstdarg>
#include <cstdio>

#include "Debug.h"

void LOGCONSOLE(const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    struct timeval now;
    gettimeofday(&now, NULL);
    struct tm* tm = gmtime(&now.tv_sec);
    printf("%04d-%02d-%02d %02d:%02d:%02d.%03lu ", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, now.tv_usec / 1000U);
    vprintf(msg, args);
    printf("\n");
}

#endif
