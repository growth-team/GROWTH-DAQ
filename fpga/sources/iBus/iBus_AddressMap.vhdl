--iBus_AddressMap.vhdl
--iBus Address map definition for UserModules
-- 
--ver20070902
--FADC Module

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;

package iBus_AddressMap is
  ----------------------------------
  -- Address Map
  ----------------------------------
  -- SDRAM
  constant InitialAddressOf_SDRAM : std_logic_vector(31 downto 0) := x"00000000";
  constant FinalAddressOf_SDRAM   : std_logic_vector(31 downto 0) := x"00fffffe";

  -- Templates
  constant InitialAddressOf_Template_Lite : std_logic_vector(15 downto 0) := x"f000";
  constant FinalAddressOf_Template_Lite   : std_logic_vector(15 downto 0) := x"f00e";

  constant InitialAddressOf_Template : std_logic_vector(15 downto 0) := x"f010";
  constant FinalAddressOf_Template   : std_logic_vector(15 downto 0) := x"f01e";

  -- LED Module
  constant InitialAddressOf_LEDModule : std_logic_vector(15 downto 0) := x"f020";
  constant FinalAddressOf_LEDModule   : std_logic_vector(15 downto 0) := x"f02e";

  -- DIO Module
  constant InitialAddressOf_DIOModule : std_logic_vector(15 downto 0) := x"f030";
  constant FinalAddressOf_DIOModule   : std_logic_vector(15 downto 0) := x"f03e";

  -- e2iConnector
  constant InitialAddressOfe2iConnector             : std_logic_vector(15 downto 0) := x"f100";
  constant FinalAddressOfe2iConnector               : std_logic_vector(15 downto 0) := x"f1fe";
  constant InitialAddressOfe2iConnector_InternalRAM : std_logic_vector(15 downto 0) := x"f200";
  constant FinalAddressOfe2iConnector_InternalRAM   : std_logic_vector(15 downto 0) := x"f5fe";  -- +3fe=1024

  -- SDRAMC
  constant InitialAddressOfSDRAMC : std_logic_vector(15 downto 0) := x"f600";
  constant FinalAddressOfSDRAMC   : std_logic_vector(15 downto 0) := x"f6fe";

  -- ChMgr
  constant InitialAddressOf_ChMgr       : std_logic_vector(15 downto 0) := x"0000";
  constant FinalAddressOf_ChMgr         : std_logic_vector(15 downto 0) := x"00fe";
  -- ConsumerMgr
  constant InitialAddressOf_ConsumerMgr : std_logic_vector(15 downto 0) := x"0100";
  constant FinalAddressOf_ConsumerMgr   : std_logic_vector(15 downto 0) := x"01fe";
  -- ChModules
  constant InitialAddressOf_ChModule_0  : std_logic_vector(15 downto 0) := x"1000";
  constant FinalAddressOf_ChModule_0    : std_logic_vector(15 downto 0) := x"10fe";
  constant InitialAddressOf_ChModule_1  : std_logic_vector(15 downto 0) := x"1100";
  constant FinalAddressOf_ChModule_1    : std_logic_vector(15 downto 0) := x"11fe";
  constant InitialAddressOf_ChModule_2  : std_logic_vector(15 downto 0) := x"1200";
  constant FinalAddressOf_ChModule_2    : std_logic_vector(15 downto 0) := x"12fe";
  constant InitialAddressOf_ChModule_3  : std_logic_vector(15 downto 0) := x"1300";
  constant FinalAddressOf_ChModule_3    : std_logic_vector(15 downto 0) := x"13fe";
  constant InitialAddressOf_ChModule_4  : std_logic_vector(15 downto 0) := x"1400";
  constant FinalAddressOf_ChModule_4    : std_logic_vector(15 downto 0) := x"14fe";
  constant InitialAddressOf_ChModule_5  : std_logic_vector(15 downto 0) := x"1500";
  constant FinalAddressOf_ChModule_5    : std_logic_vector(15 downto 0) := x"15fe";
  constant InitialAddressOf_ChModule_6  : std_logic_vector(15 downto 0) := x"1600";
  constant FinalAddressOf_ChModule_6    : std_logic_vector(15 downto 0) := x"16fe";
  constant InitialAddressOf_ChModule_7  : std_logic_vector(15 downto 0) := x"1700";
  constant FinalAddressOf_ChModule_7    : std_logic_vector(15 downto 0) := x"17fe";

  -- Simulation Commander
  constant InitialAddressOf_Commander : std_logic_vector(15 downto 0) := x"2000";
  constant FinalAddressOf_Commander   : std_logic_vector(15 downto 0) := x"2ffe";

  -- for simulation
  constant InitialAddressOf_simulator : std_logic_vector(15 downto 0) := x"3000";
  constant FinalAddressOf_simulator   : std_logic_vector(15 downto 0) := x"3ffe";

  ----------------------------------
  -- Register Address Mapping
  ----------------------------------
  -- e2iConnector
