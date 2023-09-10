#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pico/stdlib.h> // needed for Pico SDK support


const uint startLineLength = 8; // the linebuffer will automatically grow for longer lines
const char eof = 255;           // EOF in stdio.h -is -1, but getchar returns int 255 to avoid blocking

/*
 *  read a line of any  length from stdio (grows)
 *
 *  @param fullDuplex input will echo on entry (terminal mode) when false
 *  @param linebreak defaults to "\n", but "\r" may be needed for terminals
 *  @return entered line on heap - don't forget calling free() to get memory back
 */
char *getLine(bool fullDuplex, char lineBreak)
{
  // th line buffer
  // will allocated by pico_malloc module if <cstdlib> gets included
  char *pStart = (char *)malloc(startLineLength);
  char *pPos = pStart;             // next character position
  size_t maxLen = startLineLength; // current max buffer size
  size_t len = maxLen;             // current max length
  int c;

  if (!pStart)
  {
    return NULL; // out of memory or dysfunctional heap
  }

  while (1)
  {
    c = getchar(); // expect next character entry
    if (c == eof || c == lineBreak)
    {
      break; // non blocking exit
    }

    if (fullDuplex)
    {
      putchar(c); // echo for fullDuplex terminals
    }

    if (--len == 0)
    { // allow larger buffer
      len = maxLen;
      // double the current line buffer size
      char *pNew = (char *)realloc(pStart, maxLen *= 2);
      if (!pNew)
      {
        free(pStart);
        return NULL; // out of memory abort
      }
      // fix pointer for new buffer
      pPos = pNew + (pPos - pStart);
      pStart = pNew;
    }

    // stop reading if lineBreak character entered
    if ((*pPos++ = c) == lineBreak)
    {
      break;
    }
  }

  *pPos = '\0'; // set string end mark
  return pStart;
};


int findNthSetBitAndFlip(uint16_t mask, int n) {
    int count = 0;
    int position = -1; // Initialize to -1 to indicate not found

    // Iterate through each bit of the mask
    for (int i = 0; i < 16; i++) {
        if ((mask & (1 << i)) != 0) {
            count++;
            if (count == n) {
                position = i; // Found the nth set bit
                break;
            }
        }
    }

    if (position != -1) {
        // Flip the nth set bit
        mask ^= (1 << position);
        return mask;
    } else {
        // If the nth set bit is not found, return an error value (e.g., -1)
        return -1;
    }
};


int findNthSetBit(uint16_t mask, int n) {
    int count = 0;
    int position = -1; // Initialize to -1 to indicate not found

    // Iterate through each bit of the mask
    for (int i = 0; i < 16; i++) {
        if ((mask & (1 << i)) != 0) {
            count++;
            if (count == n) {
                position = i; // Found the nth set bit
                break;
            }
        }
    }

    return position;

   
    
};



int countSetBits(uint16_t mask)
{
  int count = 0;

  while (mask)
  {
    count += mask & 1;
    mask >>= 1;
  }

  return count;
};


void reorderArrays(int arr1[], int arr2[], int size) {
    for (int i = 0; i < size - 1; i++) {
        int minIndex = i;
        for (int j = i + 1; j < size; j++) {
            if (arr2[j] < arr2[minIndex]) {
                minIndex = j;
            }
        }
        if (minIndex != i) {
            swap(&arr1[i], &arr1[minIndex]);
            swap(&arr2[i], &arr2[minIndex]);
        }
    }
}

