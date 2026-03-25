/*
 *   Copyright (C) 2023,2026 by Tatu Peltola OH2EAT
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

#include "Config.h"
#include "Globals.h"
#include "SerialPort.h"
#include "SerialLinux.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#define BAUDRATE B460800

CSerialLinux::CSerialLinux():
  m_fd(-1),
  m_rxBufferDataLen(0),
  m_rxBufferReadPos(0),
  m_txBufferDataLen(0)
{
}

int CSerialLinux::getFd()
{
  return m_fd;
}

void CSerialLinux::begin(const char *symlink_path)
{
  // Create virtual serial port
  m_fd = ::open("/dev/ptmx", O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (m_fd < 0) {
    ::perror("\nFailed to open /dev/ptmx");
    ::exit(1);
    return;
  }
  ::grantpt(m_fd);
  ::unlockpt(m_fd);
  char* pts_name = ::ptsname(m_fd);
  if (pts_name == NULL) {
    ::perror("\nFailed to get pseudoterminal name");
    ::exit(1);
    return;
  }
  ::fprintf(stderr, "\nPseudoterminal name: %s\n", pts_name);

  // Try to remove the symlink if it already exists
  (void)::unlink(symlink_path);
  // Create symlink to virtual serial port
  if (::symlink(pts_name, symlink_path) < 0) {
    ::fprintf(stderr, "Failed to create pseudoterminal symlink at %s: %s\n", symlink_path, ::strerror(errno));
    ::exit(1);
  } else {
    ::fprintf(stderr, "Pseudoterminal symlinked to %s\n", symlink_path);
  }

  /* serial port parameters */
  struct termios newtio = {};
  struct termios oldtio = {};
  ::tcgetattr(m_fd, &oldtio);

  newtio = oldtio;
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = 0;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VMIN] = 1;
  newtio.c_cc[VTIME] = 0;
  ::tcflush(m_fd, TCIFLUSH);

  ::cfsetispeed(&newtio, BAUDRATE);
  ::cfsetospeed(&newtio, BAUDRATE);
  ::tcsetattr(m_fd, TCSANOW, &newtio);
}

void CSerialLinux::receive()
{
  // Read bytes from virtual serial port to buffer.

  // Looking at CSerialPort::process(), it looks like
  // it processes all bytes available in the buffer,
  // so by the time SerialLinux::receive() gets called again,
  // all previous bytes should have been processed.
  // This means remaining data in the buffer
  // does not need to be handled.
  // assert to make sure in case I missed something in the code.
  assert(m_rxBufferReadPos == m_rxBufferDataLen);

  m_rxBufferReadPos = 0;
  m_rxBufferDataLen = 0;

  ssize_t ret = ::read(m_fd, m_rxBuffer, SERIAL_LINUX_RX_BUFFER_SIZE);
  if (ret >= 0) {
    m_rxBufferDataLen = ret;
  } else {
    ::perror("\nError reading from pseudoterminal\n");
    return;
  }
}

int CSerialLinux::availableForRead()
{
  return m_rxBufferDataLen - m_rxBufferReadPos;
}

uint8_t CSerialLinux::read()
{
  if (m_rxBufferReadPos < m_rxBufferDataLen) {
    return m_rxBuffer[m_rxBufferReadPos++];
  } else {
    return 0xFF;
  }
}

int CSerialLinux::availableForWrite()
{
  return SERIAL_LINUX_TX_BUFFER_SIZE - m_txBufferDataLen;
}

void CSerialLinux::write(const uint8_t* data, uint16_t length)
{
  assert(m_txBufferDataLen + length <= SERIAL_LINUX_TX_BUFFER_SIZE);
  memcpy(m_txBuffer + m_txBufferDataLen, data, length);
  m_txBufferDataLen += length;
}

void CSerialLinux::transmit()
{
  const uint8_t *data = m_txBuffer;
  ssize_t length = m_txBufferDataLen;
  m_txBufferDataLen = 0;
  while (length > 0) {
    int ret = ::write(m_fd, data, length);
    if (ret >= 0) {
      data += ret;
      length -= ret;
    } else {
      ::perror("\nError writing to pseudoterminal");
      return;
    }
  }
}


void CSerialPort::beginInt(uint8_t n, int speed)
{
  // Only implement a virtual port for host communication (n=1).
  // The repeater port (n=3) could be connected to a real serial port
  // when needed but that is not a critical feature for now.
  if (n == 1) {
    serial1.begin(HOST_PTS_PATH);
  }
}

int CSerialPort::availableForReadInt(uint8_t n)
{
  if (n == 1) {
    return serial1.availableForRead();
  }
  return 0;
}

int CSerialPort::availableForWriteInt(uint8_t n)
{
  if (n == 1) {
    return serial1.availableForWrite();
  }
  return 0;
}

uint8_t CSerialPort::readInt(uint8_t n)
{
  if (n == 1) {
    return serial1.read();
  }
  return 0xFF;
}

void CSerialPort::writeInt(uint8_t n, const uint8_t* data, uint16_t length, bool flush)
{
  if (n == 1) {
    serial1.write(data, length);
    if (flush) {
      serial1.transmit();
    }
  }
}

#endif
