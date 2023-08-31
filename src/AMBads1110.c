
#include "AMBads1110.h"



void _AMBads1110_init(AMBads1110_t *self, MI2C *mi2c)
{

  self->_mi2c = mi2c;

  // ocbit not set 0x80 no effect on RDY bit continuous conversion mode
  uint8_t gain = 0;       // default x1
  uint8_t samplerate = 1; // 16SPS 16 bit
  uint8_t mode = 1;       // 1 - continuous conversion 0 - one-shot
  uint8_t channel = 0;    // obviously this will be changed by the code
  // self->config = 0x80;// | (channel << 5) | (mode << 4) | (samplerate << 2) | (gain) ;
  self->config = 0x84; // 4; //8; //4; //8; //0x8C;      //0 | (channel << 5) | (mode << 4) | (samplerate << 2) | (gain) ;
}

uint16_t _AMBads1110_setconfig(AMBads1110_t *self, int channel)
{

  uint8_t AMBADS1110_ADDRESS;

  uint16_t ret = MI2C_start(self->_mi2c, AMBADS1110_ADDRESS_CH0 << 1 | MI2C_WRITE);

  if (ret != MI2C_ACK)
  {
    return ret;
  }

  ret = MI2C_write(self->_mi2c, self->config);

  if (ret != MI2C_ACK)
  {
    return ret;
  }
  MI2C_stop(self->_mi2c);

  ret = MI2C_start(self->_mi2c, AMBADS1110_ADDRESS_CH1 << 1 | MI2C_WRITE);

  if (ret != MI2C_ACK)
  {
    return ret;
  }

  ret = MI2C_write(self->_mi2c, self->config);

  if (ret != MI2C_ACK)
  {
    return ret;
  }
  MI2C_stop(self->_mi2c);

  return 0;
}

uint16_t _AMBads1110_read(AMBads1110_t *self, float *dataByCramp, int channel)
{

  uint16_t firstByte[8];
  uint16_t secondByte[8];
  uint16_t thirdByte[8];
  uint8_t third0Byte[8];
  uint8_t third1Byte[8];
  uint8_t third2Byte[8];

  // need to add the mask here

  uint8_t firstByteByCramp[16];
  uint8_t secondByteByCramp[16];
  uint8_t thirdByteByCramp[16];

  uint8_t nCramps = self->_mi2c->_nCramps;

  // printf("begin AMBads1110_read nCramps=%d\n",nCramps);
  uint16_t retc = 0;

  uint8_t AMBADS1110_ADDRESS = channel==0 ? AMBADS1110_ADDRESS_CH0 : AMBADS1110_ADDRESS_CH1;
  

  retc = MI2C_start(self->_mi2c, AMBADS1110_ADDRESS << 1 | MI2C_READ);


  MI2C_read(self->_mi2c, 0, &firstByte);

  MI2C_read(self->_mi2c, 0, &secondByte);

  // rearrange by cramp
  for (uint8_t i = 0; i < 8; i++)
  {
    uint16_t port1 = firstByte[i];
    uint16_t port2 = secondByte[i];
    uint16_t port3 = thirdByte[i];
    uint8_t *pFirstByteByCramp = (uint8_t *)firstByteByCramp;
    uint8_t *pSecondByteByCramp = (uint8_t *)secondByteByCramp;
    uint8_t *pThirdByteByCramp = (uint8_t *)thirdByteByCramp;
    for (uint8_t chn = 0; chn < nCramps; chn++)
    {
      pFirstByteByCramp[0] <<= 1;
      pFirstByteByCramp[0] |= port1 & 1;
      port1 >>= 1;
      pSecondByteByCramp[0] <<= 1;
      pSecondByteByCramp[0] |= port2 & 1;
      port2 >>= 1;

      pFirstByteByCramp++;
      pSecondByteByCramp++;
    }
  }

  for (uint8_t chn = 0; chn < nCramps; chn++)
  {

    uint16_t rawdata = (firstByteByCramp[chn] << 8) + secondByteByCramp[chn];
    int16_t val = (int16_t)(rawdata);

    dataByCramp[chn] = AMBADS1110_REFV * (float)val / AMBADS1110_FS;
  }

  
  return retc;
}
