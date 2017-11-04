--UserModule_ChannelManager.vhdl
--
--SpaceWire Board / User FPGA / Modularized Structure Template
--UserModule / Ch Module
--
--ver20081209 Takayuki Yuasa
--added PRESET_NUMBEROFEVENT_MODE
--ver20081112 Takayuki Yuasa
--ver20071025 Takayuki Yuasa
--file created
--based on UserModule_ChModule.vhdl (ver20071025)

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
library UNISIM;
use UNISIM.Vcomponents.all;

---------------------------------------------------
--Entity Declaration
---------------------------------------------------
entity UserModule_ChannelManager is
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
end UserModule_ChannelManager;

---------------------------------------------------
--Behavioral description
---------------------------------------------------
architecture Behavioral of UserModule_ChannelManager is

  ---------------------------------------------------
  --Declarations of Components
  ---------------------------------------------------
  --BusIF used for BusProcess(Bus Read/Write Process)
  component iBus_BusIFLite
    generic(
      InitialAddress : std_logic_vector(15 downto 0);
      FinalAddress   : std_logic_vector(15 downto 0)
      );
    port(
      --connected to BusController
      BusIF2BusController : out iBus_Signals_BusIF2BusController;
      BusController2BusIF : in  iBus_Signals_BusController2BusIF;

      --Connected to UserModule
      --UserModule master signal
      SendAddress : in  std_logic_vector(15 downto 0);
      SendData    : in  std_logic_vector(15 downto 0);
      SendGo      : in  std_logic;
      SendDone    : out std_logic;

      ReadAddress : in  std_logic_vector(15 downto 0);
      ReadData    : out std_logic_vector(15 downto 0);
      ReadGo      : in  std_logic;
      ReadDone    : out std_logic;

      --UserModule target signal
      beReadAddress : out std_logic_vector(15 downto 0);
      beRead        : out std_logic;
      beReadData    : in  std_logic_vector(15 downto 0);
      beReadDone    : in  std_logic;

      beWrittenAddress : out std_logic_vector(15 downto 0);
      beWritten        : out std_logic;
      beWrittenData    : out std_logic_vector(15 downto 0);
      beWrittenDone    : in  std_logic;

      Clock       : in std_logic;
      GlobalReset : in std_logic
      );
  end component;

  component UserModule_ChannelManager_Timer
    port(
      ChMgr2LiveTimer : in  Signal_ChMgr2LiveTimer;
      LiveTimer2ChMgr : out Signal_LiveTimer2ChMgr;
      --clock and reset
      Clock           : in  std_logic;
      GlobalReset     : in  std_logic
      );
  end component;

  component UserModule_ChannelManager_EventCounter
    port(
      ChMgr2EventCounter : in  Signal_ChMgr2EventCounter;
      EventCounter2ChMgr : out Signal_EventCounter2ChMgr;
      --clock and reset
      Clock              : in  std_logic;
      GlobalReset        : in  std_logic
      );
  end component;

  ---------------------------------------------------
  --Declarations of Signals
  ---------------------------------------------------
  --Signals     
  signal ResetWaitCounter        : integer range 0 to 3                               := 0;
  signal StartStopSemaphore_Wait : std_logic                                          := '0';
  signal Realtime                : std_logic_vector(WidthOfRealTime-1 downto 0)       := (others => '0');
  signal Reset_internal          : std_logic                                          := '1';
  signal LoopI                   : integer range 0 to NumberOfProducerNodes           := 0;
  signal VetoArray               : std_logic_vector(NumberOfProducerNodes-1 downto 0) := (others => '0');
  signal TriggerBus              : std_logic_vector(WidthOfTriggerBus-1 downto 0)     := (others => '0');

  signal Livetimer2ChMgr_vector : Signal_Livetimer2ChMgr_vector(NumberOfProducerNodes-1 downto 0);
  signal ChMgr2Livetimer_vector : Signal_ChMgr2Livetimer_vector(NumberOfProducerNodes-1 downto 0);

  signal EventCounter2ChMgr_vector : Signal_EventCounter2ChMgr_vector(NumberOfProducerNodes-1 downto 0);
  signal ChMgr2EventCounter_vector : Signal_ChMgr2EventCounter_vector(NumberOfProducerNodes-1 downto 0);

  --Registers
  signal StartStopRegister             : std_logic_vector(15 downto 0) := (others => '0');
  --StartStopRegister 1=started 0=stoppped
  signal StartStopSemaphoreRegister    : std_logic_vector(15 downto 0) := (others => '0');
  signal AdcClockRegister              : std_logic_vector(15 downto 0) := ADCClockFrequencyRegister_ADCClock200MHz;
  signal PresetModeRegister            : std_logic_vector(15 downto 0) := (others => '0');
  signal PresetLivetimeRegisterL       : std_logic_vector(15 downto 0) := (others => '0');
  signal PresetLivetimeRegisterH       : std_logic_vector(15 downto 0) := (others => '0');
  signal ResetRegister                 : std_logic_vector(15 downto 0) := (others => '0');
  signal PresetNumberOfEventRegisterL  : std_logic_vector(15 downto 0) := (others => '0');
  signal PresetNumberOfEventRegisterH  : std_logic_vector(15 downto 0) := (others => '0');
  signal NumberOfEventChSelectRegister : std_logic_vector(15 downto 0) := (others => '0');


  constant PRESET_LIVETIME_MODE      : std_logic_vector(15 downto 0) := x"0001";
  constant PRESET_NUMBEROFEVENT_MODE : std_logic_vector(15 downto 0) := x"0002";

  --iBus_BusIFLite signals
  --Signals used in iBus process
  signal SendAddress : std_logic_vector(15 downto 0);
  signal SendData    : std_logic_vector(15 downto 0);
  signal SendGo      : std_logic;
  signal SendDone    : std_logic;

  signal ReadAddress : std_logic_vector(15 downto 0);
  signal ReadData    : std_logic_vector(15 downto 0);
  signal ReadGo      : std_logic;
  signal ReadDone    : std_logic;

  --UserModule target signal
  signal beReadAddress : std_logic_vector(15 downto 0);
  signal beRead        : std_logic;
  signal beReadData    : std_logic_vector(15 downto 0);
  signal beReadDone    : std_logic;

  signal beWrittenAddress : std_logic_vector(15 downto 0);
  signal beWritten        : std_logic;
  signal beWrittenData    : std_logic_vector(15 downto 0);
  signal beWrittenDone    : std_logic;

  --Registers

  --Counters

  --for simulation

  --State Machines' State-variables
  type iBus_beWritten_StateMachine_State is
    (Initialize, Idle, beWritten2, ClearStartRegister, ClearStartRegister_2, ResetWait, WaitDone);
  signal iBus_beWritten_state : iBus_beWritten_StateMachine_State := Initialize;

  type iBus_beRead_StateMachine_State is
    (Initialize, Idle, WaitDone);
  signal iBus_beRead_state : iBus_beRead_StateMachine_State := Initialize;

  ---------------------------------------------------
  --Beginning of behavioral description
  ---------------------------------------------------
