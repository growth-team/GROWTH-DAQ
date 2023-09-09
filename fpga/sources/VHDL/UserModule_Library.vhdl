--UserModule_Library.vhdl
--
--SpaceWire Board / User FPGA / Modularized Structure Template
--UserModule / Library
--
--ver20071025 Takayuki Yuasa
--renamed from UserModule_ChModule_Library.vhdl
--to UserModule_Library.vhdl
--
--ver20081027 Takayuki Yuasa
--ver20071022 Takayuki Yuasa
--file created
--based on iBus_Library.vhdl (ver20071021)
--ver20141105 Takayuki Yuasa
--updated for SpaceFibreADC FPGA IP Core

---------------------------------------------------
--Declarations of Libraries
---------------------------------------------------
library ieee, work;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

---------------------------------------------------
--Package for UserModules
---------------------------------------------------
package UserModule_Library is

  ---------------------------------------------------
  --Global variables
  ---------------------------------------------------
  constant Count10msec   : integer := 1250000;   -- at 125Mhz
  constant Count100msec  : integer := 12500000;  -- at 125MHz

  -- ADC configuration
  constant NADCChannels : integer := 4;
  constant ADCResolution : integer := 12;

  -- Channel module configuration
  constant MaximumOfDelay                    : integer := 32;    --32clk delay
  constant WaveformBufferDataWidth           : integer := 18;    -- control flag 2 bits + data 16 bits (ADC sample or header)
  constant EventPackeBufferDataWidth         : integer := 16;    --Event Packet Buffer FIFO = 16bit word
  constant MaximumOfProducerAndConsumerNodes : integer := 16;
  constant NumberOfProducerNodes             : integer := 4;
  constant NumberOfConsumerNodes             : integer := 4;

  constant BUFFER_FIFO_CONTROL_FLAG_FIRST_DATA : std_logic_vector(1 downto 0) := "01";
  constant BUFFER_FIFO_CONTROL_FLAG_DATA : std_logic_vector(1 downto 0) := "00";
  constant BUFFER_FIFO_CONTROL_FLAG_HEADER : std_logic_vector(1 downto 0) := "10";
  constant BUFFER_FIFO_CONTROL_FLAG_END : std_logic_vector(1 downto 0) := "11";

  constant HEADER_FLAG : std_logic_vector(3 downto 0) := "0100";

  constant REGISTER_ALL_ONE  : std_logic_vector(15 downto 0) := x"ffff";
  constant REGISTER_ALL_ZERO : std_logic_vector(15 downto 0) := x"0000";

  constant InitialAddressOf_Sdram_EventList : std_logic_vector(31 downto 0) := x"00000000";
  constant FinalAddressOf_Sdram_EventList   : std_logic_vector(31 downto 0) := x"00fffffe";

  type     adcClockFrequencies is (ADCClock200MHz, ADCClock100MHz, ADCClock50MHz);
  constant ADCClockFrequencyRegister_ADCClock200MHz : std_logic_vector(15 downto 0) := conv_std_logic_vector(20000, 16);
  constant ADCClockFrequencyRegister_ADCClock100MHz : std_logic_vector(15 downto 0) := conv_std_logic_vector(10000, 16);
  constant ADCClockFrequencyRegister_ADCClock50MHz  : std_logic_vector(15 downto 0) := conv_std_logic_vector(5000, 16);

  type   Vector10Bits is array (natural range<>) of std_logic_vector(9 downto 0);
  type   Vector12Bits is array (natural range<>) of std_logic_vector(11 downto 0);

  ---------------------------------------------------
  --Signals: between ChMgr and InternalModule
  ---------------------------------------------------
  constant WidthOfTriggerMode     : integer := 4;  --max mode=2^4=16types
  constant WidthOfNumberOfSamples : integer := 16;  --max sample=2^16=65536
  constant WidthOfDepthOfDelay    : integer := 7;  --max depth=2^7=128
  constant WidthOfSizeOfHeader    : integer := 4;  --max depth=2^4=16words
  constant SizeOfHeader           : integer := 5;  --real time (3 words) + trigger count (2 words)
  constant WidthOfRealTime        : integer := 48;  --max length(@50MHz)=65days
  constant WidthOfTriggerBus      : integer := 8;  -- 8 lines

  --trigger mode
  constant Mode_1_StartingTh_NumberOfSamples            : std_logic_vector(WidthOfTriggerMode-1 downto 0) := x"1";
  constant Mode_2_CommonGateIn_NumberOfSamples          : std_logic_vector(WidthOfTriggerMode-1 downto 0) := x"2";
  constant Mode_3_StartingTh_NumberOfSamples_ClosingTh  : std_logic_vector(WidthOfTriggerMode-1 downto 0) := x"3";
  constant Mode_4_DeltaAboveDelayedADC_NumberOfSamples  : std_logic_vector(WidthOfTriggerMode-1 downto 0) := x"4";
  constant Mode_5_CPUTrigger                            : std_logic_vector(WidthOfTriggerMode-1 downto 0) := x"5";
  constant Mode_8_TriggerBusSelectedOR                  : std_logic_vector(WidthOfTriggerMode-1 downto 0) := x"8";
  constant Mode_9_TriggerBusSelectedAND                 : std_logic_vector(WidthOfTriggerMode-1 downto 0) := x"9";

  type Signal_ChModule2InternalModule is record
    --ADC Module
    AdcPowerDown      : std_logic;
    --Trigger Module
    TriggerMode       : std_logic_vector(WidthOfTriggerMode-1 downto 0);
    CommonGateIn      : std_logic;
    CPUTrigger        : std_logic;
    TriggerBus        : std_logic_vector(WidthOfTriggerBus-1 downto 0);
    TriggerBusMask    : std_logic_vector(WidthOfTriggerBus-1 downto 0);
    -- FastVetoTrigger                          : std_logic;
    -- HitPatternTrigger        : std_logic;
    --
    DepthOfDelay      : std_logic_vector(WidthOfDepthOfDelay-1 downto 0);
    NumberOfSamples   : std_logic_vector(WidthOfNumberOfSamples-1 downto 0);
    ThresholdStarting : std_logic_vector(ADCResolution-1 downto 0);
    ThresholdClosing  : std_logic_vector(ADCResolution-1 downto 0);
    --
    SizeOfHeader      : std_logic_vector(WidthOfSizeOfHeader-1 downto 0);
    --
    TriggerCount      : std_logic_vector(31 downto 0);
    --
    RealTime          : std_logic_vector(WidthOfRealTime-1 downto 0);
    Veto              : std_logic;
  end record;

  constant WidthOfDepthOfFIFO : integer := 10;  --max depth=1024
  type     Signal_InternalModule2ChModule is record
    --
    TriggerOut      : std_logic;
    --To know the current usage of fifo
    DataCountOfFIFO : std_logic_vector(WidthOfDepthOfFIFO-1 downto 0);
  end record;

  ---------------------------------------------------
  --Signals: between Timer and ChMgr
  ---------------------------------------------------
  constant WidthOfLiveTime : integer := 32;
  --livetime counter is counted up every 10ms
  --round at 42949672.95 sec = 497 days
  type     Signal_LiveTimer2ChMgr is record
    Livetime : std_logic_vector(WidthOfLiveTime-1 downto 0);
    Done     : std_logic;
  end record;
  type Signal_ChMgr2LiveTimer is record
    Veto           : std_logic;
    PresetLivetime : std_logic_vector(WidthOfLiveTime-1 downto 0);
    Reset          : std_logic;
  end record;

  type Signal_LiveTimer2ChMgr_Vector is array (integer range <>) of Signal_LiveTimer2ChMgr;
  type Signal_ChMgr2LiveTimer_Vector is array (integer range <>) of Signal_ChMgr2LiveTimer;

  ---------------------------------------------------
  --Signals: between Timer and ChMgr
  ---------------------------------------------------
  constant WidthOfNumberOfEvent : integer := 32;
  --2^32 event made count dekiru
  type     Signal_EventCounter2ChMgr is record
    EventCounterVeto : std_logic;
    NumberOfEvent    : std_logic_vector(WidthOfNumberOfEvent-1 downto 0);
  end record;
  type Signal_ChMgr2EventCounter is record
    Veto                : std_logic;
    Trigger             : std_logic;
    PresetNumberOfEvent : std_logic_vector(WidthOfNumberOfEvent-1 downto 0);
    Reset               : std_logic;
  end record;

  type Signal_EventCounter2ChMgr_Vector is array (integer range <>) of Signal_EventCounter2ChMgr;
  type Signal_ChMgr2EventCounter_Vector is array (integer range <>) of Signal_ChMgr2EventCounter;


  ---------------------------------------------------
  --Signals: between ChMgr and ChModules
  ---------------------------------------------------
  type Signal_ChModule2ChMgr is record
    Veto    : std_logic;
    Trigger : std_logic;
  end record;
  type Signal_ChMgr2ChModule is record
    Realtime     : std_logic_vector(WidthOfRealTime-1 downto 0);
    Livetime     : std_logic_vector(WidthOfLiveTime-1 downto 0);
    CommonGateIn : std_logic;
    TriggerBus   : std_logic_vector(WidthOfTriggerBus-1 downto 0);
    Veto         : std_logic;
  end record;
  type Signal_ChModule2ChMgr_Vector is array (integer range <>) of Signal_ChModule2ChMgr;
  type Signal_ChMgr2ChModule_Vector is array (integer range <>) of Signal_ChMgr2ChModule;

  ---------------------------------------------------
  --Signals: between Consumer and ConsumerMgr
  ---------------------------------------------------
  type Signal_Consumer2ConsumerMgr is record
    EventReady  : std_logic;
    WriteEnable : std_logic;
    Data        : std_logic_vector(EventPackeBufferDataWidth-1 downto 0);
  end record;
  type Signal_ConsumerMgr2Consumer is record
    Grant                        : std_logic;
    GateSize_FastGate            : std_logic_vector(15 downto 0);
    GateSize_SlowGate            : std_logic_vector(15 downto 0);
    EventPacket_NumberOfWaveform : std_logic_vector(15 downto 0);
    NumberOf_BaselineSample      : std_logic_vector(15 downto 0);
  end record;
  type Signal_Consumer2ConsumerMgr_Vector is array (integer range <>) of Signal_Consumer2ConsumerMgr;
  type Signal_ConsumerMgr2Consumer_Vector is array (integer range <>) of Signal_ConsumerMgr2Consumer;

  ---------------------------------------------------
  --Signals: temporal
  ---------------------------------------------------
  type Data_Vector is array (integer range <>) of std_logic_vector(15 downto 0);
  type Signal_std_logic_vector8 is array (integer range <>) of std_logic_vector(7 downto 0);
  type ArrayOf_Signed8bitInteger is array (integer range <>) of integer range -128 to 127;

end UserModule_Library;
