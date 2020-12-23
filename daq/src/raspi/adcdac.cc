#include "adcdac.hh"
#include "wiringPi.h"
#include "wiringPiSPI.h"

#include "spdlog/spdlog.h"

#include <iomanip>
#include <sstream>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

std::string ADCData::toString() const {
  std::stringstream ss;
  for (size_t i = 0; i < ADCData::nTemperatureSensors; i++) {
    ss << std::setw(5) << std::right << temperature_raw[i] << " " << std::setw(6) << std::fixed << std::setprecision(1)
       << temperature[i];
    if (i != ADCData::nTemperatureSensors - 1) {
      ss << " ";
    }
  }
  for (size_t i = 0; i < ADCData::nCurrentSensors; i++) {
    ss << std::setw(5) << std::right << current_raw[i] << " " << std::setw(6) << std::fixed << std::setprecision(1)
       << current[i];
    if (i != ADCData::nCurrentSensors - 1) {
      ss << " ";
    }
  }
  for (size_t i = 0; i < ADCData::nGeneralPurposeADC; i++) {
    ss << std::setw(5) << std::right << generalPurposeADC_raw[i] << " " << std::setw(7) << std::fixed
       << std::setprecision(2) << generalPurposeADC[i];
    if (i != ADCData::nGeneralPurposeADC - 1) {
      ss << " ";
    }
  }
  return ss.str();
}

ADCData::ADCData() {}

void ADCDAC::initialize() {
  i32 status{};
  spdlog::debug("Opening SPI interface...");

  status = wiringPiSPISetup(SPIChannelADC, SPIClockFrequency);
  dumpError(status);
  spdlog::debug("Opening SPI interface done...");
  spdlog::debug("wiringPiSPISetup status = {}", status);
  initialized = true;
}

void ADCDAC::dumpError(i32 status) {
  if (status < 0) {
    spdlog::error("error = {} ({})", strerror(errno), errno);
  }
}

ADCDAC::ADCDAC() { initialize(); }

i16 ADCDAC::readADC(size_t channel) {
  i32 status{};

  if (channel >= NADCChannels) {
    return InvalidChannel;
  }

  if (!initialized) {
    initialize();
  }

  // send AD conversion command to MCP3208
  const u8 maskedChannel = channel & 0x07;  // mask lower 3 bits
  u8 data[3] = {static_cast<u8>(0x06 + (maskedChannel >> 2)), static_cast<u8>(0x00 + (maskedChannel << 6)), 0x00};
  status = wiringPiSPIDataRW(SPIChannelADC, data, 3);
  dumpError(status);

  spdlog::debug("wiringPiSPIDataRW status = {}", status);
  for (size_t i = 0; i < 3; i++) {
    spdlog::debug("data[{}]={}", i, data[i]);
  }

  return (data[1] & 0x0F) * 0x100 + data[2];
}

f32 ADCDAC::readTemperature(size_t channel) {
  if (channel >= NADCChannels) {
    return InvalidChannel;
  }

  // convert obtained ADC value to voltage
  const i16 adcValue = readADC(channel);
  if (adcValue < 0) {
    return adcValue;
  }

  // then to temperature
  const f32 temperature = convertToTemperature(adcValue);

  spdlog::debug("Temperature = {:.2f} degC", temperature);

  return temperature;
}

f64 ADCDAC::convertToTemperature(u16 adcValue) {
  const f32 voltage = ((f32)adcValue) / ADCMax * ADCVref;
  const f32 temperature = (voltage - LM60Offset) / LM60Coefficient;
  return temperature;
}

f64 ADCDAC::convertToCurrent(u16 adcValue) {
  const f32 voltage = ((f32)adcValue) / ADCMax * ADCVref;
  const f32 Vsense = voltage / (LT6106_Rout / LT6106_Rin);
  const f32 current = Vsense / LT6106_Rsense * 1000;  // mA
  return current;
}

