// Distributed with a free-will license.
// Use it any way you want, profit or free, provided it fits in the licenses of its associated works.
// SHT31
// This code is designed to work with the SHT31_I2CS I2C Mini Module available from ControlEverything.com.
// https://www.controleverything.com/content/Humidity?sku=SHT31_I2CS#tabs-0-product_tabset-2

// [g]cc -lm SHT31.c

#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

int check_crc (unsigned char *byte)
{
	unsigned char crc = 0xff;
	int i, j;

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 8; j++)
		{
			if (((byte[i] << j) & 0x80) == (crc & 0x80))
			{
				crc <<= 1;
			}
			else
			{
				crc <<= 1;
				crc ^= 0x30;
				crc |= 1;
			}
                }
	}
	return (crc == 0);
}

void main(int argc, char **argv) 
{
	// defaults
	int address = 0x44;
	char config[2] = {0x24, 0x00};	// measurement command without clock stretching
	char *bus = "/dev/i2c-1";

	int c;
	while ((c = getopt(argc, argv, "01b:chml")) != EOF)
	{
		switch (c)
		{
		case '0':	// address 0x44
		case '1':	// address 0x45
			address = c - '0' + 0x44;
			break;
		case 'b':	// I2C bus
			bus = optarg;
			break;
		case 'c':	// Test CRC function
			{
				unsigned char beef[3] = {0xBE, 0xEF, 0x92};
				int ret = check_crc(&beef[0]);
				printf("Test CRC %s\n", ret ? "OK" : "FAILED");
				exit(ret ^ 1);
			}
		case 'h':	// high repeatability
			config[1] = 0x00;
			break;
		case 'm':	// medium repeatability
			config[1] = 0x0B;
			break;
		case 'l':	// low repeatability
			config[1] = 0x16;
			break;
		case '?':
			fprintf(stderr, "usage: %s [-0 | -1] [-h | -m | -l] [-b bus]\n",
			    basename(argv[0]));
			exit(1);
		}
	}

	// Create I2C bus
	int file;
	if((file = open(bus, O_RDWR)) < 0) 
	{
		perror(bus);
		exit(1);
	}
	// Get I2C device, SHT31 I2C address is 0x44(68)
	if (ioctl(file, I2C_SLAVE, address) < 0)
	{
		perror("ioctl(I2C_SLAVE)");
		exit(1);
	}

	// Send measurement command
	if (write(file, config, 2) != 2)
	{
		perror("write");
		exit(1);
	}
	struct timespec start;
	clock_gettime(CLOCK_MONOTONIC_RAW, &start); // start time

	// Read 6 bytes of data
	// temp msb, temp lsb, temp CRC, humidity msb, humidity lsb, humidity CRC
	unsigned char data[6];
	int loops = 1;
	int ret;
	// loop until we get the answer (typically 12-15ms)
	do
	{
		usleep(1000);	// Wait 1ms

		// Note - if productising this code, increase the usleep
		// to 10ms or get rid of the loop and wait 100ms once.
		// We only use a small value here so you can see how many
		// milliseconds the conversion takes in the SHT31.

		ret = read(file, data, 6);
	}
	while (ret < 0 && errno == EREMOTEIO && loops++ < 1000);
	if (ret != 6)
	{
		if (ret < 0)
			perror("read");
		else
			fprintf(stderr,
			    "Giving up after %d read loops (read() = %d)\n",
			    loops, ret);
		exit(1);
	}
	struct timespec finish;
	clock_gettime(CLOCK_MONOTONIC_RAW, &finish); // finish time

	ret = 0;
	if (!check_crc(&data[0]))
	{
		fprintf(stderr, "Temperature CRC error {%02X,%02X,%02X}\n",
		    data[0], data[1], data[2]);
		ret |= 1;
	}
	if (!check_crc(&data[3]))
	{
		fprintf(stderr, "Humidity CRC error {%02X,%02X,%02X}\n",
		    data[3], data[4], data[5]);
		ret |= 1;
	}

	if (ret == 0)	// CRC checks out OK
	{
		// Convert the data
		double cTemp = (((data[0] * 256) + data[1]) * 175.0) / 65535.0  - 45.0;
		double fTemp = (((data[0] * 256) + data[1]) * 315.0) / 65535.0 - 49.0;
		double humidity = (((data[3] * 256) + data[4])) * 100.0 / 65535.0;
		// Calculate Dew point (Magnus formula optimised for range –45C to 60C)
		// [Sensirion: "Application Note Dew-point Calculation"]
		double H = (log10(humidity)-2)/0.4343 + (17.62*cTemp)/(243.12+cTemp);
		double cDew = 243.12*H/(17.62-H);

		// Output data to screen
		printf("%-17s : %6.2f C   %6.2f F\n",     "Temperature",   cTemp, fTemp);
		printf("%-17s : %6.2f %%\n",              "Relative humidity",      humidity);
		printf("%-17s : %6.2f C   %6.2f F\n",     "Dew point",     cDew, cDew * 1.8 + 32);
	}

	int duration = finish.tv_nsec / 1000000 - start.tv_nsec / 1000000;
	duration += (finish.tv_sec - start.tv_sec) * 1000;
	printf("%-17s : %3d ms     %3d read loop%s\n", "Response time", duration, loops,
	    loops != 1 ? "s" : "");

	exit(ret);
}
