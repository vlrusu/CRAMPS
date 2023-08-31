#ifndef _AMBADS1110_SOFT_H
#define _AMBADS1110_SOFT_H

#include <stdint.h>
#include "MI2C.h"

//#define AMBADS1110_FS 8192
//#define AMBADS1110_FS 32768
#define AMBADS1110_FS 8192 //16384
#define AMBADS1110_REFV 2.048
#define AMBADS1110_ADDRESS_CH0 0x4A  // -- 4A = ED2 -- 4D = ED5 
#define AMBADS1110_ADDRESS_CH1 0x4D  // -- 4A = ED2 -- 4D = ED5 
//#define AMBADS1110_ADDRESS 0x68  // -- mcp3426 

typedef struct {
  int config;
  MI2C*  _mi2c;
  
} AMBads1110_t;


void _AMBads1110_init(AMBads1110_t* self, MI2C* i2c);
uint16_t _AMBads1110_setconfig(AMBads1110_t* self, int channel);
uint16_t _AMBads1110_read(AMBads1110_t* self, float*, int channel);

#endif
