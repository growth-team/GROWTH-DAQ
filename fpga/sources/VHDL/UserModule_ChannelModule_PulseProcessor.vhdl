--UserModule_ChModule_PulseProcessor.vhdl
--
--SpaceWire Board / User FPGA / Modularized Structure Template
--UserModule / Consumer Module
--extract Max ADC as Pulseheight / calculate pulse shape index
--
--
--20071114 Takayuki Yuasa
--file created
--based on UserModule_ConsumerModule_Calculator_<axValue_PSD.vhdl
--20140717 Takayuki Yuasa
--converted from UserModule_ConsumerModule_Calculator_MaxValue_PSD
--20140718 Takayuki Yuasa
--changed not to use EventManager. directly connected to event buffer in the ChannelModule
--20141102 Takayuki Yuasa
--changed data format; trigger count included and risetime removed
--20151016 Takayuki Yuasa
--changed data format; phaMin, phaFirst, phaLast, phaMaxTime, and maxDerivative added

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
entity UserModule_ChModule_PulseProcessor is
  port(
    hasEvent              : in  std_logic;
    EventBufferDataOut    : in  std_logic_vector(FifoDataWidth-1 downto 0);
    EventBufferReadEnable : out std_logic;
    --
    Consumer2ConsumerMgr  : out Signal_Consumer2ConsumerMgr;
    ConsumerMgr2Consumer  : in  Signal_ConsumerMgr2Consumer;
    --clock and reset
    Clock                 : in  std_logic;
    GlobalReset           : in  std_logic
    );
end UserModule_ChModule_PulseProcessor;

---------------------------------------------------
--Behavioral description
---------------------------------------------------
architecture Behavioral of UserModule_ChModule_PulseProcessor is

  ---------------------------------------------------
  --Declarations of Components
  ---------------------------------------------------
  component UserModule_Ram
    port(
      Address     : in  std_logic_vector(9 downto 0);
      DataIn      : in  std_logic_vector(15 downto 0);
      DataOut     : out std_logic_vector(15 downto 0);
      WriteEnable : in  std_logic;
      Clock       : in  std_logic
      );
  end component;

  ---------------------------------------------------
  --Declarations of Signals
  ---------------------------------------------------
  constant NumberOf_BaselineSample : integer := 4;

  --Signals
  signal RamAddress     : std_logic_vector(9 downto 0)  := (others => '0');
  signal RamDataIn      : std_logic_vector(15 downto 0) := (others => '0');
  signal RamDataOut     : std_logic_vector(15 downto 0) := (others => '0');
  signal Temp           : std_logic_vector(15 downto 0) := (others => '0');
  signal RamWriteEnable : std_logic                     := '0';
  signal CurrentCh      : std_logic_vector(2 downto 0)  := (others => '0');
  signal HeaderSize     : std_logic_vector(11 downto 0) := (others => '0');
  signal Realtime       : std_logic_vector(47 downto 0) := (others => '0');
  signal FlagI          : std_logic                     := '0';
  signal LoopI          : integer range 0 to 127        := 0;
  signal DataCount      : integer range 0 to 1024       := 0;
  signal LoopO          : integer range 0 to 1024       := 0;
  signal LoopP          : integer range 0 to 3          := 0;

  signal PhaPrevious    : std_logic_vector(15 downto 0) := (others => '0');
  signal PhaMax         : std_logic_vector(15 downto 0) := (others => '0');
  signal PhaMaxTime : std_logic_vector(9 downto 0)  := (others => '0');
  signal PhaMin         : std_logic_vector(15 downto 0) := (others => '0');
  signal PhaFirst         : std_logic_vector(15 downto 0) := (others => '0');
  signal PhaLast         : std_logic_vector(15 downto 0) := (others => '0');
  signal MaxDerivative         : std_logic_vector(15 downto 0) := (others => '0');
  signal Baseline       : std_logic_vector(31 downto 0) := (others => '0');
  signal TriggerCount : std_logic_vector(31 downto 0) := (others => '0');

  --Registers

  type   AdcDataVector is array (integer range <>) of std_logic_vector(ADCResolution-1 downto 0);
  signal AdcDataArray : AdcDataVector(MaximumOfDelay downto 0);

  type UserModule_StateMachine_State is
    (Initialize, Idle, Wait2Clocks,
     --Event Copy
     CopyEvent_0, CopyEvent_1, CopyEvent_1_5, CopyEvent_2, CopyEvent_3, CopyHeader_0, CopyHeader_0_5, CopyHeader_1, CopyHeader_2, CopyHeader_3,
     --Analysis
     StartAnalysis, DeleteChFlag, DeleteChFlag_2, DeleteChFlag_3, SearchPhaMax, SearchPhaMax_2,
     Baseline_1, Baseline_2, Baseline_3, Baseline_4,
     AnalysisDone,
     --Sent Result
     WaitMgrGrant,
     Send_1, Send_2, Send_3, Send_4, Send_5, Send_6, Send_7, Send_8, Send_9, Send_10, Send_11, Send_12, Send_13, Send_14,
     WriteSeparator_0,
     --Finalize
     Finalize);
  signal UserModule_state : UserModule_StateMachine_State := Initialize;

  constant ConsumerNumber : integer := 0;

  ---------------------------------------------------
  --Beginning of behavioral description
  ---------------------------------------------------
