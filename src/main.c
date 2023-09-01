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

#define POWERPIN 0
#define PIN_MISO 4

#define PIN_SCK 6
#define PIN_MOSI 7

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

AMBads1110_t adc[4];

int SlotCounter = 0;
int enabledSlots[MAX_LINE_LENGTH];


clock_t clock()
{
    return (clock_t) time_us_64() / 10000;
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

  uint16_t crampMasks[4] = {0, 0, 0, 0};
  // initialize cramps
  for (int i = 0; i < SlotCounter; i++)
    {

      printf("Enabling slot %d\n", enabledSlots[i]);

      if (enabledSlots[i] >= 0 && enabledSlots[i] < 14)
	crampMasks[0] |= (1 << (enabledSlots[i] + 2));
      if (enabledSlots[i] >= 14 && enabledSlots[i] < 24)
	crampMasks[1] |= (1 << (enabledSlots[i] - 14));
      if (enabledSlots[i] >= 24 && enabledSlots[i] < 38)
	crampMasks[2] |= (1 << (enabledSlots[i] - 24 + 2));
      if (enabledSlots[i] >= 38 && enabledSlots[i] < 48)
	crampMasks[3] |= (1 << (enabledSlots[i] - 38));
    }

  MI2C_setup(&mi2c_cramps[0], &crampMCP[MCPHV0], crampMasks[0], &crampMCP[MCPHV0], 1); // SDA SCL - this is new
  MI2C_setup(&mi2c_cramps[1], &crampMCP[MCPHV1], crampMasks[1], &crampMCP[MCPHV0], 1); // SDA SCL - this is new
  MI2C_setup(&mi2c_cramps[2], &crampMCP[MCPHV2], crampMasks[2], &crampMCP[MCPHV2], 1); // SDA SCL - this is new
  MI2C_setup(&mi2c_cramps[3], &crampMCP[MCPHV3], crampMasks[3], &crampMCP[MCPHV2], 1); // SDA SCL - this is new

  for (int i = 0; i < 4; i++)
    {
      _AMBads1110_init(&adc[i], &mi2c_cramps[i]);
    }

  for (int i = 0; i < 4; i++)
    {
      _AMBads1110_setconfig(&adc[i], 0); // crampchannel);
    }
  for (int i = 0; i < 4; i++)
    {
      _AMBads1110_setconfig(&adc[i], 1); // crampchannel);
    }

  printf("Initialization complete\n");
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
	  startACQ=0;
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

	  SlotCounter = 0;
	  char *pLine = getLine(false, '\r');
	  if (!*pLine)
	    {
	      printf("returned empty - nothing blocked");
	    }
	  else
	    {

	      // Tokenize the input string using strtok
	      char *token = strtok(pLine, " ");
	      while (token != NULL)
		{
		  enabledSlots[SlotCounter++] = atoi(token); // Convert token to a double
		  token = strtok(NULL, " ");                 // Get the next token
		}

	      // Print the parsed numbers
	      for (int i = 0; i < SlotCounter; i++)
		{
		  printf("%d ", enabledSlots[i]);
		}
	      printf("\r\n");
	      free(pLine); // dont forget freeing buffer !!
	    }
	  initialization(); // channelnumber);
	}

      else if (input == 'S')
	startACQ= 1;
      


      if (startACQ){
	int n = 0;


	countloop ++;
	
	float currents[24];
	float currents1[24];
	float tmpcurrents[24];
	float tmpcurrents1[24];

	uint8_t ncramps = 0;
	int tmp = 0;
	uint8_t currentArrayIndex = 0;
	uint8_t currentArrayIndex1 = 0;

	for (int i = 0; i < 4; i++)
	  {
	    ncramps = adc[i]._mi2c->_nCramps;
	    uint8_t retc = _AMBads1110_read(&adc[i], &tmpcurrents, 0);

	    memcpy(currents + currentArrayIndex, tmpcurrents, sizeof(float) * ncramps);
	    currentArrayIndex += ncramps;
	  }

	for (int i = 0; i < 4; i++)
	  {

	    ncramps = adc[i]._mi2c->_nCramps;
	    uint8_t retc = _AMBads1110_read(&adc[i], &tmpcurrents1, 1);

	    // printf("tmpcurrents1 =%04x \n", tmpcurrents1);
	    memcpy(currents1 + currentArrayIndex1, tmpcurrents1, sizeof(float) * ncramps);
	    currentArrayIndex1 += ncramps;
	  }

	for (int i = 0; i < currentArrayIndex; i++)
	  {
	    // flag_recover = 0;
	    currents[i] = 1e2 * currents[i];
	    currents1[i] = 1e2 * currents1[i];
	    printf("%6.3f %6.3f ", currents[i], currents1[i]);
	    // send data out to pi
	  }

	if (currentArrayIndex > 0)
	  printf("\n");

	if (countloop%100==0){
	  clock_t endTime = clock();
	  double executionTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
	  printf("%.8f sec\n", executionTime);
	  startTime = clock();
	}

      }
    }

  return 0;
}
