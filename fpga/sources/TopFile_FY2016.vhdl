----------------------------------------------------------------------------------
-- Top-level file for GROWTH FPGA Board
----------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

library UNISIM;
use UNISIM.VComponents.all;

library work;
use work.iBus_Library.all;
use work.iBus_AddressMap.all;
use work.UserModule_Library.all;

library work;
use work.SpaceWireCODECIPPackage.all;
use work.RMAPTargetIPPackage.all;

library xil_defaultlib;
use xil_defaultlib.all;

entity GROWTH_FY2016_FPGA is
  port (
        CLKIN           : in    STD_LOGIC;
        nF_RESET        : in    STD_LOGIC;
        FPGA_LED        : out   STD_LOGIC_VECTOR(04 downto 01);
        RasPi_TXD       : in    STD_LOGIC;
        RasPi_RXD       : out   STD_LOGIC;
        RasPi_FPGA_GPIO : inout STD_LOGIC_VECTOR(03 downto 00);
        FPGA_GPIO       : inout STD_LOGIC_VECTOR(10 downto 00);
        SpW_LED_Red     : out   STD_LOGIC;
        SpW_LED_Green   : out   STD_LOGIC;
--        SpW_DOUT        : out   STD_LOGIC;
--        SpW_SOUT        : out   STD_LOGIC;
--        SpW_SIN         : in    STD_LOGIC;
--        SpW_DIN         : in    STD_LOGIC;
        -- ADC Chip1/2 Common Control
        AD_PDWN         : out   STD_LOGIC;
        AD_OEn          : out   STD_LOGIC;
        AD_SCLK         : out   STD_LOGIC;
        AD_SDIO         : inout STD_LOGIC;
        AD1_CSn         : out   STD_LOGIC;
        AD2_CSn         : out   STD_LOGIC;
        -- ADC Chip 1
        AD1_SYNC        : out   STD_LOGIC;
        AD1_CLK         : out   STD_LOGIC;
        AD1_DCOB        : in    STD_LOGIC;
        AD1_ORA         : in    STD_LOGIC;
        AD1_DA          : in    STD_LOGIC_VECTOR(11 downto 00);
        AD1_DCOA        : in    STD_LOGIC;
        AD1_ORB         : in    STD_LOGIC;
        AD1_DB          : in    STD_LOGIC_VECTOR(11 downto 00);
        -- ADC Chip 2
        AD2_SYNC        : out   STD_LOGIC;
        AD2_CLK         : out   STD_LOGIC;
        AD2_DCOA        : in    STD_LOGIC;
        AD2_ORA         : in    STD_LOGIC;
        AD2_DA          : in    STD_LOGIC_VECTOR(11 downto 00);
        AD2_DCOB        : in    STD_LOGIC;
        AD2_ORB         : in    STD_LOGIC;
        AD2_DB          : in    STD_LOGIC_VECTOR(11 downto 00);
        -- FT2232 Control
        FT2232_RESET    : in    STD_LOGIC;
        PWERENn         : in    STD_LOGIC;
        SUSPENDn        : in    STD_LOGIC;
        -- FT2232 Bank A - FT245 FIFO Sync mode
        ADBUS_D         : inout STD_LOGIC_VECTOR (07 downto 00);
        ACBUS0_RXFn     : in    STD_LOGIC;
        ACBUS1_TXEn     : in    STD_LOGIC;
        ACBUS2_RDn      : out   STD_LOGIC;
        ACBUS3_WRn      : out   STD_LOGIC;
        ACBUS4_SIWUA    : out   STD_LOGIC;
        ACBUS5_CLKOUT   : in    STD_LOGIC;
        ACBUS6_OEn      : out   STD_LOGIC;
        ACBUS7          : in    STD_LOGIC;
        -- FT2232 Bank B - RS232 mode
        BDBUS0_TXD      : in    STD_LOGIC;
        BDBUS1_RXD      : out   STD_LOGIC;
        BDBUS2_RTSn     : in    STD_LOGIC;
        BDBUS3_CTSn     : out   STD_LOGIC;
        BDBUS4_DTRn     : in    STD_LOGIC;
        BDBUS5_DSRn     : out   STD_LOGIC;
        BDBUS6_DCDn     : out   STD_LOGIC;
        BDBUS7_RIn      : out   STD_LOGIC;
        BCBUS0_TXDEN    : in    STD_LOGIC;
        BCBUS1          : in    STD_LOGIC;
        BCBUS2          : in    STD_LOGIC;
        BCBUS3_RXLEDn   : in    STD_LOGIC;
        BCBUS4_TXLEDn   : in    STD_LOGIC;
        BCBUS5          : in    STD_LOGIC;
        BCBUS6          : in    STD_LOGIC
    );
end GROWTH_FY2016_FPGA;