--      constant AddressOf_SDRAM_Addresss_High_Register :       std_logic_vector(15 downto 0) :=x"f110";
--      constant AddressOf_SDRAM_Addresss_Low_Register  :       std_logic_vector(15 downto 0) :=x"f112";
--
--      constant AddressOf_SDRAM_Write_Register                 :       std_logic_vector(15 downto 0) :=x"f104";
--      constant AddressOf_SDRAM_Read_Register                          :       std_logic_vector(15 downto 0) :=x"f106";
--      constant AddressOf_SDRAM_WriteThenIncrement_Register    :       std_logic_vector(15 downto 0) :=x"f108";
--      constant AddressOf_SDRAM_IncrementThenWrite_Register    :       std_logic_vector(15 downto 0) :=x"f10a";
--      constant AddressOf_SDRAM_ReadThenIncrement_Register     :       std_logic_vector(15 downto 0) :=x"f10c";
--      constant AddressOf_SDRAM_IncrementThenRead_Register     :       std_logic_vector(15 downto 0) :=x"f10e";
--
--      constant AddressOf_SDRAM_WriteAddresss_High_Register    :       std_logic_vector(15 downto 0) :=x"f110";
--      constant AddressOf_SDRAM_WriteAddresss_Low_Register     :       std_logic_vector(15 downto 0) :=x"f112";
--      constant AddressOf_SDRAM_ReadAddresss_High_Register     :       std_logic_vector(15 downto 0) :=x"f114";
--      constant AddressOf_SDRAM_ReadAddresss_Low_Register              :       std_logic_vector(15 downto 0) :=x"f116";

  constant AddressOf_SDRAM_Addresss_High_Register : std_logic_vector(15 downto 0) := x"f610";
  constant AddressOf_SDRAM_Addresss_Low_Register  : std_logic_vector(15 downto 0) := x"f612";

  constant AddressOf_SDRAM_Write_Register              : std_logic_vector(15 downto 0) := x"f604";
  constant AddressOf_SDRAM_Read_Register               : std_logic_vector(15 downto 0) := x"f606";
  constant AddressOf_SDRAM_WriteThenIncrement_Register : std_logic_vector(15 downto 0) := x"f608";
  constant AddressOf_SDRAM_IncrementThenWrite_Register : std_logic_vector(15 downto 0) := x"f60a";
  constant AddressOf_SDRAM_ReadThenIncrement_Register  : std_logic_vector(15 downto 0) := x"f60c";
  constant AddressOf_SDRAM_IncrementThenRead_Register  : std_logic_vector(15 downto 0) := x"f60e";

  constant AddressOf_SDRAM_WriteAddresss_High_Register : std_logic_vector(15 downto 0) := x"f610";
  constant AddressOf_SDRAM_WriteAddresss_Low_Register  : std_logic_vector(15 downto 0) := x"f612";
  constant AddressOf_SDRAM_ReadAddresss_High_Register  : std_logic_vector(15 downto 0) := x"f614";
  constant AddressOf_SDRAM_ReadAddresss_Low_Register   : std_logic_vector(15 downto 0) := x"f616";


  constant AddressOf_EventOutputDisableRegister            : std_logic_vector(15 downto 0) := x"0100";
  constant AddressOf_ReadPointerRegister_High              : std_logic_vector(15 downto 0) := x"0102";
  constant AddressOf_ReadPointerRegister_Low               : std_logic_vector(15 downto 0) := x"0104";
  constant AddressOf_WritePointerRegister_High             : std_logic_vector(15 downto 0) := x"0106";
  constant AddressOf_WritePointerRegister_Low              : std_logic_vector(15 downto 0) := x"0108";
  constant AddressOF_GuardBitRegister                      : std_logic_vector(15 downto 0) := x"010a";
  constant AddressOf_AddressUpdateGoRegister               : std_logic_vector(15 downto 0) := x"010c";
  constant AddressOf_GateSize_FastGate_Register            : std_logic_vector(15 downto 0) := x"010e";
  constant AddressOf_GateSize_SlowGate_Register            : std_logic_vector(15 downto 0) := x"0110";
  constant AddressOf_NumberOf_BaselineSample_Register      : std_logic_vector(15 downto 0) := x"0112";
  constant AddressOf_ResetRegister                         : std_logic_vector(15 downto 0) := x"0114";
  constant AddressOf_EventPacket_NumberOfWaveform_Register : std_logic_vector(15 downto 0) := x"0116";
  constant AddressOf_Writepointer_Semaphore_Register       : std_logic_vector(15 downto 0) := x"0118";

  --ChMgr
  constant AddressOf_ChMgr_StartStopRegister             : std_logic_vector(15 downto 0) := x"0002";
  constant AddressOf_ChMgr_StartStopSemaphoreRegister    : std_logic_vector(15 downto 0) := x"0004";
  constant AddressOf_ChMgr_PresetModeRegister            : std_logic_vector(15 downto 0) := x"0006";
  constant AddressOf_ChMgr_PresetLivetimeRegisterL       : std_logic_vector(15 downto 0) := x"0008";
  constant AddressOf_ChMgr_PresetLivetimeRegisterH       : std_logic_vector(15 downto 0) := x"000a";
  constant AddressOf_ChMgr_RealtimeRegisterL             : std_logic_vector(15 downto 0) := x"000c";
  constant AddressOf_ChMgr_RealtimeRegisterM             : std_logic_vector(15 downto 0) := x"000e";
  constant AddressOf_ChMgr_RealtimeRegisterH             : std_logic_vector(15 downto 0) := x"0010";
  constant AddressOf_ChMgr_ResetRegister                 : std_logic_vector(15 downto 0) := x"0012";
  constant AddressOf_ChMgr_AdcClockRegister              : std_logic_vector(15 downto 0) := x"0014";
  constant AddressOf_ChMgr_PresetNumberOfEventRegisterL  : std_logic_vector(15 downto 0) := x"0020";
  constant AddressOf_ChMgr_PresetNumberOfEventRegisterH  : std_logic_vector(15 downto 0) := x"0022";
  constant AddressOf_ChMgr_NumberOfEventChSelectRegister : std_logic_vector(15 downto 0) := x"0024";
  constant AddressOf_ChMgr_NumberOfEventRegisterL        : std_logic_vector(15 downto 0) := x"0026";
  constant AddressOf_ChMgr_NumberOfEventRegisterH        : std_logic_vector(15 downto 0) := x"0028";

  constant AddressOf_LED_Register : std_logic_vector(15 downto 0) := x"f020";

  type iBusAddresses is array (integer range <>) of std_logic_vector(15 downto 0);

end package iBus_AddressMap;