begin

  ---------------------------------------------------
  --Instantiations of Components
  ---------------------------------------------------
  inst_ram : UserModule_Ram
    port map(
      Address     => RamAddress,
      DataIn      => RamDataIn,
      DataOut     => RamDataOut,
      WriteEnable => RamWriteEnable,
      Clock       => Clock
      );

  ---------------------------------------------------
  --Static relationships
  ---------------------------------------------------

  ---------------------------------------------------
  --Dynamic Processes with Sensitivity List
  ---------------------------------------------------
  --UserModule main state machine
  MainProcess : process (Clock, GlobalReset)
  begin
    --is this process invoked with GlobalReset?
    if (GlobalReset = '0') then
      UserModule_state <= Initialize;
      --is this process invoked with Clock Event?
    elsif (Clock'event and Clock = '1') then
      case UserModule_state is
        when Initialize =>
          EventBufferReadEnable            <= '0';
          Consumer2ConsumerMgr.Data        <= (others => '0');
          Consumer2ConsumerMgr.WriteEnable <= '0';
          Consumer2ConsumerMgr.EventReady  <= '0';
          RamWriteEnable                   <= '0';
          PhaMax                           <= (others => '0');
          PhaMaxTime                   <= (others => '0');
          PhaMin                           <= (others => '1');
          PhaFirst                           <= (others => '0');
          PhaLast                           <= (others => '0');
          MaxDerivative                           <= (others => '0');
          FlagI                            <= '0';
          UserModule_state                 <= Idle;
        when Idle =>
                                        --initialize
          Baseline <= (others => '0');
          LoopI    <= 0;
                                        --if there is an event in the EventBuffer
          if (hasEvent = '1') then
            RamAddress       <= (others => '0');
            DataCount        <= 0;
            LoopP            <= 0;
            UserModule_state <= CopyEvent_0;
          end if;
          --------------------------------------------
          --Copy Event Body from EventProducer
          --------------------------------------------
        when CopyEvent_0 =>
          RamWriteEnable        <= '0';
          EventBufferReadEnable <= '1';
          UserModule_state      <= CopyEvent_1;
        when CopyEvent_1 =>
          EventBufferReadEnable <= '0';
          if (LoopP = 2) then
            LoopP            <= 0;
            UserModule_state <= CopyEvent_2;
          else
            LoopP <= LoopP + 1;
          end if;
        when CopyEvent_2 =>
          if (EventBufferDataOut(15 downto 12) = HEADER_FLAG) then
                                        --the data/header separator
            HeaderSize       <= EventBufferDataOut(11 downto 0);
            UserModule_state <= CopyHeader_0;
          else
            RamDataIn        <= EventBufferDataOut;
            RamWriteEnable   <= '1';
            UserModule_state <= CopyEvent_3;
          end if;
        when CopyEvent_3 =>
          RamWriteEnable   <= '0';
          RamAddress       <= RamAddress + 1;
          DataCount        <= DataCount + 1;
          UserModule_state <= CopyEvent_0;
          --------------------------------------------
          --Copy Event Header from EventProducer
          --------------------------------------------
        when CopyHeader_0 =>
          EventBufferReadEnable <= '1';
          LoopP                 <= 0;
          UserModule_state      <= CopyHeader_0_5;
        when CopyHeader_0_5 =>
          EventBufferReadEnable <= '0';
          if (LoopP = 2) then
            LoopP            <= 0;
            UserModule_state <= CopyHeader_1;
          else
            LoopP <= LoopP + 1;
          end if;
        when CopyHeader_1 =>
          LoopI <= LoopI + 1;
          case LoopI is
            when 0 =>
              Realtime(47 downto 32) <= EventBufferDataOut;
              UserModule_state       <= CopyHeader_0;
            when 1 =>
              Realtime(31 downto 16) <= EventBufferDataOut;
              UserModule_state       <= CopyHeader_0;
            when 2 =>
              Realtime(15 downto 0) <= EventBufferDataOut;
              RamAddress            <= (others => '0');
              UserModule_state      <= CopyHeader_0;
            when 3 =>
              TriggerCount(31 downto 16) <= EventBufferDataOut;
              UserModule_state           <= CopyHeader_0;
            when 4 =>
              TriggerCount(15 downto 0) <= EventBufferDataOut;
              UserModule_state          <= StartAnalysis;
            when others =>
              UserModule_state <= StartAnalysis;
          end case;
          --------------------------------------------
          --Event Analysis
          --------------------------------------------
        when StartAnalysis =>
                                        --first of all, delete ch information
                                        --which is written in the first data
          RamAddress       <= (others => '0');
          UserModule_state <= DeleteChFlag;
        when DeleteChFlag =>
          Temp             <= RamDataOut;
          CurrentCh        <= RamDataOut(14 downto 12);
          UserModule_state <= DeleteChFlag_2;
        when DeleteChFlag_2 =>
          --write back to RAM
          RamDataIn        <= x"0" & Temp(11 downto 0);
          RamWriteEnable   <= '1';
          --update pha_first
          PhaFirst <= x"0" & Temp(11 downto 0);
          UserModule_state <= DeleteChFlag_3;
        when DeleteChFlag_3 =>
          RamWriteEnable   <= '0';
          RamAddress       <= (others => '0');
          UserModule_state <= SearchPhaMax;
          --loop to search pha_max and pha_min
        when SearchPhaMax =>
          --initialize pha_max and pha_min
          PhaMax           <= (others => '0');
          PhaMin           <= (others => '1');
          PhaPrevious      <= (others => '0');
          MaxDerivative      <= (others => '0');
          LoopO            <= 0;
          RamAddress       <= RamAddress + 1;
          UserModule_state <= SearchPhaMax_2;
        when SearchPhaMax_2 =>
          --search pha_max
          if (PhaMax < RamDataOut) then
            PhaMax         <= RamDataOut;
            PhaMaxTime <= RamAddress;
          end if;
          --search pha_min
          if (RamDataOut < PhaMin) then
            PhaMin         <= RamDataOut;
          end if;
          --search maxDerivative
          if(LoopO/=0)then --only when 1<=LoopO
            if(RamDataOut<PhaPrevious)then
              if( --
                MaxDerivative < --
                (PhaPrevious-RamDataOut) --derivative
                )then
                --update MaxDerivative
                MaxDerivative <= PhaPrevious-RamDataOut;
              end if;
            else
              if( --
                MaxDerivative < --
                (RamDataOut-PhaPrevious) --derivative
                )then
                --update MaxDerivative
                MaxDerivative <= RamDataOut-PhaPrevious;
              end if;
            end if;
          end if;
          --update PhaPrevious
          PhaPrevious <= RamDataOut;
          --loop
          if (LoopO = DataCount-1) then -- last pha
            UserModule_state <= Baseline_1;
            --update pha_last
            PhaLast <= RamDataOut;
          else
            LoopO      <= LoopO + 1;
            RamAddress <= RamAddress + 1;
          end if;
          --calc baseline
        when Baseline_1 =>
          RamAddress       <= (others => '0');
          LoopO            <= 0;
          UserModule_state <= Baseline_2;
        when Baseline_2 =>
          RamAddress       <= RamAddress + 1;
          UserModule_state <= Baseline_3;
        when Baseline_3 =>
          if (LoopO = NumberOf_BaselineSample) then
            Baseline         <= "00" & Baseline(31 downto 2);
            UserModule_state <= Baseline_4;
          else
            LoopO      <= LoopO + 1;
            Baseline   <= Baseline + (x"0000"&RamDataOut);
            RamAddress <= RamAddress + 1;
          end if;
        when Baseline_4 =>
          UserModule_state <= AnalysisDone;
        when AnalysisDone =>
          RamWriteEnable                  <= '0';
          Consumer2ConsumerMgr.EventReady <= '1';
                                        --send waveform data to SDRAM
          UserModule_state                <= WaitMgrGrant;
          --------------------------------------------
          --Send to ConsumerMgr
          --Event packet format version 20151016
          --------------------------------------------
        when WaitMgrGrant =>
          if (ConsumerMgr2Consumer.Grant = '1') then
            UserModule_state <= Send_1;
          end if;
        when Send_1 =>
          Consumer2ConsumerMgr.WriteEnable <= '1';
          Consumer2ConsumerMgr.Data        <= x"FFF0";
          UserModule_state                 <= Send_2;
        when Send_2 =>
          Consumer2ConsumerMgr.Data(15 downto 11) <= (others => '0');
          Consumer2ConsumerMgr.Data(10 downto 8)  <= CurrentCh;
          Consumer2ConsumerMgr.Data(7 downto 0)  <= Realtime(39 downto 32);
          UserModule_state                       <= Send_3;
        when Send_3 =>
          Consumer2ConsumerMgr.Data <= Realtime(31 downto 16);
          UserModule_state          <= Send_4;
        when Send_4 =>
          Consumer2ConsumerMgr.Data <= Realtime(15 downto 0);
          UserModule_state          <= Send_5;
        when Send_5 =>
          Consumer2ConsumerMgr.Data <= x"CAFE"; --reserved
          UserModule_state          <= Send_6;
        when Send_6 =>
          Consumer2ConsumerMgr.Data <= TriggerCount(15 downto 0);
          UserModule_state          <= Send_7;
        when Send_7 =>
          Consumer2ConsumerMgr.Data <= PhaMax;
          UserModule_state          <= Send_8;
        when Send_8 =>
          Consumer2ConsumerMgr.Data <= "000000" & PhaMaxTime;
          UserModule_state          <= Send_9;
        when Send_9 =>
          Consumer2ConsumerMgr.Data <= PhaMin;
          UserModule_state          <= Send_10;
        when Send_10 =>
          Consumer2ConsumerMgr.Data <= PhaFirst;
          UserModule_state          <= Send_11;
        when Send_11 =>
          Consumer2ConsumerMgr.Data <= PhaLast;
          UserModule_state                 <= Send_12;
        when Send_12 =>
          Consumer2ConsumerMgr.Data <= MaxDerivative;
          RamAddress                <= (others => '0');
          UserModule_state                 <= Send_13;
        when Send_13 =>
          Consumer2ConsumerMgr.Data <= Baseline(15 downto 0);
          RamAddress                       <= RamAddress + 1;
			 if(conv_integer(ConsumerMgr2Consumer.EventPacket_NumberOfWaveform(9 downto 0))=0)then
            UserModule_state                 <= WriteSeparator_0;
			 else
				UserModule_state                 <= Send_14;
			  end if;
        when Send_14 =>                 --waveform
          if (RamAddress > ConsumerMgr2Consumer.EventPacket_NumberOfWaveform(9 downto 0)) then
            Consumer2ConsumerMgr.WriteEnable <= '0';
            UserModule_state                 <= WriteSeparator_0;
          else
            Consumer2ConsumerMgr.WriteEnable <= '1';
            Consumer2ConsumerMgr.Data        <= RamDataOut;
            RamAddress                       <= RamAddress + 1;
          end if;
        when WriteSeparator_0 =>
          Consumer2ConsumerMgr.WriteEnable <= '1';
          Consumer2ConsumerMgr.Data        <= x"ffff";
          UserModule_state                 <= Finalize;
          --------------------------------------------
          --Finalize
          --------------------------------------------
        when Finalize =>
          Consumer2ConsumerMgr.WriteEnable <= '0';
          Consumer2ConsumerMgr.EventReady  <= '0';
          if (ConsumerMgr2Consumer.Grant = '0') then
            LoopO            <= 0;
            UserModule_state <= Idle;
          end if;
        when others =>
          UserModule_state <= Initialize;
      end case;
    end if;
  end process;
  
end Behavioral;
