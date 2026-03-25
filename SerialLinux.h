/*
 *   Copyright (C) 2026 by Tatu Peltola OH2EAT
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

#if !defined(SERIAL_LINUX_H) && defined(LINUX)
#define SERIAL_LINUX_H

#define SERIAL_LINUX_RX_BUFFER_SIZE 0x200
#define SERIAL_LINUX_TX_BUFFER_SIZE 0x200

class SerialLinux {
public:
    SerialLinux();
    int get_fd();
    void begin(const char *symlink_path);
    void receive();
    int availableForRead();
    uint8_t read();
    int availableForWrite();
    void write(const uint8_t* data, uint16_t length);
    void transmit();

private:
    int m_fd;
    ssize_t m_rxBufferDataLen;
    ssize_t m_rxBufferReadPos;
    ssize_t m_txBufferDataLen;
    uint8_t m_rxBuffer[SERIAL_LINUX_RX_BUFFER_SIZE];
    uint8_t m_txBuffer[SERIAL_LINUX_TX_BUFFER_SIZE];
};

#endif
