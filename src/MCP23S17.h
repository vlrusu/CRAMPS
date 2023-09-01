/*
  MCP23S17.h  Version 0.1
  Microchip MCP23S17 SPI I/O Expander Class for Arduino
  Created by Cort Buffington & Keith Neufeld
  March, 2011

  Features Implemented (by word and bit):
    I/O Direction
    Pull-up on/off
    Input inversion
    Output write
    Input read

  Interrupt features are not implemented in this version
  byte based (portA, portB) functions are not implemented in this version

  NOTE:  Addresses below are only valid when IOCON.BANK=0 (register addressing mode)
         This means one of the control register values can change register addresses!
         The default values is 0, so that's how we're using it.

         All registers except ICON (0xA and 0xB) are paired as A/B for each 8-bit GPIO port.
         Comments identify the port's name, and notes on how it is used.

         *THIS CLASS ENABLES THE ADDRESS PINS ON ALL CHIPS ON THE BUS WHEN THE FIRST CHIP OBJECT IS INSTANTIATED!

  USAGE: All Read/Write functions except wordWrite are implemented in two different ways.
         Individual pin values are set by referencing "pin #" and On/Off, Input/Output or High/Low where
         portA represents pins 0-7 and portB 8-15. So to set the most significant bit of portB, set pin # 15.
         To Read/Write the values for the entire chip at once, a word mode is supported buy passing a
         single argument to the function as 0x(portB)(portA). I/O mode Output is represented by 0.
         The wordWrite function was to be used internally, but was made public for advanced users to have
         direct and more efficient control by writing a value to a specific register pair.
*/

#ifndef MCP23S17_h
#define MCP23S17_h



// REGISTERS ARE DEFINED HERE SO THAT THEY MAY BE USED IN THE MAIN PROGRAM

#define    IODIRA    (0x00)      // MCP23x17 I/O Direction Register
#define    IODIRB    (0x01)      // 1 = Input (default), 0 = Output

#define    IPOLA     (0x02)      // MCP23x17 Input Polarity Register
#define    IPOLB     (0x03)      // 0 = Normal (default)(low reads as 0), 1 = Inverted (low reads as 1)

#define    GPINTENA  (0x04)      // MCP23x17 Interrupt on Change Pin Assignements
#define    GPINTENB  (0x05)      // 0 = No Interrupt on Change (default), 1 = Interrupt on Change

#define    DEFVALA   (0x06)      // MCP23x17 Default Compare Register for Interrupt on Change
#define    DEFVALB   (0x07)      // Opposite of what is here will trigger an interrupt (default = 0)

#define    INTCONA   (0x08)      // MCP23x17 Interrupt on Change Control Register
#define    INTCONB   (0x09)      // 1 = pin is compared to DEFVAL, 0 = pin is compared to previous state (default)

#define    IOCON     (0x0A)      // MCP23x17 Configuration Register
//                   (0x0B)      //     Also Configuration Register

#define    GPPUA     (0x0C)      // MCP23x17 Weak Pull-Up Resistor Register
#define    GPPUB     (0x0D)      // INPUT ONLY: 0 = No Internal 100k Pull-Up (default) 1 = Internal 100k Pull-Up

#define    INTFA     (0x0E)      // MCP23x17 Interrupt Flag Register
#define    INTFB     (0x0F)      // READ ONLY: 1 = This Pin Triggered the Interrupt

#define    INTCAPA   (0x10)      // MCP23x17 Interrupt Captured Value for Port Register
#define    INTCAPB   (0x11)      // READ ONLY: State of the Pin at the Time the Interrupt Occurred

#define    GPIOA     (0x12)      // MCP23x17 GPIO Port Register
#define    GPIOB     (0x13)      // Value on the Port - Writing Sets Bits in the Output Latch

#define    OLATA     (0x14)      // MCP23x17 Output Latch Register
#define    OLATB     (0x15)      // 1 = Latch High, 0 = Latch Low (default) Reading Returns Latch State, Not Port Value!


