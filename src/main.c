#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// #include <cstdlib>          // need this include, if you need heap management (malloc, realloc, free)
#include <pico/stdlib.h> // needed for Pico SDK support

#include "hardware/gpio.h"
#include "hardware/spi.h"

#include "MCP23S17.h"
#include "AMBads1110.h"

#include "utils.h"

#define MCPPINBASE 100

#define NSTEPS 200
// #define SPISPEED 320000

const int PIN_CS = 5;

#define DEVICEID 0xA0

#define POWERPIN 0
#define PIN_MISO 4

#define PIN_SCK 6
#define PIN_MOSI 7

#define NADDR 3

// the assumption is that the cramps have address 2 (0x4A) at bottom and only 3 addresses are in use
const uint8_t I2CADDRESS[NADDR] = {0x48, 0x4A, 0x4D};

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

clock_t clock()
{
  return (clock_t)time_us_64() / 10000;
}

void recover1()
{

  gpio_put(POWERPIN, 0);

  sleep_ms(1000);

  gpio_put(POWERPIN, 1);
}

void initialization()
{

  printf("begin\n");

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
  printf("%d\n", retc);
  retc = MI2C_setup(&mi2c_cramps[1], &crampMCP[MCPHV1], &crampMCP[MCPHV0], 0x3ff, 1); // SDA SCL - this is new
  printf("%d\n", retc);
  retc = MI2C_setup(&mi2c_cramps[2], &crampMCP[MCPHV2], &crampMCP[MCPHV2], 0xfffc, 1); // SDA SCL - this is new
  printf("%d\n", retc);
  retc = MI2C_setup(&mi2c_cramps[3], &crampMCP[MCPHV3], &crampMCP[MCPHV2], 0x3ff, 1); // SDA SCL - this is new
  printf("%d\n", retc);

  scan();

  for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
  {
    for (int iaddr = 0; iaddr < NADDR; iaddr++)
    {
      if (crampMask[imcp][iaddr] != 0)
      {
        _AMBads1110_init(&adc[NADDR * imcp + iaddr], &mi2c_cramps[imcp], I2CADDRESS[iaddr], crampMask[imcp][iaddr]);
        uint16_t ret = _AMBads1110_setconfig(&adc[NADDR * imcp + iaddr]);
      }
    }
  }

  zNumberMap();

  /*

  for (int i = 0; i < 4; i++)
    {

    }

  for (int i = 0; i < 4; i++)
    {
      _AMBads1110_setconfig(&adc[i], 0); // crampchannel);
    }
  for (int i = 0; i < 4; i++)
    {
      _AMBads1110_setconfig(&adc[i], 1); // crampchannel);
    }
  */
  printf("Initialization complete\n");
}

void scan()
{

  for (int iaddr = 0; iaddr < NADDR; iaddr++)
  {
    for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
    {
      uint16_t ret = MI2C_scanbus(&mi2c_cramps[imcp], I2CADDRESS[iaddr]);
      crampMask[imcp][iaddr] = ret;
    }
  }
}

int countSetBits(uint16_t mask)
{
  int count = 0;

  while (mask)
  {
    count += mask & 1;
    mask >>= 1;
  }

  return count;
}

int zNumberMap()
{

  int mcpborder = 0;

  for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
  {
    for (int iaddr = 0; iaddr < NADDR; iaddr++)
    {

      uint8_t adcindex = NADDR * imcp + iaddr;
      uint8_t ncramps = adc[adcindex]._nCramps;

      uint8_t count = 0;

      for (int ibit = 0; ibit < 16; ibit++)
      {
        if ((adc[adcindex]._addrMask & (1 << i)) == 0)
          continue;
        zNumbers[imcp][iaddr][count] = mcpborder + ibit;
        count++;
      }
    }

    mcpborder += countSetBits(mi2c_cramps[imcp]._sdaPinMaskAll)
  }
}

int main(int argc, char *argv[])
{

  // readInput();

  stdio_init_all(); // needed for redirect stdin/stdout to Pico's USB or UART ports

  sleep_ms(1000);

  spi_init(spi0, 10000000);
  spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

  gpio_init(POWERPIN);
  gpio_set_dir(POWERPIN, GPIO_OUT);
  gpio_put(POWERPIN, 0);

  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
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

  sleep_ms(1000);

  int countloop = 0;

  //  MCP_setup(&crampMCP[0], 0);

  clock_t startTime = clock();
  //  initialization();
  while (1)
  {

    int input = getchar_timeout_us(1);
    if (input == 'R')
    {
      printf("Resetting\n");
      startACQ = 0;
      SlotCounter = 0;
    }

    else if (input == 'P')
    {
      printf("Powering in\n");
      gpio_put(POWERPIN, 1);
    }
    else if (input == 'O')
    {
      printf("Powering off\n");
      gpio_put(POWERPIN, 0);
    }

     else if (input == 'O')
    {
      printf("%.2x\n",DEVICEID);
    }
    else if (input == 'S')
    {
      printf("Scanning\n");
      //	  scan();
      int Slot[48][2];
      memset(Slot, -1, 48 * 2 * sizeof(int));
      for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
      {

        for (int iaddr = 0; iaddr < NADDR; iaddr++)
        {
          uint8_t adcindex = NADDR * imcp + iaddr;
          ncramps = adc[adcindex]._nCramps;
          for (int ich = 0; ich < ncramps; ich++)
          {

            // this assumes address 2 (0x4A, second in the array) is at bottom
            if (iaddr == 1)
              Slot[zNumbers[imcp][iaddr][ich]][0] = I2CADDRESS[iaddr];
            else
              Slot[zNumbers[imcp][iaddr][ich]][1] = I2CADDRESS[iaddr];
          }
        }
      }

      for (int i = 0; i < 48; i++)
      {
        if (Slot[i][0] >= 0)
          printf("Slot %d has a bottom address at %.2x \n", i, Slot[i][0]);
        if (Slot[i][1] >= 0)
          printf("Slot %d has a top address at %.2x \n", i, Slot[i][1]);
      }
    }

    else if (input == 'T')
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

    else if (input == 'I')
    {

      initialization(); // channelnumber);
    }

    else if (input == 'A')
      startACQ = 1;

    if (startACQ)
    {
      int n = 0;

      countloop++;

      uint8_t ncramps = 0;

      for (int imcp = MCPHV0; imcp <= MCPHV3; imcp++)
      {
        for (int iaddr = 0; iaddr < NADDR; iaddr++)
        {
          if (crampMask[imcp][iaddr] != 0)
          {
            float tmpcurrents[24];
            uint8_t adcindex = NADDR * imcp + iaddr;
            ncramps = adc[adcindex]._nCramps;
            _AMBads1110_read(&adc[adcindex], &tmpcurrents);

            for (int ich = 0; ich < ncramps; ich++)
            {
              printf("%d %d %7.5f\n", zNumbers[imcp][iaddr][ich], iaddr==1?0:1, tmpcurrents[ich]);
            }
          }
        }
      }

      
      if (countloop % 100 == 0)
      {
        clock_t endTime = clock();
        double executionTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
        printf("%.8f sec\n", executionTime);
        startTime = clock();
      }
    }
  }

  return 0;
}
