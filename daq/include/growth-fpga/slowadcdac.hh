#ifndef GROWTH_FPGA_SLOWADCDAC_HH_
#define GROWTH_FPGA_SLOWADCDAC_HH_

#include "types.h"
#include "growth-fpga/registeraccessinterface.hh"

/// Communicates with the Slow ADC/DAC control block (if implemented on FPGA).
class SlowAdcDac : public RegisterAccessInterface {
 public:
  SlowAdcDac(RMAPInitiatorPtr rmapInitiator, std::shared_ptr<RMAPTargetNode> rmapTargetNode);
  ~SlowAdcDac() override = default;

  void setDac(size_t ch, u16 gain, u16 outputValue);
  u16 dacOutputValue(size_t ch) const;
  u16 readAdc(size_t ch);

 private:
  static constexpr size_t NUM_DAC_CHANNELS = 2;
  static constexpr size_t NUM_ADC_CHANNELS = 8;

  static constexpr u32 ADDRESS_DAC_CH_DATA = 0x20000140;
  static constexpr u32 ADDRESS_ADC_CH_DATA = 0x20000142;

  // Register Map
  // === DAC ===
  // i_mcp4822_control_reg <= uartRMAPBusMasterDataOut (15 downto 0);
  // 15    14    13    12    11  10  9  8  7 ...  0
  // nA/B   -    nGA  nSHDN D11 D10 D9 D8  ..... D0
  //
  // === ADC ===
  // i_mcp3208_control_reg <= uartRMAPBusMasterDataOut (15 downto 12);
  // differential   => i_mcp3208_command_reg (3)
  // channel_select => i_mcp3208_command_reg (2 downto 0)

  u16 readAdcReg() const { return read16(ADDRESS_ADC_CH_DATA); }
  u16 readDacReg() const { return read16(ADDRESS_DAC_CH_DATA); }
  void writeAdcReg(u16 value) { write(ADDRESS_ADC_CH_DATA, value); }
  void writeDacReg(u16 value) { write(ADDRESS_DAC_CH_DATA, value); }

  std::array<u16, NUM_DAC_CHANNELS> dacOutputValue_{};
};

class HighvoltageControlGpio : public RegisterAccessInterface {
 public:
  HighvoltageControlGpio(RMAPInitiatorPtr rmapInitiator, std::shared_ptr<RMAPTargetNode> rmapTargetNode);
  ~HighvoltageControlGpio() override = default;

  void outputEnable(bool enable);
  bool outputEnabled() const;

 private:
  /// There are two high-voltage modules, but on the adaptor board, one control signal (Ch.0)
  /// is connected to the enable signals of both HV0 and HV1.
  static constexpr size_t NUM_HV_CHANNELS = 1;

  static constexpr u32 ADDRESS_GPIO = 0x20000144;
};

#endif
