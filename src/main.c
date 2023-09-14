#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/types.h"
#include "pico/platform.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "hardware/sync.h"
#include "hardware/adc.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#include <time.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"

#include "MCP23S17.h"
#include "AMBads1110.h"

#include "utils.h"

#define MCPPINBASE 100

#define NSTEPS 200
// #define SPISPEED 320000

const int PIN_CS = 5;

#define DEVICEID 0xA1

#define POWERPIN 0
#define PIN_MISO 4

#define PIN_SCK 6
#define PIN_MOSI 3

#define NADDR 3


#define POWERUP 'P'
#define RESET 'R'
#define POWEROFF 'O'
#define GETDEVICEID 'D'
#define SCAN 'S'
#define TESTMCP 'T'
#define TESTREADMCP 'U'
#define INITIALIZE 'I'
#define ACQUIRE 'A'
#define POWERCYCLE 'C'


// batch 0 has 0x4A at top and 0x4D at bottom
// batch 1 has 0x4A at top and 0x48 at bottom
const uint8_t I2CADDRESS[NADDR] = {0x48, 0x4A, 0x4D};
// const uint8_t I2CADDRESS[NADDR] = {0x48, 0x4B, 0x4D};

enum MCPs
{
  MCPHV0,
  MCPHV1,
  MCPHV2,
  MCPHV3
};
MCP crampMCP[4];
MI2C mi2c_cramps[4];

#define MAX_LINE_LENGTH 24

AMBads1110_t adc[4 * NADDR];

int SlotCounter = 0;
int enabledSlots[MAX_LINE_LENGTH];

uint16_t crampMask[4][NADDR];

int zNumbers[4][NADDR][16]; // there are maximum 14 cramps on any given MCP
int crampList[2*48];

clock_t clock()
{
  return (clock_t)time_us_64() / 10000;
}

static inline void delay()
{

  asm volatile("nop \n nop \n nop \n nop \n nop");
  asm volatile("nop \n nop \n nop \n nop \n nop");

  asm volatile("nop \n nop \n nop \n nop \n nop");

  for (int i = 0; i < 40; i++)
  {

    __asm volatile("nop\n");
  }
}

static inline void delay2()
{

  // asm volatile("nop \n nop \n nop \n nop \n nop");

  for (int i = 0; i < 10; i++)
  {

    __asm volatile("nop\n");
  }
}

void recover()
{

  gpio_put(POWERPIN, 0);

  sleep_ms(1000);

  gpio_put(POWERPIN, 1);
}

