#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "HalHandler.h"

#define DEV_ADDR 0x7A

void bypass_test(unsigned char dev_addr)
{
	unsigned char w_data[16];
	unsigned char r_data[16];
	int i;

	srand(time(NULL));
	for (i=0; i<16; i++)
	{
		w_data[i] = rand() & 0xFF;
		r_data[i] = 0;
	}

	I2C_Write(dev_addr, 0x80, w_data, 8);
	I2C_Read(dev_addr, 0x80, r_data, 8);

	for (i=0; i<16; i++)
	{
		printf("%02d: %02x, %02x", i, w_data[i], r_data[i]);
		if ((w_data[i]^0x01) == r_data[i])
		{
			printf(" [%d] - OK\r\n", i);
		}
		else
		{
			printf(" [%d]- ERROR\r\n", i);
		}
	}
}

int main()
{
    HalHandlerInit();
	bypass_test(DEV_ADDR);

	return 0;
}