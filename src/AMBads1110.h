#ifndef _AMBADS1110_SOFT_H
#define _AMBADS1110_SOFT_H

#include <stdint.h>
#include "MI2C.h"




//#define AMBADS1110_FS 8192
//#define AMBADS1110_FS 32768

//sample rates: 0 - 240SPS 1 60SPS 2 30SPS 3 15SPS
//FS changes accordingly: 2048 8192 16384 32768 
#define SAMPLERATE 1
#define GAIN 2

#define AMBADS1110_FS 8192
#define AMBADS1110_REFV 2.048
#define AMBADS1110_ADDRESS_CH0 0x4A  // -- 4A = ED2 -- 4D = ED5 
#define AMBADS1110_ADDRESS_CH1 0x4D  // -- 4A = ED2 -- 4D = ED5 
//#define AMBADS1110_ADDRESS 0x68  // -- mcp3426 

typedef struct {
  int config;
  MI2C*  _mi2c;
  uint8_t _i2caddress;
  uint16_t _addrMask;
  uint16_t _flipmask;
  uint8_t _nCramps;
  
} AMBads1110_t;


void _AMBads1110_init(AMBads1110_t* self, MI2C* i2c, uint8_t, uint16_t);
uint16_t _AMBads1110_setconfig(AMBads1110_t* self);
uint16_t _AMBads1110_read(AMBads1110_t* self, float*, uint8_t*);

#endif
