/*
 * Before running this program, execute the following initialization command:
 * > gpio load spi
 *
 * Expected result:
 * > pi@raspberrypi:~$ ./read_mcp3208
 *
 * pi@raspberrypi:~$ ./dac 120
 * wiringPiSPISetup status = 3
 * DAC value = 120
 * wiringPiSPIDataRW status = 2
 * data[0]=00
 * data[1]=00
 *
 */

// GPIO2 = /LDAC
// The following setting will disable the LDAC function,
// and the output voltage will be set when /CS returns to 1.
//
// > gpio -g mode 2 out
// > gpio -g write 2 0
// 
#include "ADCDAC.hh"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char* argv[]) {

	if (argc < 3) {
		printf("Provide channel (0 or 1) and DAC value (0-4095).\n");
		exit(-1);
	}

	size_t channel = atoi(argv[1]);
	uint16_t dacValue = atoi(argv[2]);

	printf("Setting DAC Ch=%lu Value=%d\n", channel, dacValue);
	int status = ADCDAC::setDAC(channel, dacValue);

	printf("Result = %d\n", status);
	if (status == ADCDAC::Successful) {
		printf("Successfully set.\n");
	} else {
		printf("Error = %d\n", status);
	}

}
