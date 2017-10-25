#include "crc.h"

// return a char with a value of 0 or 1 depending on whether the bit in
// the pos is 0 or 1
char get_bit (char byte, int pos)
{
    byte = byte >> pos;
    if((byte & 0x01) == 1)
        return 1;
    else
        return 0; 
} 

// Function returns the remainder from a CRC calculation on a char* array 
// of length byte_len 
char crc8 (char* array, int byte_len)
{
    char poly = 0x07; //00000111
    unsigned char crc = array[0];
    int i, j;
    for(i = 1; i < byte_len; i++)
    {
        char next_byte = array[i];
        for(j = 7; j >= 0; j--)
        {
            if((crc & 0x80) == 0)
            {
                crc = crc << 1;
                crc = crc | get_bit(next_byte, j);
            }
            else
            {
                crc = crc << 1;
                crc = crc | get_bit(next_byte, j);
                crc = crc ^ poly;
            }
        }
    }

    return crc;
} 

// append crc remainder to the char array
void append_crc (char* array, int array_len)
{
    array[array_len-1] = 0;
    array[array_len-1] = crc8(array, array_len);
}

// return 1 if a frame is corrupted, otherwise return 0
int is_corrupted (char* array, int array_len)
{
    char remain = crc8(array, array_len);
    if(remain  == 0)
        return 0;
    else
        return 1;
}
