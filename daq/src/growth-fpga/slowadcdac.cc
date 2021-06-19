#include "growth-fpga/slowadcdac.hh"

#include <cassert>

SlowAdcDac::SlowAdcDac(RMAPInitiatorPtr rmapInitiator, std::shared_ptr<RMAPTargetNode> rmapTargetNode)
    : RegisterAccessInterface(std::move(rmapInitiator), std::move(rmapTargetNode)) {}

void SlowAdcDac::setDac(size_t ch, u16 gain, u16 outputValue) {
  assert(ch < NUM_DAC_CHANNELS);
  assert(gain == 1 || gain == 2);
  assert(outputValue <= 0xFFF);  // 12 bits
  const u16 gainBit = (gain == 1) ? 1 : 0;
  constexpr u16 nSHDN = 1;  // always output enable
  const u16 registerValue = (static_cast<u16>(ch) << 15) | (gainBit << 13) | (nSHDN << 12) | (outputValue & 0x0FFF);
  writeDacReg(registerValue);
  dacOutputValue_.at(ch) = outputValue;
}

u16 SlowAdcDac::dacOutputValue(size_t ch) const {
  assert(ch < NUM_DAC_CHANNELS);
  return dacOutputValue_.at(ch);
}

u16 SlowAdcDac::readAdc(size_t ch) {
  assert(ch < NUM_ADC_CHANNELS);
  constexpr u16 DIFFERENTIAL_FLAG = 0;  // not differential (i.e. single-ended mode)
  const u16 adcRegisterValue = (DIFFERENTIAL_FLAG << 15) | (static_cast<u16>(ch) << 12);
  writeAdcReg(adcRegisterValue);
  constexpr u16 ADC_READ_BUSY_FLAG = 1 << 15;
  u16 readResult = readAdcReg();
  while ((readResult & ADC_READ_BUSY_FLAG) != 0) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
    // TODO: add log message here
    readResult = readAdcReg();
  }
  return readResult;
}

HighvoltageControlGpio::HighvoltageControlGpio(RMAPInitiatorPtr rmapInitiator,
                                               std::shared_ptr<RMAPTargetNode> rmapTargetNode)
    : RegisterAccessInterface(std::move(rmapInitiator), std::move(rmapTargetNode)) {}

void HighvoltageControlGpio::outputEnable(bool enable) {
  const u16 registerValue = enable ? 1 : 0;
  write(ADDRESS_GPIO, registerValue);
}

bool HighvoltageControlGpio::outputEnabled() const {
  constexpr u16 OUTPUT_ENABLED = 1;
  return (read16(ADDRESS_GPIO) & 0x01) == OUTPUT_ENABLED;
}