f64 ADCDAC::convertToVoltage(u16 adcValue) {
  const f32 voltage = ((f32)adcValue) / ADCMax * ADCVref;
  return voltage;
}

f32 ADCDAC::readCurrent(size_t channel) {
  if (channel != ADCChannel_Current5V && channel != ADCChannel_Current3V3) {
    return InvalidChannel;
  }

  // convert obtained ADC value to current
  const i16 adcValue = readADC(channel);
  if (adcValue < 0) {
    return adcValue;
  }

  // Vout = Vsense*(Rout/Rin)
  // Isense = Vsense/Rsense
  const f32 current = convertToCurrent(adcValue);
  spdlog::debug("Current = {:.3f} mA", current);

  return current;
}

i32 ADCDAC::setDAC(size_t channel, u16 value) {
  i32 status{};
  if (channel >= NDACChannels) {
    return InvalidChannel;
  }
  if (DACMax < value) {
    return InvalidRange;
  }

  if (!initialized) {
    initialize();
  }

  // set DAC value
  spdlog::debug("DAC value = {}", value);
  u8 data[2];
  if (channel == 0x00) {
    data[0] = DACChannelA;
  } else {
    data[0] = DACChannelB;
  }
  data[0] = DACGain1 | DACShutdownNo + ((value & 0x0FFF) >> 8);
  data[1] = (value & 0x00FF);

  status = wiringPiSPIDataRW(SPIChannelDAC, data, 2);
  dumpError(status);

  spdlog::debug("wiringPiSPIDataRW status = {}", status);
  for (size_t i = 0; i < 2; i++) {
    spdlog::debug("data[{}]={:02x}", i, data[i]);
  }

  // dump message if error code is returned
  if (status == -1) {
    spdlog::error("error = {} ({})", strerror(errno), errno);
    return SPICommunicationError;
  }

  return Successful;
}

i32 ADCDAC::disableDAC(size_t channel) {
  i32 status;
  if (channel >= NDACChannels) {
    return InvalidChannel;
  }

  if (!initialized) {
    initialize();
  }

  // disable DAC value
  u8 data[2];
  if (channel == 0x00) {
    data[0] = DACChannelA;
  } else {
    data[0] = DACChannelB;
  }
  data[0] = DACGain1 | DACShutdownYes;
  data[1] = 0x00;

  status = wiringPiSPIDataRW(SPIChannelDAC, data, 2);
  dumpError(status);

  spdlog::debug("wiringPiSPIDataRW status = {}", status);
  for (size_t i = 0; i < 2; i++) {
    spdlog::debug("data[{}]={:02x}", i, data[i]);
  }

  // dump message if error code is returned
  if (status == -1) {
    spdlog::error("error = {} ({}\n)\n", strerror(errno), errno);
    return SPICommunicationError;
  }

  return Successful;
}

ADCData ADCDAC::getADCData() {
  ADCData adcData;
  spdlog::debug("ADCDAC::getData(): entered");

  for (size_t i = 0; i < ADCData::nTemperatureSensors; i++) {
    adcData.temperature_raw[i] = ADCDAC::readADC(i);
    adcData.temperature[i] = ADCDAC::convertToTemperature(adcData.temperature_raw[i]);
  }
  for (size_t i = 0; i < ADCData::nCurrentSensors; i++) {
    adcData.current_raw[i] = ADCDAC::readADC(i + 4);
    adcData.current[i] = ADCDAC::convertToCurrent(adcData.current_raw[i]);
  }
  for (size_t i = 0; i < ADCData::nGeneralPurposeADC; i++) {
    adcData.generalPurposeADC_raw[i] = ADCDAC::readADC(i + 6);
    adcData.generalPurposeADC[i] = ADCDAC::convertToVoltage(adcData.generalPurposeADC_raw[i]);
  }

  spdlog::debug("ADCDAC::getData(): returning");
  return adcData;
}
