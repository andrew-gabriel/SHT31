// Distributed with a free-will license.
// Use it any way you want, profit or free, provided it fits in the licenses of its associated works.
// SHT31
// This code is designed to work with the SHT31_I2CS I2C Mini Module available from ControlEverything.com.
// https://www.controleverything.com/content/Humidity?sku=SHT31_I2CS#tabs-0-product_tabset-2

#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>

void main() 
{
	// Create I2C bus
	int file;
	char *bus = "/dev/i2c-1";
	if((file = open(bus, O_RDWR)) < 0) 
	{
		printf("Failed to open the bus. \n");
		exit(1);
	}
	// Get I2C device, SHT31 I2C address is 0x44(68)
	ioctl(file, I2C_SLAVE, 0x44);

	// Send high repeatability measurement command
	// Command msb, command lsb(0x2C, 0x06)
	char config[2] = {0};
	config[0] = 0x2C;
	config[1] = 0x06;
	write(file, config, 2);
	sleep(1);

	// Read 6 bytes of data
	// temp msb, temp lsb, temp CRC, humidity msb, humidity lsb, humidity CRC
	char data[6] = {0};
	if(read(file, data, 6) != 6)
	{
		printf("Error : Input/output Error \n");
	}
	else
	{
	// Convert the data
	double cTemp = (((data[0] * 256) + data[1]) * 175.0) / 65535.0  - 45.0;
	double fTemp = (((data[0] * 256) + data[1]) * 315.0) / 65535.0 - 49.0;
	double humidity = (((data[3] * 256) + data[4])) * 100.0 / 65535.0;

	// Output data to screen
	printf("Temperature in Celsius : %.2f C \n", cTemp);
	printf("Temperature in Fahrenheit : %.2f F \n", fTemp);
	printf("Relative Humidity is : %.2f RH \n", humidity);
	}
}
