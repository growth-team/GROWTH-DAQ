--UserModule_ChModule_Trigger.vhdl
--
--SpaceWire Board / User FPGA / Modularized Structure Template
--UserModule / Ch Module / Trigger Module
--
--ver20081027 Takayuki Yuasa
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
entity UserModule_ChModule_Trigger is
  port(
    ChModule2InternalModule : in  Signal_ChModule2InternalModule;
    --
    AdcDataIn               : in  std_logic_vector(ADCResolution-1 downto 0);
    --
    TriggerOut              : out std_logic;
    Veto                    : out std_logic;
    --
    TriggerCountOut         : out std_logic_vector(31 downto 0);
    --clock and reset
    Clock                   : in  std_logic;
    GlobalReset             : in  std_logic
    );
end UserModule_ChModule_Trigger;

---------------------------------------------------
--Behavioral description
---------------------------------------------------
architecture Behavioral of UserModule_ChModule_Trigger is

  ---------------------------------------------------
  --Declarations of Components
  ---------------------------------------------------

  ---------------------------------------------------
  --Declarations of Signals
  ---------------------------------------------------
  --Signals
  signal Trigger                    : std_logic := '0';
  signal Veto_latched_in_Idle_state : std_logic := '0';

  --Registers
  signal ave_b1, ave_b2, ave_b3, ave_b4 : std_logic_vector(15 downto 0) := (others => '0');
  signal Average_Before4                : std_logic_vector(17 downto 0) := (others => '0');
  signal ave_a1, ave_a2, ave_a3, ave_a4 : std_logic_vector(15 downto 0) := (others => '0');
  signal Average_After4                 : std_logic_vector(17 downto 0) := (others => '0');

  --Counters
  signal SampledNumber : integer range 0 to 1024 := 0;
  signal TriggerEnd    : std_logic               := '0';

  --State Machines' State-variables

  type UserModule_StateMachine_State is
    (Initialize, Idle, Triggered, Finalize);
  signal UserModule_state : UserModule_StateMachine_State := Initialize;

  --for simulation
  signal temp : std_logic := '0';

  ---------------------------------------------
  --20141102 Takayuki Yuasa
  -- added for trigger count
  signal TriggerCount    : std_logic_vector(31 downto 0) := (others => '0');
  signal TriggerPrevious : std_logic                     := '0';
  ---------------------------------------------

  ---------------------------------------------
  --20141102 Takayuki Yuasa
  -- for enabling CPU trigger in the TriggerMode 3
  signal CPUTriggered : std_logic := '0';