architecture Behavioral of GROWTH_FY2016_FPGA is

  ---------------------------------------------
  -- Reset
  ---------------------------------------------
  -- Active-high reset
  signal Reset : std_logic := '0';
  -- Active-low reset used in iBus and old UserModule modules
  signal GlobalReset : std_logic := '1';

  ---------------------------------------------
  --Clock
  ---------------------------------------------
  component clk_wiz_0
  port
   (-- Clock in ports
    clk_in1           : in     std_logic;
    -- Clock out ports
    clk_out1          : out    std_logic;
    clk_out2          : out    std_logic;
    clk_out3          : out    std_logic;
    -- Status and control signals
    reset             : in     std_logic;
    locked            : out    std_logic
   );
  end component;
  
  signal   Clock100MHz       : std_logic;
  signal   Clock50MHz        : std_logic;
  signal   Clock10MHz        : std_logic;
  signal   clockWizardLocked : std_logic;

  constant Count1sec     : integer                      := 1e8;
  signal   counter1sec   : integer range 0 to Count1sec := 0;
  constant Count10msec   : integer                      := 1e6;
  signal   counter10msec : integer range 0 to Count1sec := 0;

  ---------------------------------------------
  --LED
  ---------------------------------------------
  signal fpgaLED        : std_logic_vector(3 downto 0) := (others => '0');
  signal daughterLED    : std_logic_vector(3 downto 0) := (others => '0');
  signal led1sec        : std_logic := '0';
  signal led10msec      : std_logic := '0';
  signal iSpW_LED_Red   : std_logic := '0';
  signal iSpW_LED_Green : std_logic := '0';

  ---------------------------------------------
  --ADC
  ---------------------------------------------
  signal adcClock              : std_logic;
  signal adcClock_previous     : std_logic;
  signal adcData               : Vector12Bits(NADCChannels-1 downto 0);
  signal adcClockIn            : std_logic_vector(NADCChannels-1 downto 0);
  
  signal dividedADCClockA : std_logic := '0';
  signal dividedADCClockB : std_logic := '0';
  constant ADC_CLOCK_DIVIDE_COUNT_MAX : integer := 4; -- 10 MHz / (2*(4+1)) = 1 MHz
  signal adcClockDivideCounterA : integer range 0 to ADC_CLOCK_DIVIDE_COUNT_MAX := 0;
  signal adcClockDivideCounterB : integer range 0 to ADC_CLOCK_DIVIDE_COUNT_MAX := 0;

  constant Count1secAtADCClock : integer                                := 200000000;
  signal   adcClockCounter     : integer range 0 to Count1secAtADCClock := 0;

  -- ADC's SENSE pin is connected to GND.
  -- This results in "Vref=1.0" and span = 2 * Vref = 2.0V.

  -- Used to distribute ADn_DCOA / ADn_DCOB
  component BUFG
    port(I : in std_logic; O : out std_logic);
  end component;

  ---------------------------------------------
  -- Channel Manager
  ---------------------------------------------
  component UserModule_ChannelManager is
    generic(
      InitialAddress : std_logic_vector(15 downto 0);
      FinalAddress   : std_logic_vector(15 downto 0)
      );
    port(
        -- Signals connected to BusController
        BusIF2BusController        : out iBus_Signals_BusIF2BusController;
        BusController2BusIF        : in  iBus_Signals_BusController2BusIF;
        -- Ch mgr(time, veto, ...)
        ChMgr2ChModule_vector      : out Signal_ChMgr2ChModule_Vector(NumberOfProducerNodes-1 downto 0);
        ChModule2ChMgr_vector      : in  Signal_ChModule2ChMgr_Vector(NumberOfProducerNodes-1 downto 0);
        -- Control
        CommonGateIn               : in  std_logic;
        MeasurementStarted         : out std_logic;
        -- ADC Clock
        ADCClockFrequencySelection : out adcClockFrequencies;
        -- Clock and reset
        Clock                      : in  std_logic;
        GlobalReset                : in  std_logic;
        ResetOut                   : out std_logic  -- 0=reset, 1=no reset
      );
  end component;
  
  signal MeasurementStarted : std_logic := '0';
  
  ---------------------------------------------
  --Channel Module
  ---------------------------------------------
  component UserModule_ChannelModule is
    generic(
      InitialAddress : std_logic_vector(15 downto 0);
      FinalAddress   : std_logic_vector(15 downto 0);
      ChNumber       : std_logic_vector(2 downto 0) := (others => '0')
      );
    port(
      --signals connected to BusController
      BusIF2BusController  : out iBus_Signals_BusIF2BusController;
      BusController2BusIF  : in  iBus_Signals_BusController2BusIF;
      --adc signals
      AdcDataIn            : in  std_logic_vector(ADCResolution-1 downto 0);
      AdcClockIn           : in  std_logic;
      --ch mgr(time, veto, ...)
      ChModule2ChMgr       : out Signal_ChModule2ChMgr;
      ChMgr2ChModule       : in  Signal_ChMgr2ChModule;
      --consumer mgr
      Consumer2ConsumerMgr : out Signal_Consumer2ConsumerMgr;
      ConsumerMgr2Consumer : in  Signal_ConsumerMgr2Consumer;
      --debug
      Debug                : out std_logic_vector(7 downto 0);
      --clock and reset
      ReadClock            : in  std_logic;
      GlobalReset          : in  std_logic
      );
  end component;

  constant ChannelModuleInitialAddresses : iBusAddresses(NADCChannels-1 downto 0) :=
    (0 => InitialAddressOf_ChModule_0, 
     1 => InitialAddressOf_ChModule_1,
     2 => InitialAddressOf_ChModule_2,
     3 => InitialAddressOf_ChModule_3);
  constant ChannelModuleFinalAddresses : iBusAddresses(NADCChannels-1 downto 0) :=
    (0 => FinalAddressOf_ChModule_0,
     1 => FinalAddressOf_ChModule_1,
     2 => FinalAddressOf_ChModule_2,
     3 => FinalAddressOf_ChModule_3);

  -- See UserModule_Library.vhdl for details of the following signal types
  signal ChModule2ChMgr       : Signal_ChModule2ChMgr_vector(NADCChannels-1 downto 0);
  signal ChMgr2ChModule       : Signal_ChMgr2ChModule_vector(NADCChannels-1 downto 0);
  signal Consumer2ConsumerMgr : Signal_Consumer2ConsumerMgr_vector(NADCChannels-1 downto 0);
  signal ConsumerMgr2Consumer : Signal_ConsumerMgr2Consumer_vector(NADCChannels-1 downto 0);

  ---------------------------------------------
  -- Consumer Manager
  ---------------------------------------------
  signal EventFIFOWriteData              : std_logic_vector(15 downto 0);
  signal EventFIFOWriteEnable            : std_logic;
  signal EventFIFOFull                   : std_logic;
  signal EventFIFOReadData               : std_logic_vector(15 downto 0);
  signal EventFIFOReadEnable             : std_logic;
  signal EventFIFOEmpty                  : std_logic;
  signal EventFIFODataCount              : std_logic_vector(13 downto 0);
  signal EventFIFOReset                  : std_logic := '0';
  signal EventFIFOReset_from_ConsumerMgr : std_logic := '0';

  type   EventFIFOReadStates is (
    Initialization, Idle, Read1, Read2,
    Read_gpsDataFIFO_1, Read_gpsDataFIFO_2, Read_gpsDataFIFO_3,
    Ack, Finalize);
  signal EventFIFOReadState : EventFIFOReadStates := Initialization;

  ---------------------------------------------
  -- Registers accesible via RMAP (non-iBus registers)
  ---------------------------------------------
  constant AddressOfEventFIFODataCountRegister : std_logic_vector(31 downto 0) := x"20000000";
  constant AddressOfGPSTimeRegister            : std_logic_vector(31 downto 0) := x"20000002";
  constant GPSRegisterLengthInBytes            : integer                       := 20;
  constant AddressOfFPGATypeRegister_L         : std_logic_vector(31 downto 0) := x"30000000";
  constant AddressOfFPGATypeRegister_H         : std_logic_vector(31 downto 0) := x"30000002";
  constant AddressOfFPGAVersionRegister_L      : std_logic_vector(31 downto 0) := x"30000004";
  constant AddressOfFPGAVersionRegister_H      : std_logic_vector(31 downto 0) := x"30000006";

  ---------------------------------------------
  -- GPS related
  ---------------------------------------------
  -- 96 bits = 12 bytes
  signal gpsYYMMDDHHMMSS_latched             : std_logic_vector(95 downto 0)                := (others => '0');
  -- 48 bits = 6 bytes
  signal fpgaRealtime_latched                : std_logic_vector(WidthOfRealTime-1 downto 0) := (others => '0');
  -- (12 + 6 + 2)=20 bytes = 160 bits = 10 16-bit words
  signal GPSTimeTableRegister                : std_logic_vector(143 downto 0)               := (others => '0');

  signal GPS_1PPS                     : std_logic := '0';
  signal gpsData                      : std_logic_vector(7 downto 0);
  signal gpsDataEnable                : std_logic;
  signal gpsDDMMYY                    : std_logic_vector(47 downto 0);
  signal gpsHHMMSS_SSS                : std_logic_vector(71 downto 0);
  signal gpsDateTimeUpdatedSingleShot : std_logic;
  signal gps1PPSSingleShot            : std_logic;
  signal gpsLED                       : std_logic := '0';

  ---------------------------------------------
  -- UART / FT2232 FT245 Mode
  ---------------------------------------------
  component UARTInterface is
    generic(
      InputClockPeriodInNanoSec : integer := 20;     -- ns
      BaudRate                  : integer := 115200  -- bps
      );
    port(
      Clock     : in  std_logic;  -- Clock input (tx/rx clocks will be internally generated)
      Reset     : in  std_logic;        -- Set '1' to reset this module
      TxSerial  : out std_logic;        -- Serial Tx output
      RxSerial  : in  std_logic;        -- Serial Rx input
      txDataIn  : in  std_logic_vector(7 downto 0);  -- Send data
      rxDataOut : out std_logic_vector(7 downto 0);  -- Received data
      txEnable  : in  std_logic;        -- Set '1' to send data in DataIn
      received  : out std_logic;        -- '1' when new DataOut is valid
      txReady   : out std_logic         -- '1' when Tx is not busy
      );
  end component;

  component FT245AsynchronousFIFO is
    port(
      -- Signals connected to FT245 (FT2232H)
      DATA     : inout std_logic_vector(7 downto 0);
      nRXF     : in    std_logic;
      nTXE     : in    std_logic;
      nRD      : out   std_logic;
      nWR      : out   std_logic;
      -- FT245 => FPGA
      rx_data  : out   std_logic_vector(7 downto 0);
      rx_valid : out   std_logic;
      rx_full  : in    std_logic;
      -- FPGA => FT245
      tx_data  : in    std_logic_vector(7 downto 0);
      tx_enable: in    std_logic;
      tx_done  : out   std_logic;
      tx_ready : out   std_logic;
      -- Cloc and reset
      clock    : in    std_logic;
      reset    : in    std_logic
    );          
  end component;

  ---------------------------------------------
  -- internal Bus (iBus)
  ---------------------------------------------
  component iBus_RMAPConnector is
    generic(
      InitialAddress : std_logic_vector(15 downto 0);
      FinalAddress   : std_logic_vector(15 downto 0)
      );
    port(
      --connected to BusController
      BusIF2BusController         : out iBus_Signals_BusIF2BusController;
      BusController2BusIF         : in  iBus_Signals_BusController2BusIF;
      --RMAP bus signals
      rmapBusMasterCycleOut       : in  std_logic;
      rmapBusMasterStrobeOut      : in  std_logic;
      rmapBusMasterAddressOut     : in  std_logic_vector(31 downto 0);
      rmapBusMasterByteEnableOut  : in  std_logic_vector( 1 downto 0);
      rmapBusMasterDataIn         : out std_logic_vector(15 downto 0);
      rmapBusMasterDataOut        : in  std_logic_vector(15 downto 0);
      rmapBusMasterWriteEnableOut : in  std_logic;
      rmapBusMasterReadEnableOut  : in  std_logic;
      rmapBusMasterAcknowledgeIn  : out std_logic;
      rmapBusMasterTimeOutErrorIn : out std_logic;
      --debug
      rmapProcessStateInteger     : out integer range 0 to 7;
      --clock and reset
      Clock                       : in  std_logic;
      GlobalReset                 : in  std_logic
      );
  end component;
  constant iBusNumberofNodes : integer := 7;
  signal BusIF2BusController : ibus_signals_busif2buscontroller_vector(iBusNumberofNodes-1 downto 0);
  signal BusController2BusIF : ibus_signals_buscontroller2busif_vector(iBusNumberofNodes-1 downto 0);

  ---------------------------------------------
  -- SocketVHDL-RMAP
  ---------------------------------------------
  component SSDTP2ToRMAPTargetBridge is
    generic (
      gBusWidth            : integer range 8 to 32 := 32;  -- 8 = 8bit, 16 = 16bit, 32 = 32bit
      bufferDataCountWidth : integer               := 10
      );  
    port(
      -- clock and reset
      clock                        : in  std_logic;
      reset                        : in  std_logic;
      ---------------------------------------------
      -- SocketVHDL signals
      ---------------------------------------------
      tcpSendFIFOData              : out std_logic_vector(7 downto 0);
      tcpSendFIFOWriteEnable       : out std_logic;
      tcpSendFIFOFull              : in  std_logic;
      tcpReceiveFIFOEmpty          : in  std_logic;
      tcpReceiveFIFOData           : in  std_logic_vector(7 downto 0);
      tcpReceiveFIFODataCount      : in  std_logic_vector(bufferDataCountWidth-1 downto 0);
      tcpReceiveFIFOReadEnable     : out std_logic;
      ---------------------------------------------
      -- RMAPTarget signals 
      ---------------------------------------------
      --Internal BUS 
      busMasterCycleOut            : out std_logic;
      busMasterStrobeOut           : out std_logic;
      busMasterAddressOut          : out std_logic_vector (31 downto 0);
      busMasterByteEnableOut       : out std_logic_vector ((gBusWidth/8)-1 downto 0);
      busMasterDataIn              : in  std_logic_vector (gBusWidth-1 downto 0);
      busMasterDataOut             : out std_logic_vector (gBusWidth-1 downto 0);
      busMasterWriteEnableOut      : out std_logic;
      busMasterReadEnableOut       : out std_logic;
      busMasterAcknowledgeIn       : in  std_logic;
      busMasterTimeOutErrorIn      : in  std_logic;
      -- RMAP Statemachine state                                     
      commandStateOut              : out commandStateMachine;
      replyStateOut                : out replyStateMachine;
      -- RMAP_User_Decode
      rmapLogicalAddressOut        : out std_logic_vector(7 downto 0);
      rmapCommandOut               : out std_logic_vector(3 downto 0);
      rmapKeyOut                   : out std_logic_vector(7 downto 0);
      rmapAddressOut               : out std_logic_vector(31 downto 0);
      rmapDataLengthOut            : out std_logic_vector(23 downto 0);
      requestAuthorization         : out std_logic;
      authorizeIn                  : in  std_logic;
      rejectIn                     : in  std_logic;
      replyStatusIn                : in  std_logic_vector(7 downto 0);
      -- RMAP Error Code and Status
      rmapErrorCode                : out std_logic_vector(7 downto 0);
      -- SSDTP2 state out
      stateOutSSDTP2TCPToSpaceWire : out std_logic_vector(7 downto 0);
      stateOutSSDTP2SpaceWireToTCP : out std_logic_vector(7 downto 0);
      -- statistics                                    
      statisticalInformationClear  : in  std_logic;
      statisticalInformation       : out bit32X8Array
      );
  end component;

  type   RMAPAccessModeType is (RMAPAccessMode_iBus, RMAPAccessMode_EventFIFO);
  signal RMAPAccessMode : RMAPAccessModeType := RMAPAccessMode_iBus;

  constant RMAPTargetLogicalAddress : std_logic_vector(7 downto 0) := x"FE";
  constant RMAPTargetKey            : std_logic_vector(7 downto 0) := x"00";
  constant RMAPTargetCRCRevision    : std_logic                    := '1';  -- RMAP Draft F version

  ---------------------------------------------
  -- RMAP over UART
  ---------------------------------------------
  constant uartRMAPBusWidth                       : integer                                            := 16;
  signal   uartRMAPBusMasterCycleOut              : std_logic                                          := '0';
  signal   uartRMAPBusMasterStrobeOut             : std_logic                                          := '0';
  signal   uartRMAPBusMasterAddressOut            : std_logic_vector (31 downto 0)                     := (others => '0');
  signal   uartRMAPBusMasterByteEnableOut         : std_logic_vector ((uartRMAPBusWidth/8)-1 downto 0) := (others => '0');
  signal   uartRMAPBusMasterDataIn                : std_logic_vector (uartRMAPBusWidth-1 downto 0)     := (others => '0');
  signal   uartRMAPBusMasterDataIn_iBus           : std_logic_vector (uartRMAPBusWidth-1 downto 0)     := (others => '0');
  signal   uartRMAPBusMasterDataIn_EventFIFO      : std_logic_vector (uartRMAPBusWidth-1 downto 0)     := (others => '0');
  signal   uartRMAPBusMasterDataOut               : std_logic_vector (uartRMAPBusWidth-1 downto 0)     := (others => '0');
  signal   uartRMAPBusMasterWriteEnableOut        : std_logic                                          := '0';
  signal   uartRMAPBusMasterReadEnable            : std_logic                                          := '0';
  signal   uartRMAPBusMasterReadEnable_iBus       : std_logic                                          := '0';
  signal   uartRMAPBusMasterReadEnable_EventFIFO  : std_logic                                          := '0';
  signal   uartRMAPBusMasterAcknowledge           : std_logic                                          := '0';
  signal   uartRMAPBusMasterAcknowledge_iBus      : std_logic                                          := '0';
  signal   uartRMAPBusMasterAcknowledge_EventFIFO : std_logic                                          := '0';
  signal   uartRMAPBusMasterTimeOutErrorIn        : std_logic                                          := '0';
  signal   uartRMAPProcessStateInteger            : integer range 0 to 7;
  signal   uartRMAPProcessStateIntegerPrevious    : integer range 0 to 7;
  signal   uartRMAPCommandStateOut                : commandstatemachine;
  signal   uartRMAPReplyStateOut                  : replystatemachine;
  signal   uartRMAPCommandStateOutAscii           : std_logic_vector(7 downto 0)                       := (others => '0');
  signal   uartRMAPReplyStateOutAscii             : std_logic_vector(7 downto 0)                       := (others => '0');
  signal   uartRMAPLogicalAddressOut              : std_logic_vector(7 downto 0)                       := (others => '0');
  signal   uartRMAPCommandOut                     : std_logic_vector(3 downto 0)                       := (others => '0');
  signal   uartRMAPKeyOut                         : std_logic_vector(7 downto 0)                       := (others => '0');
  signal   uartRMAPAddressOut                     : std_logic_vector(31 downto 0)                      := (others => '0');
  signal   uartRMAPDataLengthOut                  : std_logic_vector(23 downto 0)                      := (others => '0');
  signal   uartRMAPRequestAuthorization           : std_logic                                          := '0';
  signal   uartRMAPAuthorizeIn                    : std_logic                                          := '0';
  signal   uartRMAPRejectIn                       : std_logic                                          := '0';
  signal   uartRMAPReplyStatusIn                  : std_logic_vector(7 downto 0)                       := (others => '0');
  signal   uartRMAPErrorCode                      : std_logic_vector(7 downto 0)                       := (others => '0');
  signal   stateOutSSDTP2TCPToSpaceWire           : std_logic_vector(7 downto 0)                       := (others => '0');
  signal   stateOutSSDTP2SpaceWireToTCP           : std_logic_vector(7 downto 0)                       := (others => '0');
  signal   uartRMAPStatisticalInformationClear    : std_logic                                          := '0';
  signal   uartRMAPStatisticalInformation         : bit32x8array;

  ---------------------------------------------
  -- UART (Raspberry Pi / FT232)
  ---------------------------------------------
  signal FT232_RX_FPGA_TX     : std_logic := '0';
  signal FT232_TX_FPGA_RX     : std_logic := '0';
  signal GPS_RX_FPGA_TX       : std_logic := '0';
  signal GPS_TX_FPGA_RX       : std_logic := '0';

  constant ClockPeriodInNanoSec_for_UART : integer := 10;  --10ns for Clock100MHz
  constant BaudRate_FT232                : integer := 230400;
  constant BaudRate_GPS                  : integer := 9600;

  -- FT245 (FT2232 Bank A)
  signal ft245TxData        : std_logic_vector(7 downto 0) := (others => '0');
  signal ft245RxData        : std_logic_vector(7 downto 0) := (others => '0');
  signal ft245RxDataLatched : std_logic_vector(7 downto 0) := (others => '0');
  signal ft245TxEnable      : std_logic                    := '0';
  signal ft245TxDone        : std_logic                    := '0';
  signal ft245Received      : std_logic                    := '0';
  signal ft245TxReady       : std_logic                    := '0';

  signal ft245ReceiveFIFOWriteData   : std_logic_vector(7 downto 0) := (others => '0');
  signal ft245ReceiveFIFOWriteEnable : std_logic                    := '0';
  signal ft245ReceiveFIFOReadEnable  : std_logic                    := '0';
  signal ft245ReceiveFIFOReadData    : std_logic_vector(7 downto 0) := (others => '0');
  signal ft245ReceiveFIFOFull        : std_logic                    := '0';
  signal ft245ReceiveFIFOEmpty       : std_logic                    := '0';
  signal ft245ReceiveFIFODataCount   : std_logic_vector(9 downto 0) := (others => '0');

  signal ft245SendFIFOClock       : std_logic;
  signal ft245SendFIFOReset       : std_logic;
  signal ft245SendFIFODataIn      : std_logic_vector(7 downto 0);
  signal ft245SendFIFOWriteEnable : std_logic;
  signal ft245SendFIFOReadEnable  : std_logic;
  signal ft245SendFIFODataOut     : std_logic_vector(7 downto 0);
  signal ft245SendFIFOFull        : std_logic;
  signal ft245SendFIFOEmpty       : std_logic;
  signal ft245SendFIFODataCount   : std_logic_vector(9 downto 0);

  ---------------------------------------------
  -- FIFO
  ---------------------------------------------
  component fifo8x1k
    port (
      clk        : IN  STD_LOGIC;
      rst        : IN  STD_LOGIC;
      din        : IN  STD_LOGIC_VECTOR(7 DOWNTO 0);
      wr_en      : IN  STD_LOGIC;
      rd_en      : IN  STD_LOGIC;
      dout       : OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
      full       : OUT STD_LOGIC;
      empty      : OUT STD_LOGIC;
      data_count : OUT STD_LOGIC_VECTOR(9 DOWNTO 0)
    );
  end component;

  component EventFIFO
    port (
      clk        : IN  STD_LOGIC;
      rst        : IN  STD_LOGIC;
      din        : IN  STD_LOGIC_VECTOR(15 DOWNTO 0);
      wr_en      : IN  STD_LOGIC;
      rd_en      : IN  STD_LOGIC;
      dout       : OUT STD_LOGIC_VECTOR(15 DOWNTO 0);
      full       : OUT STD_LOGIC;
      empty      : OUT STD_LOGIC;
      data_count : OUT STD_LOGIC_VECTOR(13 DOWNTO 0)
    );
  end component;
  signal eventFIFODataSendState : integer range 0 to 255 := 0;

  -- Version register
  constant FPGAType    : std_logic_vector(31 downto 0) := x"20160110";
  constant FPGAVersion : std_logic_vector(31 downto 0) := x"20171031";

  --GPS data FIFO
  signal gpsDataFIFOReset                  : std_logic := '0';
  signal gpsDataFIFODataIn                 : std_logic_vector(7 downto 0);
  signal gpsDataFIFOWriteEnable            : std_logic;
  signal gpsDataFIFOReadEnable             : std_logic;
  signal gpsDataFIFODataOut                : std_logic_vector(7 downto 0);
  signal gpsDataFIFOFull                   : std_logic;
  signal gpsDataFIFOEmpty                  : std_logic;
  signal gpsDataFIFODataCount              : std_logic_vector(9 downto 0);
  signal AddressOfGPSDataFIFOResetRegister : std_logic_vector(31 downto 0) := x"20001000";
  signal InitialAddressOfGPSDataFIFO       : std_logic_vector(31 downto 0) := x"20001002";
  signal FinalAddressOfGPSDataFIFO         : std_logic_vector(31 downto 0) := x"20001FFF";

  -- For ILA
