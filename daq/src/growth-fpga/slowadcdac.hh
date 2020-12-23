#ifndef GROWTH_FPGA_SLOWADCDAC_HH_
#define GROWTH_FPGA_SLOWADCDAC_HH_

#include "types.h"
#include "growth-fpga/registeraccessinterface.hh"

#include <cassert>

class RMAPHandlerUART;

/** A class which represents ConsumerManager module in the VHDL logic.
 * It also holds information on a ring buffer constructed on SDRAM.
 */
class SlowAdcDac : public RegisterAccessInterface {
 public:
  /** Constructor.
   * @param[in] rmapHandler RMAPHandlerUART
   * @param[in] adcRMAPTargetNode RMAPTargetNode that corresponds to the ADC board
   */
  SlowAdcDac(std::shared_ptr<RMAPHandlerUART> rmapHandler, std::shared_ptr<RMAPTargetNode> rmapTargetNode)
      : RegisterAccessInterface(rmapHandler, rmapTargetNode) {}
  ~SlowAdcDac() override = default;

  void setDac(size_t ch, u16 gain, u16 outputValue) {
    assert(ch < NUM_DAC_CHANNELS);
    assert(gain == 1 || gain == 2);
    assert(outputValue <= 0xFFF);  // 12 bits
    const u16 gainBit = (gain == 1) ? 1 : 0;
    constexpr u16 nSHDN = 1;  // always output enable
    const u16 registerValue = (static_cast<u16>(ch) << 15) | (gainBit << 13) | (nSHDN << 12) | (outputValue & 0x0FFF);
    writeDacReg(registerValue);
    dacOutputValue_.at(ch) = outputValue;
  }

  u16 dacOutputValue(size_t ch) const {
    assert(ch == 0 || ch == 1);
    return dacOutputValue_.at(ch);
  }

  u16 readAdc(size_t ch) {
    assert(ch < NUM_ADC_CHANNELS);
    constexpr u16 DIFFERENTIAL_FLAG = 0;  // not differential (i.e. single-ended mode)
    const u16 adcRegisterValue = (DIFFERENTIAL_FLAG << 15) | (static_cast<u16>(ch) << 12);
    writeAdcReg(adcRegisterValue);
    return readAdcReg();
  }

 private:
  static constexpr size_t NUM_DAC_CHANNELS = 2;
  static constexpr size_t NUM_ADC_CHANNELS = 8;

  static constexpr u32 ADDRESS_DAC_CH_DATA = 0x20000140;
  static constexpr u32 ADDRESS_ADC_CH_DATA = 0x20000142;
  static constexpr u32 ADDRESS_GPIO = 0x20000144;

  u16 readAdcReg() const { return read16(ADDRESS_ADC_CH_DATA); }
  u16 readDacReg() const { return read16(ADDRESS_DAC_CH_DATA); }
  void writeAdcReg(u16 value) { write(ADDRESS_ADC_CH_DATA, value); }
  void writeDacReg(u16 value) { write(ADDRESS_DAC_CH_DATA, value); }

  std::array<u16, NUM_DAC_CHANNELS> dacOutputValue_{};
};

#endif

// DAC
// i_mcp4822_control_reg <= uartRMAPBusMasterDataOut (15 downto 0);
// 15    14    13    12    11  10  9  8  7 ...  0
// nA/B   -    nGA  nSHDN D11 D10 D9 D8  ..... D0
//
// ADC
// i_mcp3208_control_reg <= uartRMAPBusMasterDataOut (15 downto 12);
// differential   => i_mcp3208_command_reg (3),
// channel_select => i_mcp3208_command_reg (2 downto 0),
