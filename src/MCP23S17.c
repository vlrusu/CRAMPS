/*
  MCP23S17.cpp  Version 0.1
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

#include "MCP23S17.h" // Header files for this class
#include "hardware/spi.h"
#include "hardware/gpio.h"

// Defines to keep logical information symbolic go here

#define HIGH (1)
#define LOW (0)
#define ON (1)
#define OFF (0)

#define MCPDELAY 1

//#define SOFTSPI

#ifdef SOFTSPI
#define PIN_MISO 4

#define PIN_SCK 6
#define PIN_MOSI 3
#endif

// #define PIN_MISO 4
// #define PIN_SCK 6
// #define PIN_MOSI 7

// Here we have things for the SPI bus configuration

// Control byte and configuration register information - Control Byte: "0100 A2 A1 A0 R/W" -- W=0

#define OPCODEW (0b01000000) // Opcode for MCP23S17 with LSB (bit0) set to write (0), address OR'd in later, bits 1-3
#define OPCODER (0b01000001) // Opcode for MCP23S17 with LSB (bit0) set to read (1), address OR'd in later, bits 1-3

static const uint16_t spi_delay = 0;

extern const int PIN_CS;

static inline void delay()
{

  // asm volatile("nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop");

  for (int i = 0; i < 10; i++)
  {

    __asm volatile("nop\n");
  }

  
}

#ifdef SOFTSPI
void spi_write(uint8_t *buf, int len)
{

  for (int iw = 0; iw < len; iw++)
  {
    uint8_t data = buf[iw];
    for (int i = 7; i >= 0; i--)
    {
      gpio_put(PIN_MOSI, (data >> i) & 1);
      delay();
      gpio_put(PIN_SCK, 1); // Rising edge to clock data in
      delay();
      gpio_put(PIN_SCK, 0); // Falling edge
      delay();
    }
  }
}

void spi_read(uint8_t *rxbuf, int len)
{

  for (int j = 0; j < len; j++)
  {
    uint8_t data = 0;
    for (int i = 7; i >= 0; i--)
    {
      gpio_put(PIN_SCK, 1); // Rising edge to clock data out
      delay();
      data |= (gpio_get(PIN_MISO) << i);
      delay();
      gpio_put(PIN_SCK, 0); // Falling edge
      delay();
    }
    rxbuf[j] = data;
  }
}
#endif

static inline void cs_select()
{
  //  asm volatile("nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop");
  gpio_put(PIN_CS, 0); // Active low
  //  asm volatile("nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop");

  //  asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect()
{
  // asm volatile("nop \n nop \n nop");
  // asm volatile("nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop");
  gpio_put(PIN_CS, 1);
  //  asm volatile("nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop");
  //  asm volatile("nop \n nop \n nop");
}

void MCP_setup(MCP *mcp, uint8_t address)
{

  mcp->_address = address;

  MCP_byteWrite(mcp, IOCON, IOCON_INIT);
  //uint8_t ret = MCP_byteRead(mcp, IOCON);
  //printf("%.2x\n", ret);

  //        MCP_byteWrite(mcp, IOCON, 0);

  mcp->_modeCache = 0xFFFF;   // Default I/O mode is all input, 0xFFFF
  mcp->_outputCache = 0x0000; // Default output state is all off, 0x0000
  mcp->_pullupCache = 0x0000; // Default pull-up state is all off, 0x0000
  mcp->_invertCache = 0x0000; // Default input inversion state is not inverted, 0x0000

  MCP_wordWrite(mcp, IODIRA, 0x0);
  // MCP_wordWrite(mcp, GPIOA, 0xff);
  MCP_wordWrite(mcp, GPIOA, 0xbeef);

  // exit(1);
  //  mcp->_outputACache = MCP_byteRead(mcp, OLATA) ;
  //  mcp->_outputBCache = MCP_byteRead(mcp, OLATB) ;
};

void MCP_byteWrite(MCP *mcp, uint8_t reg, uint8_t value)
{ // Accept the register and byte

  // sleep_ms(MCPDELAY);
  delay();
  uint8_t tx_buf[3];
  tx_buf[0] = OPCODEW | (mcp->_address << 1);
  tx_buf[1] = reg;
  tx_buf[2] = value;

  cs_select();

#ifdef SOFTSPI
  spi_write(tx_buf, 3);
#else

  spi_write_blocking(spi0, &tx_buf, 3);
#endif
  cs_deselect();
}

void MCP_wordWrite(MCP *mcp, uint8_t reg, uint16_t value)
{ // Accept the start register and word

  // sleep_ms(MCPDELAY);
  delay();
  uint8_t tx_buf[4];
  tx_buf[0] = OPCODEW | (mcp->_address << 1);
  tx_buf[1] = reg;
  tx_buf[2] = value & 0xFF;
  tx_buf[3] = (value >> 8) & 0xFF;

  cs_select();

#ifdef SOFTSPI
  spi_write(tx_buf, 4);
#else
  spi_write_blocking(spi0, &tx_buf, 4);
#endif
  cs_deselect();
}

uint8_t MCP_byteRead(MCP *mcp, uint8_t reg)
{

  // sleep_ms(MCPDELAY);
  delay();
  uint8_t tx_buf[3];
  tx_buf[0] = OPCODER | (mcp->_address << 1);
  tx_buf[1] = reg;
  tx_buf[2] = 0;

  uint8_t rx_buf[3];

  cs_select();
  //   uint8_t n = spi_write_read_blocking(spi0, tx_buf, rx_buf, 3);

#ifdef SOFTSPI
  spi_write(tx_buf, 2);
  spi_read(rx_buf, 1);
#else

  spi_write_blocking(spi0, &tx_buf, 2);
  spi_read_blocking(spi0, 0, &rx_buf, 1);
#endif
  cs_deselect();

  //  uint8_t recv = rx_buf[2];
  uint8_t recv = rx_buf[0];

  //  printf("%d\n",n);
  //  printf("%2x %2x %2x\n",rx_buf[0],rx_buf[1],rx_buf[2]);
  //
  return recv;
}

uint16_t MCP_wordRead(MCP *mcp, uint8_t reg)
{
  //  sleep_ms(MCPDELAY);
  delay();
  uint16_t value = 0;
  uint8_t tx_buf[4];
  tx_buf[0] = OPCODER | (mcp->_address << 1);
  tx_buf[1] = reg;
  tx_buf[2] = 0;
  tx_buf[3] = 0;

  uint8_t rx_buf[sizeof tx_buf];

  cs_select();
  // uint8_t n = spi_write_read_blocking(spi0, tx_buf, rx_buf, 4);

#ifdef SOFTSPI
  spi_write(tx_buf, 2);
  spi_read(rx_buf, 2);
#else

  spi_write_blocking(spi0, &tx_buf, 2);
  spi_read_blocking(spi0, 0, &rx_buf, 2);
#endif
  cs_deselect();

  value |= rx_buf[0];
  value |= (rx_buf[1] << 8);

  // value |= rx_buf[2];
  // value |= (rx_buf[3] << 8);

  return value;
}

// MODE SETTING FUNCTIONS - BY PIN

void MCP_pinMode(MCP *mcp, uint8_t pin, uint8_t mode)
{ // Accept the pin # and I/O mode

  if (pin > 16)
  {
    printf("pin outta bounds\n");
    return;
  }
  if (mode == MCP_INPUT)
  {                                // Determine the mode before changing the bit state in the mode cache
    mcp->_modeCache |= 1 << (pin); // Since input = "HIGH", OR in a 1 in the appropriate place
  }
  else
  {
    mcp->_modeCache &= ~(1 << (pin)); // If not, the mode must be output, so and in a 0 in the appropriate place
  }
  MCP_wordWrite(mcp, IODIRA, mcp->_modeCache);
}

void MCP_maskpinMode(MCP *mcp, uint16_t mask, uint8_t mode)
{
  // idea here is that all devices off the MCP do the same thing

  if (mode == MCP_INPUT)
    mcp->_modeCache = (mcp->_modeCache & ~mask) | (0xffff & mask);
  else
    mcp->_modeCache = (mcp->_modeCache & ~mask) | (0 & mask);

  MCP_wordWrite(mcp, IODIRA, mcp->_modeCache);
}

// THE FOLLOWING WRITE FUNCTIONS ARE NEARLY IDENTICAL TO THE FIRST AND ARE NOT INDIVIDUALLY COMMENTED

// WEAK PULL-UP SETTING FUNCTIONS - BY WORD AND BY PIN

void MCP_pullupMode(MCP *mcp, uint8_t pin, uint8_t mode)
{

  if (pin > 16)
  {
    printf("pin outta bounds\n");
    return;
  }
  if (mode == ON)
  {
    mcp->_pullupCache |= 1 << (pin);
  }
  else
  {
    mcp->_pullupCache &= ~(1 << (pin));
  }
  MCP_wordWrite(mcp, GPPUA, mcp->_pullupCache);
}

void MCP_maskpullupMode(MCP *mcp, uint16_t mask, uint8_t mode)
{
  // idea here is that all devices off the MCP do the same thing

  if (mode == ON)
    mcp->_pullupCache = (mcp->_pullupCache & ~mask) | (0xffff & mask);
  else
    mcp->_pullupCache = (mcp->_pullupCache & ~mask) | (0 & mask);

  MCP_wordWrite(mcp, GPPUA, mcp->_pullupCache);
}

// WRITE FUNCTIONS

void MCP_pinWrite(MCP *mcp, uint8_t pin, uint8_t value)
{
  if (pin > 16)
  {
    printf("pin outta bounds\n");
    return;
  }

  if (value)
  {
    mcp->_outputCache |= 1 << (pin);
  }
  else
  {
    mcp->_outputCache &= ~(1 << (pin));
  }
  MCP_wordWrite(mcp, GPIOA, mcp->_outputCache);
}

void MCP_maskWrite(MCP *mcp, uint16_t mask, uint8_t value)
{
  // idea here is that all devices off the MCP do the same thing

  if (value)
    mcp->_outputCache = (mcp->_outputCache & ~mask) | (0xffff & mask);
  else
    mcp->_outputCache = (mcp->_outputCache & ~mask) | (0 & mask);

  MCP_wordWrite(mcp, GPIOA, mcp->_outputCache);
}

uint8_t MCP_pinRead(MCP *mcp, uint8_t pin)
{ // Return a single bit value, supply the necessary bit (1-16)
  if (pin > 16)
  {
    printf("pin outta bounds\n");
    return;
  }

  return MCP_wordRead(mcp, GPIOA) & (1 << (pin)) ? HIGH : LOW; // Call the word reading function, extract HIGH/LOW information from the requested pin
}

uint16_t MCP_pinReadAll(MCP *mcp)
{ // Return a single bit value, supply the necessary bit (1-16)

  return MCP_wordRead(mcp, GPIOA); // Call the word reading function, extract HIGH/LOW information from the requested pin
}