void initialization()
{

  // printf("begin\n");

  // setup MCPs for cramps
  for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
  {

    MCP_setup(&crampMCP[imcp], imcp);
  }

  // initialize cramps
  /* for (int i = 0; i < SlotCounter; i++) */
  /*   { */

  /*     printf("Enabling slot %d\n", enabledSlots[i]); */

  /*     if (enabledSlots[i] >= 0 && enabledSlots[i] < 14) */
  /* 	crampMasks[0] |= (1 << (enabledSlots[i] + 2)); */
  /*     if (enabledSlots[i] >= 14 && enabledSlots[i] < 24) */
  /* 	crampMasks[1] |= (1 << (enabledSlots[i] - 14)); */
  /*     if (enabledSlots[i] >= 24 && enabledSlots[i] < 38) */
  /* 	crampMasks[2] |= (1 << (enabledSlots[i] - 24 + 2)); */
  /*     if (enabledSlots[i] >= 38 && enabledSlots[i] < 48) */
  /* 	crampMasks[3] |= (1 << (enabledSlots[i] - 38)); */
  /*   } */

  uint8_t retc = 0;
  retc = MI2C_setup(&mi2c_cramps[0], &crampMCP[MCPHV0], &crampMCP[MCPHV0], 0xfffc, 1); // SDA SCL - this is new
  printf("%d ", retc);
  retc = MI2C_setup(&mi2c_cramps[1], &crampMCP[MCPHV1], &crampMCP[MCPHV0], 0x3ff, 1); // SDA SCL - this is new
  printf("%d ", retc);
  retc = MI2C_setup(&mi2c_cramps[2], &crampMCP[MCPHV2], &crampMCP[MCPHV2], 0xfffc, 1); // SDA SCL - this is new
  printf("%d ", retc);
  retc = MI2C_setup(&mi2c_cramps[3], &crampMCP[MCPHV3], &crampMCP[MCPHV2], 0x3ff, 1); // SDA SCL - this is new
  printf("%d\n", retc);

  int ncr = scan();
  printf("Number of cramps: %d\n", ncr / 2);
  for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
  {
    for (int iaddr = 0; iaddr < NADDR; iaddr++)
    {
      if (crampMask[imcp][iaddr] != 0)
      {

        uint8_t adcindex = NADDR * imcp + iaddr;
        _AMBads1110_init(&adc[adcindex], &mi2c_cramps[imcp], I2CADDRESS[iaddr], crampMask[imcp][iaddr]);
        uint16_t ret = _AMBads1110_setconfig(&adc[adcindex]);
        printf("%d ", ret);

        uint8_t tmpconfigs[24];
        float tmpcurrents[24];
        uint8_t ncramps = adc[adcindex]._nCramps;
        _AMBads1110_read(&adc[adcindex], &tmpcurrents, &tmpconfigs);

        uint16_t toremove = 0;
        for (int ich = 0; ich < ncramps; ich++)
        {
          //             printf("%d %d %7.5f\n", zNumbers[imcp][iaddr][ich], iaddr==1?0:1, tmpcurrents[ich]);
          printf("TEST1: %d %d %d %.2x \n", imcp, iaddr, ich, tmpconfigs[ich]);

          if ((tmpconfigs[ich] & adc[adcindex].config & 0x7f) != (adc[adcindex].config & 0x7f)) // ignore the ready bit
          {
            toremove |= (1 << ich);
          }
        }
        if (toremove > 0)
        {
          // find the mask to flip
          uint16_t flipmask = 0;

          //     printf("TEST: %.4x %.4x\n", toremove, adc[adcindex]._addrMask);
          int removing = countSetBits(toremove);
          for (int i = 0; i < removing; i++)
          {

            int ibit = findNthSetBit(toremove, i + 1);
            int ibitmask = findNthSetBit(adc[adcindex]._addrMask, ibit + 1);
            if (ibit != -1)
            {
              flipmask |= (1 << ibitmask);
            }
            else
              printf("Bit removal failed, this is so baaaaad... %d %.4x\n", i, adc[adcindex]._addrMask);

            printf("\nCramp defective or AMB defective in slot, removing %d %d %d %.4x\n", imcp, iaddr, i + 1, adc[adcindex]._addrMask);
            adc[adcindex]._nCramps--;
          }
          printf("TEST2: %.4x %.4x\n", adc[adcindex]._addrMask, flipmask);
          adc[adcindex]._addrMask = adc[adcindex]._addrMask & (~flipmask);
          printf("TEST3: %.4x %.4x\n", adc[adcindex]._addrMask, flipmask);
          adc[adcindex]._flipmask = flipmask;
        }
      }
      printf("\n");
    }
  }

  zNumberMap();

  // final check

  for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
  {
    for (int iaddr = 0; iaddr < NADDR; iaddr++)
    {

      uint8_t adcindex = NADDR * imcp + iaddr;

      if (adc[adcindex]._addrMask != 0)
      {

        //       _AMBads1110_init(&adc[adcindex], &mi2c_cramps[imcp], I2CADDRESS[iaddr], crampMask[imcp][iaddr]);
        uint16_t ret = _AMBads1110_setconfig(&adc[adcindex]);

        uint8_t tmpconfigs[24];
        float tmpcurrents[24];
        uint8_t ncramps = adc[adcindex]._nCramps;
        _AMBads1110_read(&adc[adcindex], &tmpcurrents, &tmpconfigs);

        uint16_t toremove = 0;
        for (int ich = 0; ich < ncramps; ich++)
        {
          printf("%d %d %7.5f\n", zNumbers[imcp][iaddr][ich], iaddr == 1 ? 0 : 1, tmpcurrents[ich]);

          if ((tmpconfigs[ich] & adc[adcindex].config & 0x7f) != (adc[adcindex].config & 0x7f))
          { // ignore the ready bit
            printf("WTF %d %d %d %.4x %.2x %d\n", imcp, iaddr, ich, adc[adcindex]._addrMask, tmpconfigs[ich], findNthSetBit(adc[adcindex]._flipmask, ich + 1));

            // try again:
            //  _AMBads1110_read(&adc[adcindex], &tmpcurrents, &tmpconfigs);
            //  printf("WTF2 %d %d %d %.4x %.2x\n",  imcp, iaddr, adc[adcindex]._addrMask,tmpconfigs[ich]);
          }
        }
      }
    }
  }

  printf("Initialization complete\n");
}

