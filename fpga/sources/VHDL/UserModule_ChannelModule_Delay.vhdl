-- UserModule_ChModule_Delay.vhdl
-- Delay incoming ADC data

library ieee, work;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;
use work.iBus_Library.all;
use work.iBus_AddressMap.all;
use work.UserModule_Library.all;

entity UserModule_ChModule_Delay is
  port(
    ChModule2InternalModule : in  Signal_ChModule2InternalModule;
    --
    AdcDataIn               : in  std_logic_vector(ADCResolution-1 downto 0);
    AdcDataOut              : out std_logic_vector(ADCResolution-1 downto 0);
    --clock and reset
    Clock                   : in  std_logic;
    GlobalReset             : in  std_logic
    );
end UserModule_ChModule_Delay;

architecture Behavioral of UserModule_ChModule_Delay is

  --Signals
  signal Trigger        : std_logic                         := '0';
  signal a              : integer range 0 to MaximumOfDelay := 0;
  --Registers
  signal SampleRegister : std_logic_vector(15 downto 0)     := (others => '0');
  signal InputRegister  : std_logic_vector(15 downto 0)     := (others => '0');
  signal OutputRegister : std_logic_vector(15 downto 0)     := (others => '0');

  type   AdcDataVector is array (integer range <>) of std_logic_vector(ADCResolution-1 downto 0);
  signal AdcDataArray : AdcDataVector(MaximumOfDelay downto 0);

begin

  MainProcess : process (Clock, GlobalReset)
  begin
    if (GlobalReset = '0') then
    elsif (Clock'event and Clock = '1') then
      -- Shift register
      AdcDataArray <= AdcDataArray(MaximumOfDelay-1 downto 0) & AdcDataIn;

      -- Select output
      if (a < MaximumOfDelay) then
        AdcDataOut <= AdcDataArray(a);
      end if;
      if (conv_integer(ChModule2InternalModule.DepthOfDelay) < MaximumOfDelay) then
        a <= conv_integer(ChModule2InternalModule.DepthOfDelay);
      else
        a <= 0;
      end if;
    end if;
  end process;  
end Behavioral;
