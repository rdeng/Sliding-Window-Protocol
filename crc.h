#ifndef __CRC_H__
#define __CRC_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <math.h>
#include <sys/time.h>

// return a char with a value of 0 or 1 depending on whether the bit in
// the pos is 0 or 1
char get_bit (char byte, int pos); 

// Function returns the remainder from a CRC calculation on a char* array 
// of length byte_len 
char crc8 (char* array, int byte_len); 

// append crc remainder to the char array
void append_crc (char* array, int array_len); 

// return 1 if a frame is corrupted, otherwise return 0
int is_corrupted (char* array, int array_len); 

#endif
