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

// Instead of communicating with a real serial port, this Linux implementation
// creates a pseudoterminal acting as a "virtual" serial port.
// Based on Patrick Maier's code
// https://github.com/maierp/MMDVMSdr/blob/master/SerialPort.cpp

#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "Config.h"
#include "Globals.h"
#include "SerialPort.h"

#define BAUDRATE B460800
#define SERIAL_DEVICE_FILE "/tmp/MMDVM_PTS"

void CSerialPort::beginInt(uint8_t n, int speed)
{
    // Only implement a virtual port for host communication (n=1).
    // The repeater port (n=3) could be connected to a real serial port
    // when needed but that is not a critical feature for now.
    if (n == 1) {
        // Create virtual serial Port
        m_serial_fd = open("/dev/ptmx", O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (m_serial_fd < 0) {
            LOGCONSOLE("Failed to open /dev/ptmx: %s", strerror(errno));
            return;
        }
        grantpt(m_serial_fd);
        unlockpt(m_serial_fd);
        char* pts_name = ptsname(m_serial_fd);
        if (pts_name == NULL) {
            LOGCONSOLE("Failed to get pseudoterminal name");
            return;
        }
        LOGCONSOLE("Pseudoterminal name: %s", pts_name);

        const char *symlink_path = SERIAL_DEVICE_FILE;
        // Try to remove the virtual serial port if it already exists
        (void)remove(symlink_path);
        // Create symlink to virtual serial port
        if (symlink(pts_name, symlink_path) < 0) {
            LOGCONSOLE("Failed to create pseudoterminal symlink at %s: %s", symlink_path, strerror(errno));
        } else {
            LOGCONSOLE("Pseudoterminal symlinked to %s", symlink_path);
        }

        /* serial port parameters */
        struct termios newtio = {};
        struct termios oldtio = {};
        tcgetattr(m_serial_fd, &oldtio);

        newtio = oldtio;
        newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
        newtio.c_iflag = 0;
        newtio.c_oflag = 0;
        newtio.c_lflag = 0;
        newtio.c_cc[VMIN] = 1;
        newtio.c_cc[VTIME] = 0;
        tcflush(m_serial_fd, TCIFLUSH);

        cfsetispeed(&newtio, BAUDRATE);
        cfsetospeed(&newtio, BAUDRATE);
        tcsetattr(m_serial_fd, TCSANOW, &newtio);
    }
}

int CSerialPort::availableForReadInt(uint8_t n)
{
    int available = 0;
    if (n == 1) {
        ioctl(m_serial_fd, FIONREAD, &available);
    }
    return available;
}

int CSerialPort::availableForWriteInt(uint8_t n)
{
    // TODO: figure out whether a proper implementation is needed.
    return 100;
}

uint8_t CSerialPort::readInt(uint8_t n)
{
    uint8_t byte = 0xFFU;
    if (n == 1) {
        // Making a separate syscall to read each byte
        // is not the most efficient way but it is easiest.
        // Ignoring return value of read is fine here. Even if read fails,
        // the function has to return some uint8_t value anyway.
        (void)read(m_serial_fd, &byte, 1);
    }
    return byte;
}

void CSerialPort::writeInt(uint8_t n, const uint8_t* data, uint16_t length, bool flush)
{
    if (n == 1) {
        // TODO: handle the case where everything is not written at once.
        write(m_serial_fd, data, length);
    }
}

#endif
