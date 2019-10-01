/*
 * ADCDAC.hh
 * Controls ADC/DAC function of GROWTH-FY2015 ADC board.
 *  Created on: Jun 6, 2015
 *      Author: Takayuki Yuasa
 */

#ifndef ADCDAC_HH_
#define ADCDAC_HH_

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

  std::string toString();
  ADCData();
};

/** Controls ADC/DAC function.
 */
class ADCDAC {
 public:
  ADCDAC();

  /** Reads ADC value.
   * @param[in] channel ADC channel 0-7 (0-3 are temperature sensor)
   * @return 12-bit ADC value
   */
  int16_t readADC(size_t channel);
  /** Reads temperature sensor.
   * @param[in] channel Temperature sensor channel 0-3
   * @return temperature in degC
   */
  float readTemperature(size_t channel);
  double convertToTemperature(uint16_t adcValue);
  double convertToCurrent(uint16_t adcValue);
  double convertToVoltage(uint16_t adcValue);

  /** Reads consumed current of 5V or 3.3V line.<br>
   * Channel 4 = 5V current <br>
   * Channel 5 = 3.3V current <br>
   * @param[in] channel Temperature sensor channel 4-5
   * @return current in mA
   */
  float readCurrent(size_t channel);
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
  int setDAC(size_t channel, uint16_t value);
  /** Disable DAC output.
   * @param[in] channel 0 or 1 for Channel 0 and 1
   * @return status
   */
  int disableDAC(size_t channel);
  ADCData getADCData();

  static constexpr float LT6106_Rout   = 10e3;   // 10kOhm
  static constexpr float LT6106_Rin    = 100;    // 100Ohm
  static constexpr float LT6106_Rsense = 10e-3;  // 10mOhm

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
  void dumpError(int status);
  bool initialized = false;
  void initialize();
};

#endif /* ADCDAC_HH_ */
