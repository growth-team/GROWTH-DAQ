/*
 * ADCDAC.hh
 * Controls ADC/DAC function of GROWTH-FY2015 ADC board.
 *  Created on: Jun 6, 2015
 *      Author: Takayuki Yuasa
 */

#ifndef ADCDAC_HH_
#define ADCDAC_HH_

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CxxUtilities/CxxUtilities.hh"
#include "wiringPi.h"
#include "wiringPiSPI.h"

/** Represents ADC data.
 */
class ADCData {
 public:
  static const size_t nTemperatureSensors = 4;
  uint16_t temperature_raw[nTemperatureSensors];
  double temperature[nTemperatureSensors];

  static const size_t nCurrentSensors = 2;
  uint16_t current_raw[nCurrentSensors];
  double current[nCurrentSensors];

  static const size_t nGeneralPurposeADC = 2;
  uint16_t generalPurposeADC_raw[nGeneralPurposeADC];
  double generalPurposeADC[nGeneralPurposeADC];

 public:
  std::string toString() {
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

 public:
  ADCData() {}
};

/** Controls ADC/DAC function.
 */
class ADCDAC {
 public:
  // ADC (MCP3208)
  static const size_t NADCChannels          = 8;
  static const int SPIChannelADC            = 0x00;
  static const uint16_t ADCMax              = 4095;
  static constexpr float ADCVref            = 2.5;  // V
  static const size_t ADCChannel_Current5V  = 4;
  static const size_t ADCChannel_Current3V3 = 5;

  // DAC
  static const size_t NDACChannels = 2;
  static const int SPIChannelDAC   = 0x01;
  static const uint16_t DACMin     = 0;
  static const uint16_t DACMax     = 4095;

  static const unsigned char DACChannelA    = 0x00;
  static const unsigned char DACChannelB    = 0x80;
  static const unsigned char DACGain1       = 0x20;
  static const unsigned char DACGain2       = 0x00;
  static const unsigned char DACShutdownYes = 0x00;
  static const unsigned char DACShutdownNo  = 0x10;

  // SPI
  static const int SPIClockFrequency = 500000;  // Hz

  // Temperature sensor LM60
  static constexpr double LM60Coefficient = 0.00625;  // V/deg
  static constexpr double LM60Offset      = 0.424;    // V

  // Error status
  enum ADCDACError {
    SPICommunicationError = -3,  //
    InvalidRange          = -2,  //
    InvalidChannel        = -1,  //
    Successful            = 0    //
  };

 private:
  void dumpError(int status) {
    if (status < 0) { printf("Error: %s\n", strerror(errno)); }
  }

 private:
  bool initialized = false;

 public:
  ADCDAC() { initialize(); }

 private:
  void initialize() {
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

 public:
  /** Reads ADC value.
   * @param[in] channel ADC channel 0-7 (0-3 are temperature sensor)
   * @return 12-bit ADC value
   */
  int16_t readADC(size_t channel) {
    int status;

    if (channel >= NADCChannels) { return InvalidChannel; }

    if (!initialized) { initialize(); }

    // send AD conversion command to MCP3208
    uint8_t maskedChannel = channel & 0x07;  // mask lower 3 bits

    uint8_t data[3] = {static_cast<uint8_t>(0x06 + (maskedChannel >> 2)),
                       static_cast<uint8_t>(0x00 + (maskedChannel << 6)), 0x00};
    status          = wiringPiSPIDataRW(SPIChannelADC, data, 3);
    dumpError(status);

#ifdef DEBUG_WIRINGPI
    printf("wiringPiSPIDataRW status = %d\n", status);
    for (size_t i = 0; i < 3; i++) { printf("data[%d]=%02x\n", i, data[i]); }
#endif

    return (data[1] & 0x0F) * 0x100 + data[2];
  }

 public:
  /** Reads temperature sensor.
   * @param[in] channel Temperature sensor channel 0-3
   * @return temperature in degC
   */
  float readTemperature(size_t channel) {
    // check
    if (channel >= NADCChannels) { return InvalidChannel; }

    // convert obtained ADC value to voltage
    int16_t adcValue = readADC(channel);
    if (adcValue < 0) { return adcValue; }

    // then to temperature
    float temperature = convertToTemperature(adcValue);

#ifdef DEBUG_WIRINGPI
    printf("Temperature = %.2fdegC\n", temperature);
#endif

    return temperature;
  }

 public:
  double convertToTemperature(uint16_t adcValue) {
    float voltage     = ((float)adcValue) / ADCMax * ADCVref;
    float temperature = (voltage - LM60Offset) / LM60Coefficient;
    return temperature;
  }

 public:
  double convertToCurrent(uint16_t adcValue) {
    float voltage = ((float)adcValue) / ADCMax * ADCVref;
    float Vsense  = voltage / (LT6106_Rout / LT6106_Rin);
    float current = Vsense / LT6106_Rsense * 1000;  // mA
    return current;
  }

 public:
  double convertToVoltage(uint16_t adcValue) {
    float voltage = ((float)adcValue) / ADCMax * ADCVref;
    return voltage;
  }

 public:
  static constexpr float LT6106_Rout   = 10e3;   // 10kOhm
  static constexpr float LT6106_Rin    = 100;    // 100Ohm
  static constexpr float LT6106_Rsense = 10e-3;  // 10mOhm

 public:
  /** Reads consumed current of 5V or 3.3V line.<br>
   * Channel 4 = 5V current <br>
   * Channel 5 = 3.3V current <br>
   * @param[in] channel Temperature sensor channel 4-5
   * @return current in mA
   */
  float readCurrent(size_t channel) {
    // check
    if (channel != ADCChannel_Current5V && channel != ADCChannel_Current3V3) { return InvalidChannel; }

    // convert obtained ADC value to current
    int16_t adcValue = readADC(channel);
    if (adcValue < 0) { return adcValue; }

#ifdef DEBUG_WIRINGPI
    printf("Voltage = %.3fV\n", voltage);
#endif

    // Vout = Vsense*(Rout/Rin)
    // Isense = Vsense/Rsense

    float current = convertToCurrent(adcValue);

#ifdef DEBUG_WIRINGPI
    printf("Current = %.3fmA\n", current);
#endif

    return current;
  }

 public:
  /** Sets value to DAC.
   * Execute the following in the command line before using this method.
   * <pre>
   * > gpio -g mode 2 out
   * > gpio -g write 2 0
   * </pre>
   * @param[in] channel 0 or 1 for Channel 0 and 1
   * @param[in] value DAC value (0-4095)
   * @return status (0=successfully set, -1=range error)
   */
  int setDAC(size_t channel, uint16_t value) {
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

 public:
  /** Disable DAC output.
   * @param[in] channel 0 or 1 for Channel 0 and 1
   * @return status
   */
  int disableDAC(size_t channel) {
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

 public:
  ADCData getADCData() {
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
};

#endif /* ADCDAC_HH_ */
