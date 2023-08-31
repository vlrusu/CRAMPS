/* Arduino I2cMaster Library
 * Copyright (C) 2010 by William Greiman
 *
 * This file is part of the Arduino I2cMaster Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino I2cMaster Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef MI2C_H
#define MI2C_H

#include "MCP23S17.h"
#include <stdio.h>
#include <string.h>

#define MI2C_ACK 0

/** Delay used for software MI2C */
#define MI2C_DELAY_USEC 1

/** Bit to or with address for read start and read restart */
#define MI2C_READ 1

/** Bit to or with address for write start and write restart */
#define MI2C_WRITE 0

typedef struct{
    MCP *_mcp_scl; // SPI expander for clock line of the MI2C bus
    MCP *_mcp_sda; // SPI expander for data line of the I2c bus
    uint16_t _sdaPinMask; // pin number on _mcp_sda for sda pin
    uint8_t _nCramps;
    uint16_t _sclPin; // pin number on _mcp_scl for scl pin
} MI2C;

// sets mcp and pins, and sets pins to outputs and HIGH
void MI2C_setup(MI2C *self, MCP *mcp_sda, uint16_t sdaPinMask, MCP *mcp_scl, uint8_t sclPin);

// Performs an 8 bit read using 9 total clocks of MI2C bus.
// During a multi byte read, master holds data low for each ack except for last read.
// If last == 1 master will not ack and the read will be ended.
void MI2C_read(MI2C* self, uint8_t last, uint16_t*);

// Sets up a transfer by first setting sda and scl low and then
// writing the MI2C address of the device using 9 clocks
// addressRW == Upper 7 bits are device MI2C address, lowest bit is R/W
// the slave device will hold data low for ack. return value is this ack bit, so
// a return value of 1 means an error has occured.
uint8_t MI2C_start(MI2C* self, uint8_t addressRW);

// Stops the transfer by setting sda low, then scl high, then sda high
void MI2C_stop(MI2C* self);

// Performs an 8 bit write using 9 total clocks of the MI2C bus.
// During a write the slave device will hold data low for ack. return value is this ack bit, so
// a return value of 1 means an error has occured.
uint8_t MI2C_write(MI2C* self, uint8_t data);

#endif  // MI2C_MASTER_H