/*
#define    IODIRA    (0x00)      // MCP23x17 I/O Direction Register
#define    IPOLA     (0x01)      // MCP23x17 Input Polarity Register
#define    GPINTENA  (0x02)      // MCP23x17 Interrupt on Change Pin Assignements
#define    DEFVALA   (0x03)      // MCP23x17 Default Compare Register for Interrupt on Change
#define    INTCONA   (0x04)      // MCP23x17 Interrupt on Change Control Register
#define    IOCON     (0x05)      // MCP23x17 Configuration Register
//                   (0x0B)      //     Also Configuration Register

#define    GPPUA     (0x06)      // MCP23x17 Weak Pull-Up Resistor Register
#define    INTFA     (0x07)      // MCP23x17 Interrupt Flag Register
#define    INTCAPA   (0x08)      // MCP23x17 Interrupt Captured Value for Port Register
#define    GPIOA     (0x09)      // MCP23x17 GPIO Port Register
#define    OLATA     (0x0A)      // MCP23x17 Output Latch Register

#define    IODIRB    (0x10)      // MCP23x17 I/O Direction Register
#define    IPOLB     (0x11)      // MCP23x17 Input Polarity Register
#define    GPINTENB  (0x12)      // MCP23x17 Interrupt on Change Pin Assignements
#define    DEFVALB   (0x13)      // MCP23x17 Default Compare Register for Interrupt on Change
#define    INTCONB   (0x14)      // MCP23x17 Interrupt on Change Control Register

#define    GPPUB     (0x16)      // MCP23x17 Weak Pull-Up Resistor Register
#define    INTFB     (0x17)      // MCP23x17 Interrupt Flag Register
#define    INTCAPB   (0x18)      // MCP23x17 Interrupt Captured Value for Port Register
#define    GPIOB     (0x19)      // MCP23x17 GPIO Port Register
#define    OLATB     (0x1A)      // MCP23x17 Output Latch Register

*/


//corresponding registers for MCP23x09
/* #define    IODIR09    (0x00) */
/* #define    IPOL09     (0x01) */
/* #define    GPINTEN09  (0x02) */
/* #define    DEFVAL09   (0x03) */
/* #define    INTCON09   (0x04) */
/* #define    IOCON09    (0x05) */
/* #define    GPPU09     (0x06) */
/* #define    INTF09     (0x07) */
/* #define    INTCAP09   (0x08) */
/* #define    GPIO09     (0x09) */
/* #define    OLAT09     (0x0A) */

#define MCP_SPI_MODE 0

#define MCP_INPUT 1
#define MCP_OUTPUT 0


#define PUD_OFF                  0
#define PUD_DOWN                 1
#define PUD_UP                   2


// Bits in the IOCON register

#define IOCON_UNUSED    0x01
#define IOCON_INTPOL    0x02
#define IOCON_ODR       0x04
#define IOCON_HAEN      0x08
#define IOCON_DISSLW    0x10
#define IOCON_SEQOP     0x20
#define IOCON_MIRROR    0x40
#define IOCON_BANK_MODE 0x80


//#define IOCON_INIT      (IOCON_SEQOP| IOCON_HAEN)

#define IOCON_INIT      (IOCON_HAEN)



#include <stdio.h>
#include <string.h>
#include <stdint.h>



typedef struct{
    uint8_t _address;                        // Address of the MCP23S17 in use
    uint16_t _outputCache;
    uint16_t _pullupCache;
    uint16_t _invertCache;
    uint16_t _modeCache;
} MCP;

void MCP_setup( MCP* mcp, uint8_t address);



    void MCP_byteWrite(MCP *mcp, uint8_t, uint8_t);        // Typically only used internally, but allows the user to write any register if needed, so it's public
    void MCP_pinMode(MCP *mcp, uint8_t, uint8_t);          // Sets the mode (input or output) of a single I/O pin

    void MCP_pullupMode(MCP *mcp, uint8_t, uint8_t);       // Selects internal 100k input pull-up of a single I/O pin

    void MCP_pinWrite(MCP *mcp, uint8_t, uint8_t);     // Sets an individual output pin HIGH or LOW

    uint8_t MCP_pinRead(MCP *mcp, uint8_t);            // Reads an individual input pin
    uint8_t MCP_byteRead(MCP *mcp, uint8_t);               // Reads an individual register and returns the byte. Argument is the register address

void MCP_wordWrite(MCP *mcp, uint8_t, uint16_t);
uint16_t MCP_wordRead(MCP *mcp, uint8_t); 
uint16_t MCP_pinReadAll(MCP *mcp);


void MCP_maskWrite(MCP *mcp, uint16_t mask, uint8_t value);
void MCP_maskpullupMode(MCP *mcp, uint16_t mask, uint8_t value);
void MCP_maskpinMode(MCP *mcp, uint16_t mask, uint8_t mode);


#endif //MCP23S17
