#include "ADCDAC.hh"
#include "wiringPi.h"
#include "wiringPiSPI.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

  std::string ADCData::toString() {
    using namespace std;
    std::stringstream ss;
    for (size_t i = 0; i < ADCData::nTemperatureSensors; i++) {
      ss << setw(5) << right << temperature_raw[i] << " " << setw(6) << fixed << setprecision(1) << temperature[i];
      if (i != ADCData::nTemperatureSensors - 1) { ss << " "; }
    }
    for (size_t i = 0; i < ADCData::nCurrentSensors; i++) {
      ss << setw(5) << right << current_raw[i] << " " << setw(6) << fixed << setprecision(1) << current[i];
      if (i != ADCData::nCurrentSensors - 1) { ss << " "; }
    }
    for (size_t i = 0; i < ADCData::nGeneralPurposeADC; i++) {
      ss << setw(5) << right << generalPurposeADC_raw[i] << " " << setw(7) << fixed << setprecision(2)
         << generalPurposeADC[i];
      if (i != ADCData::nGeneralPurposeADC - 1) { ss << " "; }
    }
    return ss.str();
  }

  ADCData::ADCData() {}

  void ADCDAC::initialize() {
    using namespace std;
    // initialize SPI
    int status;
#ifdef DEBUG_WIRINGPI
    cerr << "Opening SPI interface..." << endl;
#endif
    status = wiringPiSPISetup(SPIChannelADC, SPIClockFrequency);
    dumpError(status);
#ifdef DEBUG_WIRINGPI
    cerr << "Opening SPI interface done..." << endl;
#endif
#ifdef DEBUG_WIRINGPI
    printf("wiringPiSPISetup status = %d\n", status);
#endif
    initialized = true;
  }

  void ADCDAC::dumpError(int status) {
    if (status < 0) { printf("Error: %s\n", strerror(errno)); }
  }

  ADCDAC::ADCDAC() { initialize(); }

  i16 ADCDAC::readADC(size_t channel) {
    int status;

    if (channel >= NADCChannels) { return InvalidChannel; }

    if (!initialized) { initialize(); }

    // send AD conversion command to MCP3208
    u8 maskedChannel = channel & 0x07;  // mask lower 3 bits

    u8 data[3] = {static_cast<u8>(0x06 + (maskedChannel >> 2)),
                       static_cast<u8>(0x00 + (maskedChannel << 6)), 0x00};
    status          = wiringPiSPIDataRW(SPIChannelADC, data, 3);
    dumpError(status);

#ifdef DEBUG_WIRINGPI
    printf("wiringPiSPIDataRW status = %d\n", status);
    for (size_t i = 0; i < 3; i++) { printf("data[%d]=%02x\n", i, data[i]); }
#endif

    return (data[1] & 0x0F) * 0x100 + data[2];
  }

  f32 ADCDAC::readTemperature(size_t channel) {
    // check
    if (channel >= NADCChannels) { return InvalidChannel; }

    // convert obtained ADC value to voltage
    i16 adcValue = readADC(channel);
    if (adcValue < 0) { return adcValue; }

    // then to temperature
    f32 temperature = convertToTemperature(adcValue);

#ifdef DEBUG_WIRINGPI
    printf("Temperature = %.2fdegC\n", temperature);
#endif

    return temperature;
  }

 
  f64 ADCDAC::convertToTemperature(u16 adcValue) {
    f32 voltage     = ((f32)adcValue) / ADCMax * ADCVref;
    f32 temperature = (voltage - LM60Offset) / LM60Coefficient;
    return temperature;
  }

  f64 ADCDAC::convertToCurrent(u16 adcValue) {
    f32 voltage = ((f32)adcValue) / ADCMax * ADCVref;
    f32 Vsense  = voltage / (LT6106_Rout / LT6106_Rin);
    f32 current = Vsense / LT6106_Rsense * 1000;  // mA
    return current;
  }

  f64 ADCDAC::convertToVoltage(u16 adcValue) {
    f32 voltage = ((f32)adcValue) / ADCMax * ADCVref;
    return voltage;
  }

  f32 ADCDAC::readCurrent(size_t channel) {
    // check
    if (channel != ADCChannel_Current5V && channel != ADCChannel_Current3V3) { return InvalidChannel; }

    // convert obtained ADC value to current
    i16 adcValue = readADC(channel);
    if (adcValue < 0) { return adcValue; }

#ifdef DEBUG_WIRINGPI
    printf("Voltage = %.3fV\n", voltage);
#endif

    // Vout = Vsense*(Rout/Rin)
    // Isense = Vsense/Rsense

    f32 current = convertToCurrent(adcValue);

#ifdef DEBUG_WIRINGPI
    printf("Current = %.3fmA\n", current);
#endif

    return current;
  }

  int ADCDAC::setDAC(size_t channel, u16 value) {
    int status;

    // check
    if (channel >= NDACChannels) { return InvalidChannel; }
    if (DACMax < value) { return InvalidRange; }

    if (!initialized) { initialize(); }

    // set DAC value
#ifdef DEBUG_WIRINGPI
    printf("DAC value = %d\n", value);
#endif
    unsigned char data[2];
    if (channel == 0x00) {
      data[0] = DACChannelA;
    } else {
      data[0] = DACChannelB;
    }
    data[0] = DACGain1 | DACShutdownNo + ((value & 0x0FFF) >> 8);
    data[1] = (value & 0x00FF);

    status = wiringPiSPIDataRW(SPIChannelDAC, data, 2);
    dumpError(status);

#ifdef DEBUG_WIRINGPI
    printf("wiringPiSPIDataRW status = %d\n", status);
    for (size_t i = 0; i < 2; i++) { printf("data[%d]=%02x\n", i, data[i]); }
#endif

    // dump message if error code is returned
    if (status == -1) {
      printf("Error: %s (%08x\n)\n", strerror(errno), errno);
      return SPICommunicationError;
    }

    return Successful;
  }

  int ADCDAC::disableDAC(size_t channel) {
    int status;
    // check
    if (channel >= NDACChannels) { return InvalidChannel; }

    if (!initialized) { initialize(); }

    // disable DAC value
    unsigned char data[2];
    if (channel == 0x00) {
      data[0] = DACChannelA;
    } else {
      data[0] = DACChannelB;
    }
    data[0] = DACGain1 | DACShutdownYes;
    data[1] = 0x00;

    status = wiringPiSPIDataRW(SPIChannelDAC, data, 2);
    dumpError(status);

#ifdef DEBUG_WIRINGPI
    printf("wiringPiSPIDataRW status = %d\n", status);
    for (size_t i = 0; i < 2; i++) { printf("data[%d]=%02x\n", i, data[i]); }
#endif

    // dump message if error code is returned
    if (status == -1) {
      printf("Error: %s (%08x\n)\n", strerror(errno), errno);
      return SPICommunicationError;
    }

    return Successful;
  }

  ADCData ADCDAC::getADCData() {
    using namespace std;
    ADCData adcData;

#ifdef DEBUG_WIRINGPI
    cout << "ADCDAC::getData(): entered" << endl;
#endif

    for (size_t i = 0; i < ADCData::nTemperatureSensors; i++) {
      adcData.temperature_raw[i] = ADCDAC::readADC(i);
      adcData.temperature[i]     = ADCDAC::convertToTemperature(adcData.temperature_raw[i]);
    }
    for (size_t i = 0; i < ADCData::nCurrentSensors; i++) {
      adcData.current_raw[i] = ADCDAC::readADC(i + 4);
      adcData.current[i]     = ADCDAC::convertToCurrent(adcData.current_raw[i]);
    }
    for (size_t i = 0; i < ADCData::nGeneralPurposeADC; i++) {
      adcData.generalPurposeADC_raw[i] = ADCDAC::readADC(i + 6);
      adcData.generalPurposeADC[i]     = ADCDAC::convertToVoltage(adcData.generalPurposeADC_raw[i]);
    }

#ifdef DEBUG_WIRINGPI
    cout << "ADCDAC::getData(): returning" << endl;
#endif
    return adcData;
  }
