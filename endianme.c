/* what is the endian-ness of my system */
#include <stdio.h>

int main(int argc, char *argv[])
{
    int num = 1;
    if(*(char *)&num == 1) {
        printf("Yours is an little-endian environment.\n");
        printf("WHich means that the first byte is the least significant figure.\n");
    } else
        printf("Yours encvironment is big-endian\n");
    return 0;
}

/* Notes:
 * an 4 byte has its pointer cast down to a char ptr so you'd expect the memory to be curtailed to one byte,
 * the first byte which is the most significant in big endian terms. In big endian trms this should be zero.
 */