int scan()
{

  int ncramps = 0;
  for (int iaddr = 0; iaddr < NADDR; iaddr++)
  {
    for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
    {
      uint16_t ret = MI2C_scanbus(&mi2c_cramps[imcp], I2CADDRESS[iaddr]);
      crampMask[imcp][iaddr] = ret;
      ncramps += countSetBits(ret);
    }
  }

  return ncramps;
}

int zNumberMap()
{

  int mcpborder = 0;

  for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
  {

    for (int iaddr = 0; iaddr < NADDR; iaddr++)
    {

      if (crampMask[imcp][iaddr] == 0)
        continue;
      uint8_t adcindex = NADDR * imcp + iaddr;
      uint8_t ncramps = adc[adcindex]._nCramps;

      uint8_t count = 0;

      for (int ibit = 0; ibit < 16; ibit++)
      {

        if ((adc[adcindex]._addrMask & (1 << ibit)) == 0)
          continue;
        zNumbers[imcp][iaddr][count] = mcpborder + ((imcp % 2 == 1) ? ibit : ibit - 2); // this is so janky... Relies on the actual mask
        count++;
      }
    }

    mcpborder += countSetBits(mi2c_cramps[imcp]._sdaPinMaskAll);
  }
}

// #define SOFTSPI

int main(int argc, char *argv[])
{

  // readInput();

  stdio_init_all(); // needed for redirect stdin/stdout to Pico's USB or UART ports

  sleep_ms(1000);

  // gpio_init_mask(0xffff);
  // gpio_put_all(1);

  gpio_init(POWERPIN);
  gpio_set_dir(POWERPIN, GPIO_OUT);
  gpio_put(POWERPIN, 0);

#ifndef SOFTSPI
  spi_init(spi0, 5000000);
  spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
#else
  gpio_init(PIN_SCK);
  gpio_set_dir(PIN_SCK, GPIO_OUT);
  gpio_put(PIN_SCK, 0);
  gpio_init(PIN_MOSI);
  gpio_set_dir(PIN_MOSI, GPIO_OUT);
  gpio_set_pulls(PIN_MOSI, true, false);
  gpio_put(PIN_MOSI, 0);
  gpio_init(PIN_MISO);
  gpio_set_dir(PIN_MISO, GPIO_IN);
#endif
  gpio_set_drive_strength(PIN_SCK, GPIO_DRIVE_STRENGTH_12MA);
  gpio_set_drive_strength(PIN_MOSI, GPIO_DRIVE_STRENGTH_12MA);

  //  gpio_set_function(PIN_CS, GPIO_FUNC_SPI);
  // Make the SPI pins available to picotool
  // bi_decl(bi_3pins_with_func(PIN_MISO, PIN_MOSI, PIN_SCK, GPIO_FUNC_SPI));

  // Chip select is active-low, so we'll initialise it to a driven-high state
  gpio_init(PIN_CS);
  gpio_set_dir(PIN_CS, GPIO_OUT);
  gpio_put(PIN_CS, 1);
  // Make the CS pin available to picotool
  // bi_decl(bi_1pin_with_name(PIN_CS, "SPI CS"));

  bool startACQ = 0;
  clock_t startAcqTime = clock();

  sleep_ms(1000);

  int countloop = 0;

  //  MCP_setup(&crampMCP[0], 0);

  clock_t startTime = clock();
  //  initialization();
  while (1)

  {

    int input = getchar_timeout_us(10);
    if (input == RESET)
    {
      printf("Resetting\n");
      startACQ = 0;
      SlotCounter = 0;
    }

    else if (input == POWERUP)
    {
      printf("Powering up\n");
      gpio_put(POWERPIN, 1);
    }

    else if (input == POWERCYCLE)
    {
      printf("Pwer cycling\n");
      recover();
    }
    else if (input == POWEROFF)
    {
      printf("Powering off\n");
      gpio_put(POWERPIN, 0);
    }

    else if (input == GETDEVICEID)
    {
      printf("%.2x\n", DEVICEID);
    }
    else if (input == SCAN)
    {
      printf("Scanning\n");
      //	  scan();
      int Slot[48][2];
      memset(Slot, -1, 48 * 2 * sizeof(int));
      int finalncramps = 0;
      for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
      {

        for (int iaddr = 0; iaddr < NADDR; iaddr++)
        {
          if (crampMask[imcp][iaddr] == 0)
            continue;
          uint8_t adcindex = NADDR * imcp + iaddr;
          int ncramps = adc[adcindex]._nCramps;
          for (int ich = 0; ich < ncramps; ich++)
          {

            finalncramps++;
            // this assumes address 2 (0x4A, second in the array) is at bottom
            if (iaddr == 1)
              Slot[zNumbers[imcp][iaddr][ich]][1] = I2CADDRESS[iaddr];
            else
              Slot[zNumbers[imcp][iaddr][ich]][0] = I2CADDRESS[iaddr];
          }
        }
      }

      for (int i = 0; i < 48; i++)
      {
        if (Slot[i][0] >= 0)
          printf("Slot %d has a bottom address at %.2x (%d)\n", i, Slot[i][0], (Slot[i][0] & 0x7));
        if (Slot[i][1] >= 0)
          printf("Slot %d has a top address at %.2x (%d)\n", i, Slot[i][1], (Slot[i][1] & 0x7));
      }
      printf("%d cramp channels present and functional\n", finalncramps);
      printf("Scanning complete\n");
    }

    else if (input == TESTMCP)
    {
      printf("Testing MCPs\n");

      for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
      {

        MCP_setup(&crampMCP[imcp], imcp);
      }

      for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
      {

        uint16_t test = MCP_wordRead(&crampMCP[imcp], GPIOA);
        //	uint16_t test = MCP_byteRead(&crampMCP[imcp], IOCON);
        printf("%04x ", test);
      }

      printf("\n");
    }

    else if (input == TESTREADMCP)
    {
      printf("Testing MCPs\n");

      for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
      {

        uint16_t test = MCP_wordRead(&crampMCP[imcp], GPIOA);
        //	uint16_t test = MCP_byteRead(&crampMCP[imcp], IOCON);
        printf("%04x ", test);
      }

      printf("\n");
    }

    else if (input == INITIALIZE)
    {

      initialization(); // channelnumber);
    }

    else if (input == ACQUIRE)
    {

      startACQ = 1;
      startAcqTime = clock();
      memset(crampList, -1, 2*48 * sizeof(int));
      int reorderedCrampList[2*48];
      int crampCount = 0;
      printf("LOGGING: Starting acquisition ");
      for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
      {
        for (int iaddr = 0; iaddr < NADDR; iaddr++)
        {
          if (crampMask[imcp][iaddr] != 0)
          {
            uint8_t adcindex = NADDR * imcp + iaddr;
            int ncramps = adc[adcindex]._nCramps;

            for (int ich = 0; ich < ncramps; ich++)
            {
              //printf("%d_%d ", zNumbers[imcp][iaddr][ich], iaddr == 1 ? 1 : 0);
              // printf(" %7.5f ", tmpcurrents[ich]);
              crampList[crampCount] = 2*zNumbers[imcp][iaddr][ich] + (iaddr == 1 ? 1 : 0);
	      printf(" %d ",crampList[crampCount]);
              crampCount++;
            }
            //           sleep_ms(1000);
          }
        }
      }
      printf("\n");
      
      memcpy(reorderedCrampList, crampList, sizeof(int)*crampCount );
      if (crampCount>0) {
      	reorderArrays(reorderedCrampList, crampList, crampCount);
      }


      for (int i = 0 ; i < crampCount; i++){
      	printf(" %d ",reorderedCrampList[i]);
	
      }
      printf("\n");      
      
    }

    if (startACQ)
    {

      // for (int i = 0; i < 2000; i++)
      // {
      //   gpio_put(PIN_MOSI, 1);
      //   //gpio_put(PIN_SCK, 1);
      //   //delay();
      //   //gpio_put(PIN_SCK, 1);
      //   delay();
      //   gpio_put(PIN_MOSI, 0);
      //   delay();

      // }

      int n = 0;

      countloop++;

      uint8_t ncramps = 0;
      clock_t curTime = clock();
      double thisTime = (double)(curTime - startAcqTime) / CLOCKS_PER_SEC;
      printf("%.8f ", thisTime);
      float finalcurrents[48];
      uint32_t finalconfigs[48];
      int currentArrayIndex = 0;
      int index = 0;
      memset(finalcurrents, -1, 48 * sizeof(float));
      memset(finalconfigs, 0, 48 * sizeof(uint32_t));
      for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
      {
        for (int iaddr = 0; iaddr < NADDR; iaddr++)
        {
          if (crampMask[imcp][iaddr] != 0)
          {
            float tmpcurrents[24];
            uint8_t adcindex = NADDR * imcp + iaddr;
            ncramps = adc[adcindex]._nCramps;
	    uint32_t tmpconfigs[24];
	    //	    uint8_t *tmpconfigs = (uint8_t *)malloc(ncramps * sizeof(uint8_t));
            _AMBads1110_read(&adc[adcindex], &tmpcurrents, &tmpconfigs);
            memcpy(finalcurrents+currentArrayIndex, tmpcurrents, sizeof(float)*ncramps );
	    //            memcpy(finalconfigs+currentArrayIndex, tmpconfigs, sizeof(uint32_t)*ncramps);	    
            currentArrayIndex += ncramps;
	    //	    index+= sizeof(tmpconfigs)/sizeof(uint8_t);
	    //	    free(tmpconfigs);
            // for (int ich = 0; ich < ncramps; ich++)
            // {
            //   //             printf("%d %d %7.5f\n", zNumbers[imcp][iaddr][ich], iaddr==1?0:1, tmpcurrents[ich]);
            //   printf(" %7.5f ", tmpcurrents[ich]);
            // }
            //           sleep_ms(1000);
          }
        }
      }

      reorderArrays(finalcurrents, crampList, currentArrayIndex);
      //      reorderArrays(finalconfigs, crampList, currentArrayIndex);
      for (int i = 0 ; i < currentArrayIndex; i++)
	//	printf (" %7.5f %.2x ",1e2*finalcurrents[i] , finalconfigs) ;
		printf (" %7.5f ",1e2*finalcurrents[i] ) ;
      
      printf("\n");
      //     /*
      //           if (countloop % 100 == 0)
      //           {
      //             clock_t endTime = clock();
      //             double executionTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
      //             printf("%.8f sec\n", executionTime);
      //             startTime = clock();
      //           }
      //           */
    }
  }

  return 0;
}
