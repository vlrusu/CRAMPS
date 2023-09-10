#ifndef MYUTILS_H
#define MYUTILS_H

/// @brief 
/// @param fullDuplex 
/// @param lineBreak 
/// @return 
char *getLine(bool fullDuplex, char lineBreak);
int findNthSetBitAndFlip(unsigned int mask, int n);
int countSetBits(uint16_t mask);
int findNthSetBit(uint16_t mask, int n) ;
void reorderArrays(int arr1[], int arr2[], int size) ;


#endif// _H