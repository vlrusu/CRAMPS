


#include "MI2C.h"
#include <unistd.h>

#define ALLPINS  0xffff

uint8_t MI2C_setup(MI2C *self, MCP *mcp_sda, MCP *mcp_scl, uint16_t sdaAll, uint8_t sclPin)
{
  self->_mcp_sda = mcp_sda;
  self->_mcp_scl = mcp_scl;
  self->_sclPin = sclPin;
  self->_sdaPinMaskAll = sdaAll;

  if (mcp_sda == mcp_scl && (1<<sclPin) & sdaAll)
    return 1;

  // printf("MI2C nCramps = %04x \n", count);
  MCP_maskpinMode(self->_mcp_sda, ALLPINS, MCP_OUTPUT);
  MCP_maskWrite(self->_mcp_sda, ALLPINS, 1);

  return 0;
  
}

uint16_t MI2C_scanbus(MI2C *self, uint16_t thisaddress)
{

//try each address
//  for (int iaddr = 0; iaddr<sizeof(I2CADDRESS); iaddr++){
//    uint8_t thisaddress = I2CADDRESS[iaddr];
  uint16_t ret = MI2C_start(self, self->_sdaPinMaskAll, thisaddress << 1 | MI2C_READ);
  printf("%.2x %.4x\n",thisaddress,ret);
//self->_sdaPinMask[iaddr] = ret;

  //do I need this?
  //  MI2C_write(self,  self->_sdaPinMaskAll, 0); 

  uint16_t tmp[8];
  MI2C_read(self,0,self->_sdaPinMaskAll,tmp);
  MI2C_read(self,0,self->_sdaPinMaskAll,tmp);
  
  MI2C_stop(self, self->_sdaPinMaskAll);

  return ret;
    //
    
    //  }

}

void MI2C_read(MI2C *self, uint8_t last, uint16_t mask, uint16_t *dataByByte)
{
  uint8_t b = 0;
  uint16_t tmp[16];
  MCP_maskWrite(self->_mcp_sda, mask, 1);
  MCP_maskpinMode(self->_mcp_sda, mask, MCP_INPUT);
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
  MCP_maskpinMode(self->_mcp_sda, mask, MCP_OUTPUT);
  MCP_maskWrite(self->_mcp_sda, mask, last);
  MCP_pinWrite(self->_mcp_scl, self->_sclPin, 1);
  //    usleep(MI2C_DELAY_USEC);

  MCP_pinWrite(self->_mcp_scl, self->_sclPin, 0);
  MCP_maskWrite(self->_mcp_sda, mask, 0);

  // reorder bits so that only what is masked shows out in the dataByByte array.
  // In other words, if there are holes in teh mask, those bits will bbe dropped
  
  for (int i = 0; i < 8; i++)
  {
    uint16_t r = 0x0;
    uint16_t count = 0;

    for (int j = 0; j < 16; j++)
      if ((mask >> j) & 1)
      {
        r |= ((tmp[i] >> j) & 0x1) << count;
        count++;
      }
    dataByByte[i] = r;
  }

}

uint8_t MI2C_start(MI2C *self, uint16_t mask,uint8_t addressRW)
{
  MCP_pinWrite(self->_mcp_scl, self->_sclPin, 1);
  MCP_maskWrite(self->_mcp_sda, mask, 1);
  MCP_maskWrite(self->_mcp_sda, mask, 0);
  MCP_pinWrite(self->_mcp_scl, self->_sclPin, 0);
  return MI2C_write(self, mask, addressRW);
}

void MI2C_stop(MI2C *self, uint16_t mask)
{
  MCP_maskWrite(self->_mcp_sda, mask, 0);
  MCP_pinWrite(self->_mcp_scl, self->_sclPin, 1);
  MCP_maskWrite(self->_mcp_sda, mask, 1);
}

uint16_t MI2C_write(MI2C *self, uint16_t mask, uint8_t data)
{
  // write byte
  uint8_t m;
  for (m = 0X80; m != 0; m >>= 1)
  {
    MCP_maskWrite(self->_mcp_sda, mask, m & data);
    MCP_pinWrite(self->_mcp_scl, self->_sclPin, 1);
    MCP_pinWrite(self->_mcp_scl, self->_sclPin, 0);
  }

  // get Ack or Nak
  MCP_maskpinMode(self->_mcp_sda, mask, MCP_INPUT);
  MCP_pinWrite(self->_mcp_scl, self->_sclPin, 1);
  uint16_t rack = MCP_pinReadAll(self->_mcp_sda) & mask;

  // reorder bits so that only what is masked shows out in the dataByByte array.
  // In other words, if there are holes in teh mask, those bits will bbe dropped

  /*
  uint16_t rtn = 0x0;
  uint16_t count = 0;

  for (int j = 0; j < 16; j++)
    if ((self->_sdaPinMaskAll >> j) & 1)
    {
      rtn |= ((r >> j) & 0x1) << count;
      count++;
    }
  */
  MCP_pinWrite(self->_mcp_scl, self->_sclPin, 0);
  MCP_maskpinMode(self->_mcp_sda, mask, MCP_OUTPUT);
  MCP_maskWrite(self->_mcp_sda, mask, 0);
  return rack;
}