--  attribute mark_debug : string;
--  attribute keep : string;
--  attribute mark_debug of uartRMAPBusMasterReadEnable  : signal is "true";
--  attribute mark_debug of uartRMAPBusMasterWriteEnableOut  : signal is "true";
--  attribute mark_debug of uartRMAPBusMasterTimeOutErrorIn  : signal is "true";
--  attribute mark_debug of uartRMAPBusMasterAddressOut  : signal is "true";
--  attribute mark_debug of uartRMAPErrorCode  : signal is "true";

begin
  -----------------------------------------------
  ---- Reset
  -----------------------------------------------
  Reset       <= not nF_RESET;
  GlobalReset <= nF_RESET;

  -----------------------------------------------
  ---- ADC
  -----------------------------------------------
  
  -- DEBUG (to reduce power)
  AD_PDWN <= '0' when MeasurementStarted='1' else '1';
  
  -- Configure ADC
  AD1_CSn  <= '1'; -- Disable SPI
  AD2_CSn  <= '1'; -- Disable SPI
  AD_SDIO  <= '1'; -- Duty cycle stabilizer enabled
  AD_SCLK  <= '0'; -- Output format = Offset binary
  AD_OEn   <= '0';  -- Output enabled
  -- ADC clock output
  adcClock <= Clock50MHz; -- 50MHz
  AD1_CLK  <= adcClock;
  AD2_CLK <= adcClock;
  -- ADC clock input (used to sample ADC data)
  bufgADCCh0 : BUFG
  port map (
    I => AD1_DCOA,
    O => adcClockIn(0)
  );
  bufgADCCh1 : BUFG
  port map (
    I => AD1_DCOB,
    O => adcClockIn(1)
  );
  
  process(AD2_DCOA, Reset)
  begin
    if(rising_edge(AD2_DCOA))then
      if(adcClockDivideCounterA=ADC_CLOCK_DIVIDE_COUNT_MAX)then
        dividedADCClockA <= not dividedADCClockA;
        adcClockDivideCounterA <= 0;
      else
        adcClockDivideCounterA <= adcClockDivideCounterA + 1;
      end if;
    end if;
  end process;

  process(AD2_DCOB, Reset)
  begin
    if(rising_edge(AD2_DCOB))then
      if(adcClockDivideCounterB=ADC_CLOCK_DIVIDE_COUNT_MAX)then
        dividedADCClockB <= not dividedADCClockB;
        adcClockDivideCounterB <= 0;
      else
        adcClockDivideCounterB <= adcClockDivideCounterB + 1;
      end if;
    end if;
  end process;

  bufgADCCh2 : BUFG
  port map (
    -- I => dividedADCClockA,
    I => AD2_DCOA,
    O => adcClockIn(2)
  );
  bufgADCCh3 : BUFG
  port map (
    -- I => dividedADCClockB,
    I => AD2_DCOB,
    O => adcClockIn(3)
  );

  -----------------------------------------------
  ---- GPIO
  -----------------------------------------------
  RasPi_FPGA_GPIO        <= (others => 'Z');
  FPGA_GPIO(0)           <= 'Z'; -- GPS 1PPS
  FPGA_GPIO(1)           <= GPS_Rx_FPGA_Tx;
  FPGA_GPIO(2)           <= 'Z'; -- GPS_Tx_FPGA_Rx
  FPGA_GPIO(3)           <= daughterLED(1);
  FPGA_GPIO(4)           <= daughterLED(0);
  FPGA_GPIO(5)           <= daughterLED(3);
  FPGA_GPIO(6)           <= daughterLED(2);
  FPGA_GPIO(10 downto 7) <= (10=>led1sec, 9=>led10msec, 8=>dividedADCClockB, 7=>dividedADCClockA);
  
  -----------------------------------------------
  ---- LED
  -----------------------------------------------
  FPGA_LED      <= not fpgaLED;
  SpW_LED_Red   <= not iSpW_LED_Red;
  SpW_LED_Green <= not iSpW_LED_Green;

  process(Clock100MHz, Reset)
  begin
    if(rising_edge(Clock100MHz))then
      if(counter1sec = Count1sec)then
        counter1sec <= 0;
        led1sec     <= '1';
      else
        counter1sec <= counter1sec + 1;
        led1sec     <= '0';
      end if;
    end if;
  end process;

  process(Clock100MHz, Reset)
  begin
    if(rising_edge(Clock100MHz))then
      if(counter10msec = Count10msec)then
        counter10msec <= 0;
        led10msec     <= '1';
      else
        counter10msec <= counter10msec + 1;
        led10msec     <= '0';
      end if;
    end if;
  end process;

  -- Shows FT2232 Channel B (UART) access status
  led_spw_red : entity work.Blinker
    generic map(
      LedBlinkDuration => 3000000      -- 30ms = 10ns * 3000000
      )
    port map(
      clock     => Clock100MHz,
      reset     => Reset,
      triggerIn => ft245TxEnable,
      blinkOut  => iSpW_LED_Red
      );
  
  -- Shows FT2232 Channel A (FT245 FIFO mode) access status
  led_spw_green : entity work.Blinker
    generic map(
      LedBlinkDuration => 3000000      -- 30ms = 10ns * 3000000
      )
    port map(
      clock     => Clock100MHz,
      reset     => Reset,
      triggerIn => ft245Received,
      blinkOut  => iSpW_LED_Green
      );

  -- Shows 1 sec counted by FPGA clock
  led1 : entity work.Blinker
        generic map(
          LedBlinkDuration => 3000000      -- 30ms = 10ns * 3000000
          )
        port map(
          clock     => Clock100MHz,
          reset     => Reset,
          triggerIn => led1sec,
          blinkOut  => fpgaLED(0)
          );
  
  -- Shows GPS 1PPS
  led2 : entity work.Blinker
    generic map(
      LedBlinkDuration => 3000000      -- 30ms = 10ns * 3000000
      )
    port map(
      clock     => Clock100MHz,
      reset     => Reset,
      triggerIn => gps1PPSSingleShot,
      blinkOut  => fpgaLED(1)
      );

  -- Shows GPS Message Update
  led3 : entity work.Blinker
    generic map(
      LedBlinkDuration => 3000000      -- 30ms = 10ns * 3000000
      )
    port map(
      clock     => Clock100MHz,
      reset     => Reset,
      triggerIn => gpsDateTimeUpdatedSingleShot,
      blinkOut  => fpgaLED(2)
      );

  -- Shows time out error of the internal bus (iBus)
  led4 : entity work.Blinker
    generic map(
      LedBlinkDuration => 3000000      -- 30ms = 10ns * 3000000
      )
    port map(
      clock     => Clock100MHz,
      reset     => Reset,
      triggerIn => uartRMAPBusMasterTimeOutErrorIn,
      blinkOut  => fpgaLED(3)
      );

  -- Trigger LED (on Daughter Borad)
  TriggerLEDs : for i in 0 to 3 generate
    led4 : entity work.Blinker
      generic map(
        LedBlinkDuration => 3000000      -- 30ms = 10ns * 3000000
        )
      port map(
        clock     => Clock100MHz,
        reset     => Reset,
        triggerIn => ChModule2ChMgr(i).Trigger,
        blinkOut  => daughterLED(i)
        );
  end generate TriggerLEDs;

  ---------------------------------------------
  -- Process
  ---------------------------------------------

  -- Synchronization
  process(Clock100MHz, Reset)
  begin
    if(Reset = '1')then
    elsif(Clock100MHz = '1' and Clock100MHz'event)then
      GPS_1PPS       <= FPGA_GPIO(0);
      GPS_TX_FPGA_RX <= FPGA_GPIO(2);
    end if;
  end process;

  -- For FY2016
   adcData(0) <= AD1_DA;
   adcData(1) <= AD1_DB;
   adcData(2) <= AD2_DA;
   adcData(3) <= AD2_DB;

--  -- TODO: Changed for GROWTH-FY2016 large energy deposite investigation
--  -- 2017-04-06
--  adcData(0) <= AD1_DA;
--  adcData(1) <= x"FFF" - AD1_DB;
--  adcData(2) <= AD2_DA;
--  adcData(3) <= x"FFF" - AD2_DB;

  ---------------------------------------------
  -- Clocking Wizard
  ---------------------------------------------
  clWiz : clk_wiz_0
     port map ( 
     -- Clock in ports
     clk_in1  => CLKIN, -- 50MHz clock input
    -- Clock out ports  
     clk_out1 => Clock100MHz, -- 100MHz clock output
     clk_out2 => Clock50MHz,  -- 50MHz clock output used as ADCClock
     clk_out3 => Clock10MHz,
    -- Status and control signals                
     Reset    => Reset,
     locked   => clockWizardLocked
   );

  ---------------------------------------------
  -- UART (FT232)
  ---------------------------------------------
  
  -- Synchronization
  process(Clock100MHz, Reset)
  begin
    if(Reset = '1')then
    elsif(Clock100MHz = '1' and Clock100MHz'event)then
      FT232_Tx_FPGA_Rx <= BDBUS0_TXD;
    end if;
  end process;
  
  -- Output signals
  BDBUS1_RXD  <= FT232_Rx_FPGA_Tx;
  BDBUS3_CTSn <= BDBUS2_RTSn;
  BDBUS5_DSRn <= BDBUS4_DTRn;
  BDBUS6_DCDn <= '0'; -- TODO: check if this is right
  BDBUS7_RIn  <= '0';  -- TODO: check if this is right
  -- [not used] BCBUS0_TXDEN

  -- Connect GPS serial output to FT2232 Bank B (FT232 port)
  FT232_Rx_FPGA_Tx <= GPS_TX_FPGA_RX;
  GPS_Rx_FPGA_Tx   <= FT232_Tx_FPGA_Rx;

  --GPS Data FIFO
  gpsDataFIFO : fifo8x1k
    port map (
      clk        => Clock100MHz,
      rst        => gpsDataFIFOReset,
      din        => gpsDataFIFODataIn,
      wr_en      => gpsDataFIFOWriteEnable,
      rd_en      => gpsDataFIFOReadEnable,
      dout       => gpsDataFIFODataOut,
      full       => gpsDataFIFOFull,
      empty      => gpsDataFIFOEmpty,
      data_count => gpsDataFIFODataCount
      );
  gpsDataFIFODataIn      <= gpsData;
  gpsDataFIFOWriteEnable <= gpsDataEnable;

  ---------------------------------------------
  -- FT2232 - FT245 Sync FIFO mode (Bank A) 
  ---------------------------------------------

  ft245module: FT245AsynchronousFIFO
  port map (
    -- Signals connected to FT245 (FT2232H)
    DATA     => ADBUS_D,
    nRXF     => ACBUS0_RXFn,
    nTXE     => ACBUS1_TXEn,
    nRD      => ACBUS2_RDn,
    nWR      => ACBUS3_WRn,
    -- FT245 => FPGA
    rx_data  => ft245RxData,
    rx_valid => ft245Received,
    rx_full  => ft245ReceiveFIFOFull,
    -- FPGA => FT245
    tx_data  => ft245TxData,
    tx_enable=> ft245TxEnable,
    tx_done  => ft245TxDone,
    tx_ready => ft245TxReady,
    -- Cloc and reset
    clock    => Clock100MHz,
    reset    => reset
  );
  ACBUS4_SIWUA <= '1';

  uartReceiveFIFO : fifo8x1k
    port map(
      clk        => Clock100MHz,
      rst        => Reset,
      din        => ft245ReceiveFIFOWriteData,
      wr_en      => ft245ReceiveFIFOWriteEnable,
      rd_en      => ft245ReceiveFIFOReadEnable,
      dout       => ft245ReceiveFIFOReadData,
      full       => ft245ReceiveFIFOFull,
      empty      => ft245ReceiveFIFOEmpty,
      data_count => ft245ReceiveFIFODataCount
      );

  uartSendFIFO : fifo8x1k
    port map (
      clk        => Clock100MHz,
      rst        => Reset,
      din        => ft245SendFIFODataIn,
      wr_en      => ft245SendFIFOWriteEnable,
      rd_en      => ft245SendFIFOReadEnable,
      dout       => ft245SendFIFODataOut,
      full       => ft245SendFIFOFull,
      empty      => ft245SendFIFOEmpty,
      data_count => ft245SendFIFODataCount
      );

  ft245ReceiveFIFOWriteData   <= ft245RxData;
  ft245ReceiveFIFOWriteEnable <= ft245Received;

  ---------------------------------------------
  -- Processes
  ---------------------------------------------
  process(Clock100MHz, Reset)
  begin
    if(rising_edge(Clock100MHz))then
      if(Reset = '1')then
        eventFIFODataSendState <= 0;
      else
        case eventFIFODataSendState is
          when 0 =>
            ft245TxEnable <= '0';
            if(ft245SendFIFOEmpty = '0' and ft245TxReady = '1')then  -- if not empty and Tx is ready
              ft245SendFIFOReadEnable <= '1';
              eventFIFODataSendState  <= 1;
            else
              ft245SendFIFOReadEnable <= '0';
            end if;
          when 1 =>
            ft245SendFIFOReadEnable <= '0';
            eventFIFODataSendState  <= 2;
          when 2 =>
            ft245TxData <= ft245SendFIFODataOut;
            if(ft245TxDone = '1')then
              ft245TxEnable          <= '0';
              eventFIFODataSendState <= 3;
            else
              ft245TxEnable <= '1';
            end if;
          when 3 =>
            ft245TxEnable <= '0';
            if(ft245TxReady = '1')then
              eventFIFODataSendState <= 0;
            end if;
          when others =>
            eventFIFODataSendState <= 0;
        end case;
      end if;
    end if;
  end process;

  -----------------------------------------------
  ---- UART (GPS)
  -----------------------------------------------
  uartGPS : entity work.GPSUARTInterface
    generic map(
      InputClockPeriodInNanoSec    => ClockPeriodInNanoSec_for_UART,  --ns
      BaudRate                     => BaudRate_GPS
      )
    port map(
      clock                        => Clock100MHz,
      reset                        => Reset,
      --from GPS
      gpsUARTIn                    => GPS_TX_FPGA_RX,
      gps1PPS                      => GPS_1PPS,
      --processed signals
      gpsReceivedByte              => gpsData,
      gpsReceivedByteEnable        => gpsDataEnable,
      gpsDDMMYY                    => gpsDDMMYY,
      gpsHHMMSS_SSS                => gpsHHMMSS_SSS,
      gpsDateTimeUpdatedSingleShot => gpsDateTimeUpdatedSingleShot,
      gps1PPSSingleShot            => gps1PPSSingleShot
      );

  ---------------------------------------------
  -- Channel Manager
  ---------------------------------------------
  chManager : UserModule_ChannelManager
    generic map(
      InitialAddress => InitialAddressOf_ChMgr,
      FinalAddress   => FinalAddressOf_ChMgr
      )
    port map(
      --signals connected to BusController
      BusIF2BusController        => BusIF2BusController(0),
      BusController2BusIF        => BusController2BusIF(0),
      --ch mgr(time, veto, ...)
      ChMgr2ChModule_vector      => ChMgr2ChModule,
      ChModule2ChMgr_vector      => ChModule2ChMgr,
      --control
      CommonGateIn               => '0',  -- todo: implement this
      --ADCClockSelection
      ADCClockFrequencySelection => open,
      MeasurementStarted         => MeasurementStarted, 
      --clock and reset
      Clock                      => Clock100MHz,
      GlobalReset                => GlobalReset,
      ResetOut                   => open
      );

  ---------------------------------------------
  -- Channel Module
  ---------------------------------------------  
  ChModules : for i in 0 to 3 generate
    instanceOfChannelModule0 : UserModule_ChannelModule
      generic map(
        InitialAddress => ChannelModuleInitialAddresses(i),
        FinalAddress   => ChannelModuleFinalAddresses(i),
        ChNumber       => conv_std_logic_vector(i, 3)
        )
      port map(
        BusIF2BusController  => BusIF2BusController(i+1),
        BusController2BusIF  => BusController2BusIF(i+1),
        AdcDataIn            => adcData(i),
        AdcClockIn           => adcClockIn(i),
        ChModule2ChMgr       => ChModule2ChMgr(i),
        ChMgr2ChModule       => ChMgr2ChModule(i),
        Consumer2ConsumerMgr => Consumer2ConsumerMgr(i),
        ConsumerMgr2Consumer => ConsumerMgr2Consumer(i),
        Debug                => open,
        ReadClock            => Clock100MHz,
        GlobalReset          => GlobalReset
        );
  end generate ChModules;

  ---------------------------------------------
  -- Consumer Manager
  ---------------------------------------------
  consumerManager : entity work.UserModule_ConsumerManager_EventFIFO
    generic map(
      bufferDataCountWidth => EventFIFODataCount'length,
      InitialAddress       => InitialAddressOf_ConsumerMgr,
      FinalAddress         => FinalAddressOf_ConsumerMgr
      )
    port map(
      --signals connected to BusController
      BusIF2BusController         => BusIF2BusController(5),
      BusController2BusIF         => BusController2BusIF(5),
      --signals connected to ConsumerModule
      Consumer2ConsumerMgr_vector => Consumer2ConsumerMgr,
      ConsumerMgr2Consumer_vector => ConsumerMgr2Consumer,
      -- SocketFIFO signals
      EventFIFOWriteData          => EventFIFOWriteData,
      EventFIFOWriteEnable        => EventFIFOWriteEnable,
      EventFIFOFull               => EventFIFOFull,
      EventFIFOReset              => EventFIFOReset_from_ConsumerMgr,
      --clock and reset
      Clock                       => Clock100MHz,
      GlobalReset                 => GlobalReset
      );

  ---------------------------------------------
  -- SocketVHDL-RMAP
  ---------------------------------------------
  ssdtp2RMAP : entity work.SSDTP2ToRMAPTargetBridge
    generic map(
      gBusWidth            => uartRMAPBusWidth,  --16 for iBus bridging, 32 for SDRAM-RMAP bridging
      bufferDataCountWidth => ft245ReceiveFIFODataCount'length
      )
    port map(
      clock                        => Clock100MHz,
      reset                        => Reset,
      -- TCP socket signals (tcp send)
      tcpSendFIFOData              => ft245SendFIFODataIn,
      tcpSendFIFOWriteEnable       => ft245SendFIFOWriteEnable,
      tcpSendFIFOFull              => ft245SendFIFOFull,
      -- TCP socket signals (tcp receive)
      tcpReceiveFIFOEmpty          => ft245ReceiveFIFOEmpty,
      tcpReceiveFIFOData           => ft245ReceiveFIFOReadData,
      tcpReceiveFIFODataCount      => ft245ReceiveFIFODataCount,
      tcpReceiveFIFOReadEnable     => ft245ReceiveFIFOReadEnable,
      -- RMAP Target signals (bus access)
      busMasterCycleOut            => uartRMAPBusMasterCycleOut,
      busMasterStrobeOut           => uartRMAPBusMasterStrobeOut,
      busMasterAddressOut          => uartRMAPBusMasterAddressOut,
      busMasterByteEnableOut       => uartRMAPBusMasterByteEnableOut,
      busMasterDataIn              => uartRMAPBusMasterDataIn,
      busMasterDataOut             => uartRMAPBusMasterDataOut,
      busMasterWriteEnableOut      => uartRMAPBusMasterWriteEnableOut,
      busMasterReadEnableOut       => uartRMAPBusMasterReadEnable,
      busMasterAcknowledgeIn       => uartRMAPBusMasterAcknowledge,
      busMasterTimeOutErrorIn      => uartRMAPBusMasterTimeOutErrorIn,
      -- RMAP Target signals (transaction control)
      commandStateOut              => uartRMAPCommandStateOut,
      replyStateOut                => uartRMAPReplyStateOut,
      rmapLogicalAddressOut        => uartRMAPLogicalAddressOut,
      rmapCommandOut               => uartRMAPCommandOut,
      rmapKeyOut                   => uartRMAPKeyOut,
      rmapAddressOut               => uartRMAPAddressOut,
      rmapDataLengthOut            => uartRMAPDataLengthOut,
      requestAuthorization         => uartRMAPRequestAuthorization,
      authorizeIn                  => uartRMAPAuthorizeIn,
      rejectIn                     => uartRMAPRejectIn,
      replyStatusIn                => uartRMAPReplyStatusIn,
      rmapErrorCode                => uartRMAPErrorCode,
      stateOutSSDTP2TCPToSpaceWire => stateOutSSDTP2TCPToSpaceWire,
      stateOutSSDTP2SpaceWireToTCP => stateOutSSDTP2SpaceWireToTCP,
      statisticalInformationClear  => uartRMAPStatisticalInformationClear,
      statisticalInformation       => uartRMAPStatisticalInformation
      );
  uartRMAPAuthorizeIn   <= '1' when uartRMAPRequestAuthorization = '1' else '0';
  uartRMAPRejectIn      <= '0';
  uartRMAPReplyStatusIn <= (others => '0');

  ---------------------------------------------
  -- iBus Controller
  ---------------------------------------------
  iBusController : entity work.iBus_BusController
    generic map(
      NumberOfNodes => iBusNumberofNodes
      )
    port map(
      BusIF2BusController => BusIF2BusController,
      BusController2BusIF => BusController2BusIF,
      Clock               => Clock100MHz,
      GlobalReset         => GlobalReset
      );

  --iBus Mapping
  -- 0   => Channel Manager
  -- 1-4 => Channel Module 0/1/2/3
  -- 5 => Consumer Manager
  -- 6 => iBus-RMAP bridge

  ---------------------------------------------
  -- iBus-RMAP bridge
  ---------------------------------------------  
  iBusRMAP : entity work.iBus_RMAPConnector
    generic map(
      InitialAddress => x"FFF0",  -- not used because no iBus module can access to RMAPConnector (i.e. this module acts as target)
      FinalAddress   => x"FFFF"   -- not used because no iBus module can access to RMAPConnector (i.e. this module acts as target)
      )
    port map(
      BusIF2BusController         => BusIF2BusController(6),
      BusController2BusIF         => BusController2BusIF(6),
      rmapBusMasterCycleOut       => uartRMAPBusMasterCycleOut,
      rmapBusMasterStrobeOut      => uartRMAPBusMasterStrobeOut,
      rmapBusMasterAddressOut     => uartRMAPBusMasterAddressOut,
      rmapBusMasterByteEnableOut  => uartRMAPBusMasterByteEnableOut,
      rmapBusMasterDataIn         => uartRMAPBusMasterDataIn_iBus,
      rmapBusMasterDataOut        => uartRMAPBusMasterDataOut,
      rmapBusMasterWriteEnableOut => uartRMAPBusMasterWriteEnableOut,
      rmapBusMasterReadEnableOut  => uartRMAPBusMasterReadEnable_iBus,
      rmapBusMasterAcknowledgeIn  => uartRMAPBusMasterAcknowledge_iBus,
      rmapBusMasterTimeOutErrorIn => uartRMAPBusMasterTimeOutErrorIn,
      rmapProcessStateInteger     => uartRMAPProcessStateInteger,

      Clock       => Clock100MHz,
      GlobalReset => GlobalReset
      );

  -- Multiplexing RMAP Read access
  --   Address 0x0000_xxxx = iBus address space
  --   Address 0x1000_xxxx = EventFIFO read data
  --   Address 0x2000_0000 = EventFIFO data size register
  RMAPAccessMode                        <= RMAPAccessMode_iBus               when --
    (uartRMAPBusMasterAddressOut(31 downto 16) = x"0000" or uartRMAPBusMasterAddressOut(31 downto 16) = x"0101") --
     else RMAPAccessMode_EventFIFO;
  uartRMAPBusMasterDataIn               <= uartRMAPBusMasterDataIn_iBus      when RMAPAccessMode = RMAPAccessMode_iBus                                                                       else uartRMAPBusMasterDataIn_EventFIFO;
  uartRMAPBusMasterReadEnable_iBus      <= uartRMAPBusMasterReadEnable       when RMAPAccessMode = RMAPAccessMode_iBus                                                                       else '0';
  uartRMAPBusMasterReadEnable_EventFIFO <= uartRMAPBusMasterReadEnable       when RMAPAccessMode = RMAPAccessMode_EventFIFO                                                                  else '0';
  uartRMAPBusMasterAcknowledge          <= uartRMAPBusMasterAcknowledge_iBus when RMAPAccessMode = RMAPAccessMode_iBus else uartRMAPBusMasterAcknowledge_EventFIFO;

  instEventFIFO : EventFIFO
    port map(
      rst        => EventFIFOReset,
      clk        => Clock100MHz,
      din        => EventFIFOWriteData,
      wr_en      => EventFIFOWriteEnable,
      rd_en      => EventFIFOReadEnable,
      dout       => EventFIFOReadData,
      full       => EventFIFOFull,
      empty      => EventFIFOEmpty,
      data_count => EventFIFODataCount
      );

  -- RMAP registers
  process(Clock100MHz, Reset)
  begin
    if(Reset = '1')then
      EventFIFOReadState <= Initialization;
    elsif(Clock100MHz = '1' and Clock100MHz'event)then
      EventFIFOReset     <= EventFIFOReset_from_ConsumerMgr or Reset;

      if(gps1PPSSingleShot = '1')then
        gpsYYMMDDHHMMSS_latched <=
          gpsDDMMYY(15 downto 0) & gpsDDMMYY(31 downto 16) & gpsDDMMYY(47 downto 32)
          & gpsHHMMSS_SSS(71 downto 24);
        fpgaRealtime_latched <= ChMgr2ChModule(0).Realtime;
      end if;

      -- EventFIFO readout process
      case EventFIFOReadState is
        when Initialization =>
          gpsDataFIFOReset      <= '0';
          gpsDataFIFOReadEnable <= '0';
          EventFIFOReadEnable   <= '0';
          EventFIFOReadState    <= Idle;
        when Idle =>
          if(uartRMAPBusMasterReadEnable_EventFIFO = '1')then
            ---------------------------------------------
            ---------------------------------------------
            if(-- Access to EventFIFO DataCount Register
              uartRMAPBusMasterAddressOut = AddressOfEventFIFODataCountRegister)then
              uartRMAPBusMasterDataIn_EventFIFO(15)          <= '0';
              uartRMAPBusMasterDataIn_EventFIFO(14)          <= EventFIFOFull;
              uartRMAPBusMasterDataIn_EventFIFO(13 downto 0) <= EventFIFODataCount;
              EventFIFOReadState                             <= Ack;
            elsif( -- Access to GPS Time Register
              AddressOfGPSTimeRegister        <= uartRMAPBusMasterAddressOut
              and uartRMAPBusMasterAddressOut <= AddressOfGPSTimeRegister+conv_std_logic_vector(GPSRegisterLengthInBytes,31)
              )then                     -- GPS register first word
              case conv_integer(uartRMAPBusMasterAddressOut(15 downto 0)-AddressOfGPSTimeRegister(15 downto 0)) is
                when 0 =>               -- Latch register
                  uartRMAPBusMasterDataIn_EventFIFO <= x"4750"; -- 'GP' in ASCII
                  GPSTimeTableRegister              <= gpsYYMMDDHHMMSS_latched & fpgaRealtime_latched;
                when 18 =>               -- 
                  uartRMAPBusMasterDataIn_EventFIFO <= GPSTimeTableRegister(15 downto 0);
                when 16 =>               -- 
                  uartRMAPBusMasterDataIn_EventFIFO <= GPSTimeTableRegister(31 downto 16);
                when 14 =>               -- 
                  uartRMAPBusMasterDataIn_EventFIFO <= GPSTimeTableRegister(47 downto 32);
                when 12 =>               -- 
                  uartRMAPBusMasterDataIn_EventFIFO <= GPSTimeTableRegister(63 downto 48);
                when 10 =>              -- 
                  uartRMAPBusMasterDataIn_EventFIFO <= GPSTimeTableRegister(79 downto 64);
                when 8 =>              -- 
                  uartRMAPBusMasterDataIn_EventFIFO <= GPSTimeTableRegister(95 downto 80);
                when 6 =>              -- 
                  uartRMAPBusMasterDataIn_EventFIFO <= GPSTimeTableRegister(111 downto 96);
                when 4 =>              -- 
                  uartRMAPBusMasterDataIn_EventFIFO <= GPSTimeTableRegister(127 downto 112);
                when 2 =>              -- 
                  uartRMAPBusMasterDataIn_EventFIFO <= GPSTimeTableRegister(143 downto 128);
                when others =>
                  uartRMAPBusMasterDataIn_EventFIFO <= x"FFFF";
              end case;
              EventFIFOReadState <= Ack;
            ---------------------------------------------
            ---------------------------------------------
            elsif(-- Access to GPS Data FIFO Reset Register
              uartRMAPBusMasterAddressOut = AddressOfGPSDataFIFOResetRegister
              )then
                --reset GPS Data FIFO
                gpsDataFIFOReset   <= '1';
                EventFIFOReadState <= Ack;
            ---------------------------------------------
            ---------------------------------------------
            elsif(-- Access to GPS Data FIFO
              InitialAddressOfGPSDataFIFO<=uartRMAPBusMasterAddressOut 
              and uartRMAPBusMasterAddressOut <=FinalAddressOfGPSDataFIFO
              )then
              if(gpsDataFIFOEmpty='1')then -- if already empty, return dummy data
                uartRMAPBusMasterDataIn_EventFIFO <= (others => '0');
                EventFIFOReadState                <= Ack;
              else -- if gpsDataFIFO has data
                gpsDataFIFOReadEnable <= '1';
                EventFIFOReadState    <= Read_gpsDataFIFO_1;
              end if;
            ---------------------------------------------
            ---------------------------------------------
            elsif(-- Access to FPGA Type/Version register
              AddressOfFPGATypeRegister_L<=uartRMAPBusMasterAddressOut
              and uartRMAPBusMasterAddressOut<=AddressOfFPGAVersionRegister_H
            )then
              -- Return FPGA Type or FPGA Version
              case uartRMAPBusMasterAddressOut is
              when AddressOfFPGATypeRegister_L =>
                uartRMAPBusMasterDataIn_EventFIFO <= FPGAType(15 downto 0);  
              when AddressOfFPGATypeRegister_H =>
                  uartRMAPBusMasterDataIn_EventFIFO <= FPGAType(31 downto 16);  
              when AddressOfFPGAVersionRegister_L =>
                  uartRMAPBusMasterDataIn_EventFIFO <= FPGAVersion(15 downto 0);  
              when AddressOfFPGAVersionRegister_H =>
                  uartRMAPBusMasterDataIn_EventFIFO <= FPGAVersion(31 downto 16);
              when others =>
                  uartRMAPBusMasterDataIn_EventFIFO <= (others => '1');
              end case;
              EventFIFOReadState <= Ack;
            ---------------------------------------------
            ---------------------------------------------
            else -- Access to undefined memory address
              -- Read EventFIFO
              EventFIFOReadEnable <= '1';
              EventFIFOReadState  <= Read1;
            end if;
          else
            -- If no Read access from RMAP Target IP Core, initialize control signals
            gpsDataFIFOReadEnable <= '0';
            gpsDataFIFOReset      <= '0';
            EventFIFOReadEnable   <= '0';
          end if;
        when Read1 =>
          EventFIFOReadEnable <= '0';
          EventFIFOReadState  <= Read2;
        when Read2 =>
          uartRMAPBusMasterDataIn_EventFIFO <= EventFIFOReadData;
          EventFIFOReadState                <= Ack;
        when Read_gpsDataFIFO_1 =>
          gpsDataFIFOReadEnable <= '1';
          EventFIFOReadState    <= Read_gpsDataFIFO_2;
        when Read_gpsDataFIFO_2 =>
          uartRMAPBusMasterDataIn_EventFIFO(15 downto 8) <= gpsDataFIFODataOut;
          gpsDataFIFOReadEnable <= '0';
          EventFIFOReadState    <= Read_gpsDataFIFO_3;
        when Read_gpsDataFIFO_3 =>
          uartRMAPBusMasterDataIn_EventFIFO(7 downto 0) <= gpsDataFIFODataOut;
          EventFIFOReadState  <= Ack;
        when Ack =>
          if(uartRMAPBusMasterReadEnable_EventFIFO = '0')then
            uartRMAPBusMasterAcknowledge_EventFIFO <= '0';
            EventFIFOReadState                     <= Idle;
          else                          -- still read enable '1'
            uartRMAPBusMasterAcknowledge_EventFIFO <= '1';
          end if;
          gpsDataFIFOReadEnable <= '0';
          gpsDataFIFOReset      <= '0';
        when others =>
          EventFIFOReadState    <= Initialization;
      end case;
    end if;
  end process;

end Behavioral;
