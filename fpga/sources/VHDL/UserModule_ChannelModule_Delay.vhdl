--UserModule_ChModule_Delay.vhdl
--
--SpaceWire Board / User FPGA / Modularized Structure Template
--UserModule / Ch Module / Delay Module
--
--ver20071022 Takayuki Yuasa
--file created
--based on UserModule_Template_with_BusIFLite.vhdl (ver20071021)

---------------------------------------------------
--Declarations of Libraries
---------------------------------------------------
library ieee, work;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;
use work.iBus_Library.all;
use work.iBus_AddressMap.all;
use work.UserModule_Library.all;

---------------------------------------------------
--Entity Declaration
---------------------------------------------------
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

---------------------------------------------------
--Behavioral description
---------------------------------------------------
architecture Behavioral of UserModule_ChModule_Delay is

  ---------------------------------------------------
  --Declarations of Components
  ---------------------------------------------------
  component UserModule_ChModule_Delay_1Clk_Delay
    port(
      AdcDataIn   : in  std_logic_vector(ADCResolution-1 downto 0);
      AdcDataOut  : out std_logic_vector(ADCResolution-1 downto 0);
      --clock and reset
      Clock       : in  std_logic;
      GlobalReset : in  std_logic
      );
  end component;

  ---------------------------------------------------
  --Declarations of Signals
  ---------------------------------------------------
  --Signals
  signal Trigger        : std_logic                         := '0';
  signal a              : integer range 0 to MaximumOfDelay := 0;
  --Registers
  signal SampleRegister : std_logic_vector(15 downto 0)     := (others => '0');
  signal InputRegister  : std_logic_vector(15 downto 0)     := (others => '0');
  signal OutputRegister : std_logic_vector(15 downto 0)     := (others => '0');

  type   AdcDataVector is array (integer range <>) of std_logic_vector(ADCResolution-1 downto 0);
  signal AdcDataArray : AdcDataVector(MaximumOfDelay downto 0);

  ---------------------------------------------------
  --Beginning of behavioral description
  ---------------------------------------------------
begin

  ---------------------------------------------------
  --Instantiations of Components
  ---------------------------------------------------
  Delay : for I in 0 to MaximumOfDelay-1 generate
    intst_1Clk_Delays : UserModule_ChModule_Delay_1Clk_Delay
      port map(
        ADCDataIn   => AdcDataArray(I),
        ADCDataOut  => AdcDataArray(I+1),
        Clock       => Clock,
        GlobalReset => GlobalReset
        );
  end generate;

  ---------------------------------------------------
  --Static relationships
  ---------------------------------------------------
  AdcDataArray(0) <= AdcDataIn;

  ---------------------------------------------------
  --Dynamic Processes with Sensitivity List
  ---------------------------------------------------
  --UserModule main state machine
  MainProcess : process (Clock, GlobalReset)
  begin
    --is this process invoked with GlobalReset?
    if (GlobalReset = '0') then
      --is this process invoked with Clock Event?
    elsif (Clock'event and Clock = '1') then
      if (a < MaximumOfDelay) then
        AdcDataOut <= AdcDataArray(a);
      end if;
      if (a < MaximumOfDelay) then
        a <= conv_integer(ChModule2InternalModule.DepthOfDelay);
      else
        a <= 0;
      end if;
    end if;
  end process;
  
end Behavioral;

---------------------------------------------------
--Declarations of Libraries
---------------------------------------------------
library ieee, work;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;
use work.iBus_Library.all;
use work.iBus_AddressMap.all;
use work.UserModule_Library.all;


---------------------------------------------------
--Entity Declaration
---------------------------------------------------
entity UserModule_ChModule_Delay_1Clk_Delay is
  port(
    AdcDataIn   : in  std_logic_vector(ADCResolution-1 downto 0);
    AdcDataOut  : out std_logic_vector(ADCResolution-1 downto 0);
    --clock and reset
    Clock       : in  std_logic;
    GlobalReset : in  std_logic
    );
end UserModule_ChModule_Delay_1Clk_Delay;

---------------------------------------------------
--Behavioral description
---------------------------------------------------
architecture Behavioral of UserModule_ChModule_Delay_1Clk_Delay is

  ---------------------------------------------------
  --Declarations of Components
  ---------------------------------------------------

  ---------------------------------------------------
  --Declarations of Signals
  ---------------------------------------------------
  signal AdcData : std_logic_vector(ADCResolution-1 downto 0);

  ---------------------------------------------------
  --Beginning of behavioral description
  ---------------------------------------------------
begin

  ---------------------------------------------------
  --Instantiations of Components
  ---------------------------------------------------

  ---------------------------------------------------
  --Static relationships
  ---------------------------------------------------
  AdcDataOut <= AdcData;

  ---------------------------------------------------
  --Dynamic Processes with Sensitivity List
  ---------------------------------------------------
  --UserModule main state machine
  MainProcess : process (Clock, GlobalReset)
  begin
    --is this process invoked with GlobalReset?
    if (GlobalReset = '0') then
      --is this process invoked with Clock Event?
    elsif (Clock'event and Clock = '1') then
      AdcData <= AdcDataIn;
    end if;
  end process;
  
end Behavioral;