begin

  ---------------------------------------------
  --20141102 Takayuki Yuasa
  --TriggerOut <= Trigger;
  TriggerOut      <= Trigger when Veto_latched_in_Idle_state = '0' else '0';
  TriggerCountOut <= TriggerCount;
  ---------------------------------------------

  Trigger <= '1' when UserModule_state = Triggered and TriggerEnd = '0' else '0';
  Veto    <= '1' when UserModule_state = Triggered                      else '0';


  Average : process (Clock, GlobalReset)
  begin
    --is this process invoked with GlobalReset?
    if (GlobalReset = '0') then
      --is this process invoked with Clock Event?
    elsif (Clock'event and Clock = '1') then
      ave_a1          <= "0000" & AdcDataIn; ave_a2 <= ave_a1; ave_a3 <= ave_a2; ave_a4 <= ave_a3;
      ave_b1          <= ave_a4; ave_b2 <= ave_b1; ave_b3 <= ave_b2; ave_b4 <= ave_b3;
      Average_Before4 <= ("00" & ave_b1) + ("00" & ave_b2) + ("00" & ave_b3) + ("00" & ave_b4);
      Average_After4  <= ("00" & ave_a1) + ("00" & ave_a2) + ("00" & ave_a3) + ("00" & ave_a4);
    end if;
  end process;

  --UserModule main state machine
  MainProcess : process (Clock, GlobalReset)
    variable a, b, c, d, e, N : integer;
  begin
    --is this process invoked with GlobalReset?
    if (GlobalReset = '0') then
      UserModule_state <= Initialize;
      --is this process invoked with Clock Event?
    elsif (Clock'event and Clock = '1') then

      ---------------------------------------------
      --20141102 Takayuki Yuasa
      --added for trigger count
      TriggerPrevious <= Trigger;
      if(TriggerPrevious = '0' and Trigger = '1')then
        TriggerCount <= TriggerCount + 1;
      end if;
      ---------------------------------------------

      case UserModule_state is
        when Initialize =>
          SampledNumber    <= 0;
          TriggerEnd       <= '0';
                                        --move to next state
          UserModule_state <= Idle;
        when Idle =>
          SampledNumber <= 0;
          TriggerEnd    <= '0';
          a             := conv_integer(AdcDataIn);
          b             := conv_integer(ChModule2InternalModule.ThresholdStarting);
          c             := conv_integer(ChModule2InternalModule.ThresholdClosing);
          d             := conv_integer(Average_Before4(17 downto 2));
          e             := conv_integer(Average_After4(17 downto 2));
                                        -- 20141102 Takayuki Yuasa
                                        -- veto check will be done outside the process (to realize trigger counter)
                                        --if (ChModule2InternalModule.Veto='0') then

          Veto_latched_in_Idle_state <= ChModule2InternalModule.Veto;

          case ChModule2InternalModule.TriggerMode is
            when Mode_1_StartingTh_NumberOfSamples =>
              if (a > b) then
                UserModule_state <= Triggered;
              end if;
            when Mode_2_CommonGateIn_NumberOfSamples =>
              if (ChModule2InternalModule.CommonGateIn = '1') then
                UserModule_state <= Triggered;
              end if;
            when Mode_3_StartingTh_NumberOfSamples_ClosingTh =>
              if (a > b or ChModule2InternalModule.CPUTrigger = '1') then
                if(ChModule2InternalModule.CPUTrigger = '1')then
                  CPUTriggered <= '1';
                else
                  CPUTriggered <= '0';
                end if;
                UserModule_state <= Triggered;
              end if;
            when Mode_4_Average4_StartingTh_NumberOfSamples =>
              if (e > d+b) then
                UserModule_state <= Triggered;
              end if;
            when Mode_5_CPUTrigger =>
              if (ChModule2InternalModule.CPUTrigger = '1') then
                UserModule_state <= Triggered;
              end if;
            when Mode_8_TriggerBusSelectedOR =>
                                        --if one or more of enabled channels is/are triggering
              if (conv_integer(ChModule2InternalModule.TriggerBus and ChModule2InternalModule.TriggerBusMask) /= 0) then
                UserModule_state <= Triggered;
              end if;
            when Mode_9_TriggerBusSelectedAND =>
                                        --if all enabled channels are triggering
              if (ChModule2InternalModule.TriggerBus = ChModule2InternalModule.TriggerBusMask) then
                UserModule_state <= Triggered;
              end if;
            when others =>
              UserModule_state <= Idle;
          end case;
                                        --end if;
        when Triggered =>
          a := conv_integer(AdcDataIn);
          b := conv_integer(ChModule2InternalModule.ThresholdStarting);
          c := conv_integer(ChModule2InternalModule.ThresholdClosing);
          N := conv_integer(ChModule2InternalModule.NumberOfSamples);
          case ChModule2InternalModule.TriggerMode is
            when Mode_1_StartingTh_NumberOfSamples =>
              if (SampledNumber = N) then
                UserModule_state <= Finalize;
              else
                SampledNumber <= SampledNumber + 1;
              end if;
            when Mode_2_CommonGateIn_NumberOfSamples =>
              if (SampledNumber = N) then
                TriggerEnd <= '1';
                if (ChModule2InternalModule.CommonGateIn = '0') then
                  UserModule_state <= Finalize;
                end if;
              else
                SampledNumber <= SampledNumber + 1;
              end if;
            when Mode_3_StartingTh_NumberOfSamples_ClosingTh =>
              if (SampledNumber = N) then
                TriggerEnd <= '1';
                if (a < c) then
                  UserModule_state <= Finalize;
                elsif(CPUTriggered = '1')then
                  UserModule_state <= Finalize;
                end if;
              else
                SampledNumber <= SampledNumber + 1;
              end if;
            when Mode_4_Average4_StartingTh_NumberOfSamples =>
              if (SampledNumber = N) then
                UserModule_state <= Finalize;
              else
                SampledNumber <= SampledNumber + 1;
              end if;
            when Mode_5_CPUTrigger =>
              if (SampledNumber = N) then
                TriggerEnd       <= '1';
                UserModule_state <= Finalize;
              else
                SampledNumber <= SampledNumber + 1;
              end if;
            when Mode_8_TriggerBusSelectedOR =>
              if (SampledNumber = N) then
                TriggerEnd       <= '1';
                UserModule_state <= Finalize;
              else
                SampledNumber <= SampledNumber + 1;
              end if;
            when Mode_9_TriggerBusSelectedAND =>
              if (SampledNumber = N) then
                TriggerEnd       <= '1';
                UserModule_state <= Finalize;
              else
                SampledNumber <= SampledNumber + 1;
              end if;
            when others =>
              UserModule_state <= Idle;
          end case;
        when Finalize =>
          CPUTriggered     <= '0';
          UserModule_state <= Idle;
      end case;
    end if;
  end process;
  
end Behavioral;
