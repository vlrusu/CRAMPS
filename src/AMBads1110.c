
#include "AMBads1110.h"

void _AMBads1110_init(AMBads1110_t *self, MI2C *mi2c, uint8_t address, uint16_t addressMask)
{

  self->_mi2c = mi2c;
  self->_i2caddress = address;
  self->_addrMask = addressMask;
  self->_flipmask = 0;

  uint8_t count = 0;
  for (int i = 0; i < 16; i++)
    if ((addressMask >> i & 0x1))
      count++;

  self->_nCramps = count;

  // FIXME
  //  ocbit not set 0x80 no effect on RDY bit continuous conversion mode
  self->config = 0x80 | (SAMPLERATE << 2) | GAIN; 
}

uint16_t _AMBads1110_setconfig(AMBads1110_t *self)
{

  MI2C_start(self->_mi2c, self->_addrMask);
  uint16_t ret = MI2C_write(self->_mi2c, self->_addrMask, self->_i2caddress << 1 | MI2C_WRITE);
  ret = ret & self->_addrMask;

  if (ret != MI2C_ACK)
  {
    return 1;
  }

  ret = MI2C_write(self->_mi2c, self->_addrMask, self->config);
  ret = ret & self->_addrMask;

  if (ret != MI2C_ACK)
  {
    return 2;
  }
  MI2C_stop(self->_mi2c, self->_addrMask);

  // printf("config complete for %.2x\n", self->_i2caddress);

  return 0;
}

uint16_t _AMBads1110_read(AMBads1110_t *self, float *dataByCramp, uint8_t *configrefByCramp)
{

  uint16_t firstByte[8];
  uint16_t secondByte[8];
  uint16_t thirdByte[8];

  // need to add the mask here

  uint8_t firstByteByCramp[16];
  uint8_t secondByteByCramp[16];
  uint8_t thirdByteByCramp[16];

  // printf("begin AMBads1110_read self->_nCramps=%d\n",self->_nCramps);
  uint16_t retc = 0;

  //  uint8_t AMBADS1110_ADDRESS = I2CADDRESS[channel];
  uint8_t AMBADS1110_ADDRESS = 0;

  // channel==0 ? AMBADS1110_ADDRESS_CH0 : AMBADS1110_ADDRESS_CH1;

  MI2C_start(self->_mi2c, self->_addrMask);
  retc = MI2C_write(self->_mi2c, self->_addrMask, self->_i2caddress << 1 | MI2C_READ);

  MI2C_read(self->_mi2c, 0, self->_addrMask, &firstByte);

  MI2C_read(self->_mi2c, 0, self->_addrMask, &secondByte);

  MI2C_read(self->_mi2c, 0, self->_addrMask, &thirdByte);

  MI2C_stop(self->_mi2c, self->_addrMask);

  // rearrange by cramp
  for (uint8_t i = 0; i < 8; i++)
  {
    uint16_t port1 = firstByte[i];
    uint16_t port2 = secondByte[i];
    uint16_t port3 = thirdByte[i];
    uint8_t *pFirstByteByCramp = (uint8_t *)firstByteByCramp;
    uint8_t *pSecondByteByCramp = (uint8_t *)secondByteByCramp;
    uint8_t *pThirdByteByCramp = (uint8_t *)thirdByteByCramp;
    for (uint8_t chn = 0; chn < self->_nCramps; chn++)
    {
      pFirstByteByCramp[0] <<= 1;
      pFirstByteByCramp[0] |= port1 & 1;
      port1 >>= 1;
      pSecondByteByCramp[0] <<= 1;
      pSecondByteByCramp[0] |= port2 & 1;
      port2 >>= 1;
      pThirdByteByCramp[0] <<= 1;
      pThirdByteByCramp[0] |= port3 & 1;
      port3 >>= 1;

      pFirstByteByCramp++;
      pSecondByteByCramp++;
      pThirdByteByCramp++;
    }
  }

  // this needs a lot of work
  /*    bool done = true;
     if (waitornot){
       for (uint8_t chn = 0; chn < self->_nCramps; chn++)
   {

     uint16_t rawdata = (firstByteByCramp[chn] << 8) + secondByteByCramp[chn];
     int16_t val = (int16_t)(rawdata);

     dataByCramp[chn] = AMBADS1110_REFV * (float)val / AMBADS1110_FS;

     configrefByCramp[chn] = thirdByteByCramp[chn];
     if ((configrefByCramp[chn] & 0x80) )

     } */

  for (uint8_t chn = 0; chn < self->_nCramps; chn++)
  {

    uint16_t rawdata = (firstByteByCramp[chn] << 8) + secondByteByCramp[chn];
    int16_t val = (int16_t)(rawdata);

    dataByCramp[chn] = AMBADS1110_REFV * (float)val / AMBADS1110_FS;

    configrefByCramp[chn] = thirdByteByCramp[chn];
  }

  return retc;
}
