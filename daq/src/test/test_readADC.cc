/*
 * Before running this program, execute the following initialization command:
 * > gpio load spi
 *
 * Expected result:
 * > pi@raspberrypi:~$ ./read_mcp3208
 *
 * wiringPiSPISetup status = 3
 * wiringPiSPIDataRW status = 3
 * data[0]=00
 * data[1]=03
 * data[2]=f8
 * Voltage = 0.620V
 * Temperature = 31.38degC
 *
 */

#include <iostream>
#include "ADCDAC.hh"

int main(int argc, char* argv[]) {
  printf("Read ADC\n");
  ADCDAC adc;
  for (size_t i = 0; i < 4; i++) { printf("Ch.%lu Temperature = %.2fdegC\n", i, adc.readTemperature(i)); }
  for (size_t i = 4; i <= 5; i++) {
    printf("Ch.%lu Current = %.3fmA (ADC %d)\n", i, adc.readCurrent(i), adc.readADC(i));
  }
  for (size_t i = 6; i <= 7; i++) { printf("Ch.%lu ADC = %4d\n", i, adc.readADC(i)); }
}
