


#include "MI2C.h"
#include <unistd.h>

void MI2C_setup(MI2C *self, MCP *mcp_sda, uint16_t sdaPinMask, MCP *mcp_scl, uint8_t sclPin)
{
  self->_mcp_sda = mcp_sda;
  self->_sdaPinMask = sdaPinMask;
  self->_mcp_scl = mcp_scl;
  self->_sclPin = sclPin;

  uint8_t count = 0;
  for (int i = 0; i < 16; i++)
    if ((sdaPinMask >> i & 0x1))
      count++;

  self->_nCramps = count;
  // printf("MI2C nCramps = %04x \n", count);
  MCP_maskpinMode(self->_mcp_sda, self->_sdaPinMask, MCP_OUTPUT);
  MCP_maskWrite(self->_mcp_sda, self->_sdaPinMask, 1);

  MCP_pinMode(self->_mcp_scl, self->_sclPin, MCP_OUTPUT);
  MCP_pinWrite(self->_mcp_scl, self->_sclPin, 1);
}

void MI2C_read(MI2C *self, uint8_t last, uint16_t *dataByByte)
{
  uint8_t b = 0;
  uint16_t tmp[16];
  MCP_maskWrite(self->_mcp_sda, self->_sdaPinMask, 1);
  MCP_maskpinMode(self->_mcp_sda, self->_sdaPinMask, MCP_INPUT);
  // read byte
  uint8_t i;
  for (i = 0; i < 8; i++)
  {
    //       b <<= 1;
    //        usleep(MI2C_DELAY_USEC);
    MCP_pinWrite(self->_mcp_scl, self->_sclPin, 1);
    tmp[i] = MCP_pinReadAll(self->_mcp_sda);
    // printf("MI2C tmp read =%x \n",tmp[i]);
    MCP_pinWrite(self->_mcp_scl, self->_sclPin, 0);
  }
  // send Ack or Nak
  MCP_maskpinMode(self->_mcp_sda, self->_sdaPinMask, MCP_OUTPUT);
  MCP_maskWrite(self->_mcp_sda, self->_sdaPinMask, last);
  MCP_pinWrite(self->_mcp_scl, self->_sclPin, 1);
  //    usleep(MI2C_DELAY_USEC);

  MCP_pinWrite(self->_mcp_scl, self->_sclPin, 0);
  MCP_maskWrite(self->_mcp_sda, self->_sdaPinMask, 0);

  // reorder bits so that only what is masked shows out in the dataByByte array.
  // In other words, if there are holes in teh mask, those bits will bbe dropped
  for (int i = 0; i < 8; i++)
  {
    uint16_t r = 0x0;
    uint16_t count = 0;

    for (int j = 0; j < 16; j++)
      if ((self->_sdaPinMask >> j) & 1)
      {
        r |= ((tmp[i] >> j) & 0x1) << count;
        count++;
      }
    dataByByte[i] = r;
  }
}

uint8_t MI2C_start(MI2C *self, uint8_t addressRW)
{
  MCP_pinWrite(self->_mcp_scl, self->_sclPin, 1);
  MCP_maskWrite(self->_mcp_sda, self->_sdaPinMask, 1);
  MCP_maskWrite(self->_mcp_sda, self->_sdaPinMask, 0);
  MCP_pinWrite(self->_mcp_scl, self->_sclPin, 0);
  return MI2C_write(self, addressRW);
}

void MI2C_stop(MI2C *self)
{
  MCP_maskWrite(self->_mcp_sda, self->_sdaPinMask, 0);
  MCP_pinWrite(self->_mcp_scl, self->_sclPin, 1);
  MCP_maskWrite(self->_mcp_sda, self->_sdaPinMask, 1);
}

uint8_t MI2C_write(MI2C *self, uint8_t data)
{
  // write byte
  uint8_t m;
  for (m = 0X80; m != 0; m >>= 1)
  {
    MCP_maskWrite(self->_mcp_sda, self->_sdaPinMask, m & data);
    MCP_pinWrite(self->_mcp_scl, self->_sclPin, 1);
    MCP_pinWrite(self->_mcp_scl, self->_sclPin, 0);
  }

  // get Ack or Nak
  MCP_maskpinMode(self->_mcp_sda, self->_sdaPinMask, MCP_INPUT);
  MCP_pinWrite(self->_mcp_scl, self->_sclPin, 1);
  uint16_t r = MCP_pinReadAll(self->_mcp_sda);

  // reorder bits so that only what is masked shows out in the dataByByte array.
  // In other words, if there are holes in teh mask, those bits will bbe dropped
  uint16_t rtn = 0x0;
  uint16_t count = 0;

  for (int j = 0; j < 16; j++)
    if ((self->_sdaPinMask >> j) & 1)
    {
      rtn |= ((r >> j) & 0x1) << count;
      count++;
    }

  MCP_pinWrite(self->_mcp_scl, self->_sclPin, 0);
  MCP_maskpinMode(self->_mcp_sda, self->_sdaPinMask, MCP_OUTPUT);
  MCP_maskWrite(self->_mcp_sda, self->_sdaPinMask, 0);
  return rtn;
}