begin

  ---------------------------------------------------
  --Static relationships
  ---------------------------------------------------
  ResetOut <= Reset_internal;
  
  -- Measurement status
  MeasurementStarted <= '0' when conv_integer(StartStopRegister(NADCChannels-1 downto 0))=0 else '1';
  
  adcClockFrequencySelection <=         --
                                ADCClock200Mhz when AdcClockRegister = ADCClockFrequencyRegister_ADCClock200MHz else
                                ADCClock100Mhz when AdcClockRegister = ADCClockFrequencyRegister_ADCClock100MHz else
                                ADCClock50Mhz  when AdcClockRegister = ADCClockFrequencyRegister_ADCClock50MHz  else
                                ADCClock200Mhz;
  
  Connection : for I in 0 to NumberOfProducerNodes-1 generate
    VetoArray(I)
 <= '1' when
      (StartStopRegister(I) = '0')
      or ChModule2ChMgr_vector(I).Veto = '1'
      or (Livetimer2ChMgr_vector(I).Done = '1' and PresetModeRegister = PRESET_LIVETIME_MODE)
      or (EventCounter2ChMgr_vector(I).EventCounterVeto = '1' and PresetModeRegister = PRESET_NUMBEROFEVENT_MODE)
      else '0';

    --Module for Presetmode=Livetime
    TriggerBus(I)                         <= ChModule2ChMgr_vector(I).Trigger;
    ChMgr2ChModule_vector(I).Realtime     <= Realtime;
    ChMgr2ChModule_vector(I).CommonGateIn <= CommonGateIn;
    ChMgr2ChModule_vector(I).TriggerBus   <= TriggerBus;
    ChMgr2ChModule_vector(I).Veto         <= VetoArray(I);
    ChMgr2ChModule_vector(I).Livetime     <= Livetimer2ChMgr_vector(I).Livetime;

    ChMgr2Livetimer_vector(I).PresetLivetime
 <= PresetLivetimeRegisterH & PresetLivetimeRegisterL
      when PresetModeRegister = PRESET_LIVETIME_MODE else (others => '0');
    ChMgr2Livetimer_vector(I).Veto  <= VetoArray(I);
    ChMgr2Livetimer_vector(I).Reset <= Reset_internal;
    inst_Timer : UserModule_ChannelManager_Timer
      port map(
        ChMgr2LiveTimer => ChMgr2Livetimer_vector(I),
        LiveTimer2ChMgr => Livetimer2ChMgr_vector(I),
        --clock and reset
        Clock           => Clock,
        GlobalReset     => GlobalReset
        );

    --Module for Presetmode=EventNumber
    ChMgr2EventCounter_vector(I).Veto                <= VetoArray(I);
    ChMgr2EventCounter_vector(I).Trigger             <= ChModule2ChMgr_vector(I).Trigger;
    ChMgr2EventCounter_vector(I).PresetNumberOfEvent <= PresetNumberOfEventRegisterH & PresetNumberOfEventRegisterL;
    ChMgr2EventCounter_vector(I).Reset               <= Reset_internal;
    inst_EventCounter : UserModule_ChannelManager_EventCounter
      port map(
        ChMgr2EventCounter => ChMgr2EventCounter_vector(I),
        EventCounter2ChMgr => EventCounter2ChMgr_vector(I),
        --clock and reset
        Clock              => Clock,
        GlobalReset        => GlobalReset
        );
  end generate;

  ---------------------------------------------------
  --Dynamic Processes with Sensitivity List
  ---------------------------------------------------

  --UserModule main state machine
  MainProcess : process (Clock, GlobalReset)
  begin
    --is this process invoked with GlobalReset?
    if (GlobalReset = '0') then
      Realtime <= (others => '0');
      --is this process invoked with Clock Event?
    elsif (Clock'event and Clock = '1') then
      if (Reset_internal = '1') then
        Realtime <= Realtime + 1;
      end if;
    end if;
  end process;

  --processes beWritten access from BusIF
  --usually, set register value according to beWritten data
  iBus_beWritten_Process : process (Clock, GlobalReset)
  begin
    --is this process invoked with GlobalReset?
    if (GlobalReset = '0') then
      --Initialize StateMachine's state
      iBus_beWritten_state <= Initialize;
      --is this process invoked with Clock Event?
    elsif (Clock'event and Clock = '1') then
      case iBus_beWritten_state is
        when Initialize =>
          StartStopRegister            <= (others => '0');
          StartStopSemaphoreRegister   <= (others => '0');
          PresetModeRegister           <= (others => '0');
          PresetLivetimeRegisterL      <= (others => '0');
          PresetLivetimeRegisterH      <= (others => '0');
          PresetNumberOfEventRegisterL <= (others => '0');
          PresetNumberOfEventRegisterH <= (others => '0');
          ResetRegister                <= (others => '0');
          LoopI                        <= 0;
          Reset_internal               <= '1';
          ResetWaitCounter             <= 0;
                                        --move to next state
          iBus_beWritten_state         <= Idle;
        when Idle =>
          StartStopSemaphore_Wait <= '0';
          if (beWritten = '1') then
            iBus_beWritten_state <= beWritten2;
          elsif (ResetRegister /= x"0000") then
            Reset_internal       <= '0';
            iBus_beWritten_state <= ResetWait;
          else
            if (LoopI = NumberOfProducerNodes) then
              LoopI <= 0;
            else
                                                                                                            --livetime preset mode ga done ni natta tokino shori
                                                                                                            --ch gotoni okonau (timer gotoni okonau)
              if (StartStopRegister(LoopI) = '1') then
                                                                                                            --moshi, sorezoreno preset mode de, shuryou jouken wo mitashite itara,
                                                                                                            --ClearStartRegister state ni idoushite, StartStopRegister wo clear suru
                if (
                  (Livetimer2ChMgr_vector(LoopI).Done = '1' and PresetModeRegister = PRESET_LIVETIME_MODE)  -- LIVETIME MODE
                  or (EventCounter2ChMgr_vector(LoopI).EventCounterVeto = '1' and PresetModeRegister = PRESET_NUMBEROFEVENT_MODE)  -- NUMBER OF EVENT MODE
                  ) then
                  iBus_beWritten_state <= ClearStartRegister;
                end if;
              end if;
            end if;
          end if;
        when ResetWait =>
          if (ResetWaitCounter = 3) then
            ResetWaitCounter     <= 0;
            ResetRegister        <= (others => '0');
            iBus_beWritten_state <= Initialize;
          else
            ResetWaitCounter <= ResetWaitCounter + 1;
          end if;
        when ClearStartRegister =>
          StartStopSemaphore_Wait <= '1';
          iBus_beWritten_state    <= ClearStartRegister_2;
        when ClearStartRegister_2 =>
          if (StartStopSemaphoreRegister = x"0000") then
            StartStopRegister(LoopI) <= '0';
            LoopI                    <= LoopI + 1;
            iBus_beWritten_state     <= Idle;
          end if;
        when beWritten2 =>
          case beWrittenAddress is
            when AddressOf_ChMgr_StartStopRegister =>
              if (StartStopSemaphoreRegister(0) = '1') then
                StartStopRegister(NumberOfProducerNodes-1 downto 0) <= beWrittenData(NumberOfProducerNodes-1 downto 0);
              end if;
            when AddressOf_ChMgr_StartStopSemaphoreRegister =>
              if (beWrittenData(0) = '1' and StartStopSemaphore_Wait = '0') then
                StartStopSemaphoreRegister <= x"ffff";
              else
                StartStopSemaphoreRegister <= (others => '0');
              end if;
            when AddressOf_ChMgr_PresetModeRegister =>
              PresetModeRegister <= beWrittenData;
            when AddressOf_ChMgr_PresetLivetimeRegisterL =>
              PresetLivetimeRegisterL <= beWrittenData;
            when AddressOf_ChMgr_PresetLivetimeRegisterH =>
              PresetLivetimeRegisterH <= beWrittenData;
            when AddressOf_ChMgr_PresetNumberOfEventRegisterL =>
              PresetNumberOfEventRegisterL <= beWrittenData;
            when AddressOf_ChMgr_PresetNumberOfEventRegisterH =>
              PresetNumberOfEventRegisterH <= beWrittenData;
            when AddressOf_ChMgr_ResetRegister =>
              ResetRegister <= beWrittenData;
            when AddressOf_ChMgr_AdcClockRegister =>
              AdcClockRegister <= beWrittenData;
            when AddressOf_ChMgr_NumberOfEventChSelectRegister =>
              NumberOfEventChSelectRegister <= beWrittenData;
            when others =>
          end case;
                                                                                                            --tell completion of the "beRead" process to iBus_BusIF
          beWrittenDone        <= '1';
                                        --move to next state
          iBus_beWritten_state <= WaitDone;
        when WaitDone =>
                                                                                                            --wait until the "beRead" process completes
          if (beWritten = '0') then
            beWrittenDone        <= '0';
                                        --move to next state
            iBus_beWritten_state <= Idle;
          end if;
        when others =>
                                        --move to next state
          iBus_beWritten_state <= Initialize;
      end case;
    end if;
  end process;


  --processes beRead access from BusIF
  --usually, return register value according to beRead-Address
  iBus_beRead_Process : process (Clock, GlobalReset)
  begin
    --is this process invoked with GlobalReset?
    if (GlobalReset = '0') then
      --Initialize StateMachine's state
      iBus_beRead_state <= Initialize;
      --is this process invoked with Clock Event?
    elsif (Clock'event and Clock = '1') then
      case iBus_beRead_state is
        when Initialize =>
                                        --move to next state
          iBus_beRead_state <= Idle;
        when Idle =>
          if (beRead = '1') then
            case beReadAddress is
              when AddressOf_ChMgr_StartStopRegister =>
                beReadData <= StartStopRegister;
              when AddressOf_ChMgr_StartStopSemaphoreRegister =>
                beReadData <= StartStopSemaphoreRegister;
              when AddressOf_ChMgr_PresetModeRegister =>
                beReadData <= PresetModeRegister;
              when AddressOf_ChMgr_PresetLivetimeRegisterL =>
                beReadData <= PresetLivetimeRegisterL;
              when AddressOf_ChMgr_PresetLivetimeRegisterH =>
                beReadData <= PresetLivetimeRegisterH;
              when AddressOf_ChMgr_PresetNumberOfEventRegisterL =>
                beReadData <= PresetNumberOfEventRegisterL;
              when AddressOf_ChMgr_PresetNumberOfEventRegisterH =>
                beReadData <= PresetNumberOfEventRegisterH;
              when AddressOf_ChMgr_RealtimeRegisterL =>
                beReadData <= Realtime(15 downto 0);
              when AddressOf_ChMgr_RealtimeRegisterM =>
                beReadData <= Realtime(31 downto 16);
              when AddressOf_ChMgr_RealtimeRegisterH =>
                beReadData <= Realtime(47 downto 32);
              when AddressOf_ChMgr_AdcClockRegister =>
                beReadData <= AdcClockRegister;
              when AddressOf_ChMgr_NumberOfEventChSelectRegister =>
                beReadData <= NumberOfEventChSelectRegister;
              when AddressOf_ChMgr_NumberOfEventRegisterL =>
                beReadData <= EventCounter2ChMgr_vector(conv_integer(NumberOfEventChSelectRegister(2 downto 0))).NumberOfEvent(15 downto 0);
              when AddressOf_ChMgr_NumberOfEventRegisterH =>
                beReadData <= EventCounter2ChMgr_vector(conv_integer(NumberOfEventChSelectRegister(2 downto 0))).NumberOfEvent(31 downto 16);
              when others =>
                                        --sonzai shina address heno yomikomi datta tokiha
                                        --0xabcd toiu tekitou na value wo kaeshite oku kotoni shitearu
                beReadData <= x"abcd";
            end case;
                                        --tell completion of the "beRead" process to iBus_BusIF
            beReadDone        <= '1';
                                        --move to next state
            iBus_beRead_state <= WaitDone;
          end if;
        when WaitDone =>
                                        --wait until the "beRead" process completes
          if (beRead = '0') then
            beReadDone        <= '0';
                                        --move to next state
            iBus_beRead_state <= Idle;
          end if;
        when others =>
                                        --move to next state
          iBus_beRead_state <= Initialize;
      end case;
    end if;
  end process;

  ---------------------------------------------------
  --Instantiations of Components
  ---------------------------------------------------
  --Instantiation of iBus_BusIF
  BusIF : iBus_BusIFLite
    generic map(
      InitialAddress => InitialAddress,
      FinalAddress   => FinalAddress
      )
    port map(
      --connected to BusController
      BusIF2BusController => BusIF2BusController,
      BusController2BusIF => BusController2BusIF,
      --Connected to UserModule
      --UserModule master signal
      SendAddress         => SendAddress,
      SendData            => SendData,
      SendGo              => SendGo,
      SendDone            => SendDone,

      ReadAddress => ReadAddress,
      ReadData    => ReadData,
      ReadGo      => ReadGo,
      ReadDone    => ReadDone,

      --UserModule target signal
      beReadAddress => beReadAddress,
      beRead        => beRead,
      beReadData    => beReadData,
      beReadDone    => beReadDone,

      beWrittenAddress => beWrittenAddress,
      beWritten        => beWritten,
      beWrittenData    => beWrittenData,
      beWrittenDone    => beWrittenDone,

      Clock       => Clock,
      GlobalReset => GlobalReset
      );

end Behavioral;


--UserModule_ChannelManager_Timer
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
entity UserModule_ChannelManager_Timer is
  port(
    ChMgr2LiveTimer : in  Signal_ChMgr2LiveTimer;
    LiveTimer2ChMgr : out Signal_LiveTimer2ChMgr;
    --clock and reset
    Clock           : in  std_logic;
    GlobalReset     : in  std_logic
    );
end UserModule_ChannelManager_Timer;

---------------------------------------------------
--Behavioral description
---------------------------------------------------
architecture Behavioral of UserModule_ChannelManager_Timer is
  signal Livetime     : std_logic_vector(WidthOfLiveTime-1 downto 0) := (others => '0');
  signal Reset        : std_logic;
  signal counter      : integer range 0 to Count10msec               := 0;
  signal Done         : std_logic                                    := '0';
  signal LivetimeMode : std_logic                                    := '0';
begin
  Reset                    <= '0' when GlobalReset = '0' or ChMgr2LiveTimer.Reset = '0'                  else '1';
  LivetimeMode             <= '0' when ChMgr2Livetimer.PresetLivetime = x"0000"                          else '1';
  LiveTimer2ChMgr.Livetime <= Livetime;
  LiveTimer2ChMgr.Done     <= Done;
  Done                     <= '1' when LivetimeMode = '1' and ChMgr2Livetimer.PresetLivetime <= Livetime else '0';
  --UserModule main state machine
  MainProcess : process (Clock, Reset)
  begin
    --is this process invoked with GlobalReset?
    if (Reset = '0') then
      --is this process invoked with Clock Event?
      counter  <= 0;
      Livetime <= (others => '0');
    elsif (Clock'event and Clock = '1') then
      if (counter = Count10msec) then
        counter <= 0;
        if (Done = '0') then
          Livetime <= Livetime + 1;
        end if;
      else
        if (ChMgr2Livetimer.Veto = '0') then
          counter <= counter + 1;
        end if;
      end if;
    end if;
  end process;
end Behavioral;


--UserModule_ChannelManager_EventCounter
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
entity UserModule_ChannelManager_EventCounter is
  port(
    ChMgr2EventCounter : in  Signal_ChMgr2EventCounter;
    EventCounter2ChMgr : out Signal_EventCounter2ChMgr;
    --clock and reset
    Clock              : in  std_logic;
    GlobalReset        : in  std_logic
    );
end UserModule_ChannelManager_EventCounter;

---------------------------------------------------
--Behavioral description
---------------------------------------------------
architecture Behavioral of UserModule_ChannelManager_EventCounter is
  signal NumberOfEvent       : std_logic_vector(WidthOfNumberOfEvent-1 downto 0) := (others => '0');
  signal Veto                : std_logic                                         := '0';
  signal Reset               : std_logic;
  signal Done                : std_logic                                         := '0';
  signal LivetimeMode        : std_logic                                         := '0';
  signal Trigger1ClockBefore : std_logic                                         := '0';
begin
  Reset                               <= '0' when GlobalReset = '0' or ChMgr2EventCounter.Reset = '0'    else '1';
  Veto                                <= '1' when ChMgr2EventCounter.PresetNumberOfEvent = NumberOfEvent else '0';
  EventCounter2ChMgr.EventCounterVeto <= Veto;

  --UserModule main state machine
  MainProcess : process (Clock, Reset)
  begin
    --is this process invoked with GlobalReset?
    if (Reset = '0') then
      --Reset
      NumberOfEvent <= (others => '0');
    elsif (Clock'event and Clock = '1') then
      if (Trigger1ClockBefore = '0' and ChMgr2EventCounter.Trigger = '1') then
        NumberOfEvent <= NumberOfEvent + 1;
      end if;
      Trigger1ClockBefore <= ChMgr2EventCounter.Trigger;
    end if;
  end process;
end Behavioral;
