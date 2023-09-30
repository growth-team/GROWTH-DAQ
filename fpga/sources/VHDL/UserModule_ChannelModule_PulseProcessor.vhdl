--UserModule_ChModule_PulseProcessor.vhdl
--extract Max ADC as Pulseheight / calculate pulse shape index

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
  generic(
    ChNumber : std_logic_vector(2 downto 0) := (others => '0')
  );
  port(
    hasEvent              : in  std_logic;
    WaveformBufferDataOut    : in  std_logic_vector(WaveformBufferDataWidth-1 downto 0);
    WaveformBufferReadEnable : out std_logic;
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
  --Declarations of Signals
  ---------------------------------------------------
  constant NumberOf_BaselineSample : integer := 4;

  --Signals
  signal LoopI          : integer range 0 to 127        := 0;

  signal PhaPrevious    : std_logic_vector(15 downto 0) := (others => '0');
  signal PhaMax         : std_logic_vector(15 downto 0) := (others => '0');
  signal PhaMin         : std_logic_vector(15 downto 0) := (others => '0');
  signal PhaMaxTime : std_logic_vector(15 downto 0)  := (others => '0');
  signal PhaMinTime : std_logic_vector(15 downto 0)  := (others => '0');
  signal PhaFirst         : std_logic_vector(15 downto 0) := (others => '0');
  signal PhaLast         : std_logic_vector(15 downto 0) := (others => '0');
  signal MaxDerivative         : std_logic_vector(15 downto 0) := (others => '0');
  signal Baseline       : std_logic_vector(31 downto 0) := (others => '0');
  signal TriggerCount : std_logic_vector(31 downto 0) := (others => '0');

  signal WaveformWriteCount : std_logic_vector(15 downto 0) := (others => '0');

  type UserModule_StateMachine_State is
    (Initialize, Idle, WaitMgrGrant,
     Send_0, Send_1, Send_2, Send_3, Send_4, Send_5, Send_6, Send_7, Send_8, Send_9,
     Send_10, Send_11, Send_12, Send_13,
     Finalize);
  signal UserModule_state : UserModule_StateMachine_State := Initialize;

begin

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
          WaveformBufferReadEnable            <= '0';
          Consumer2ConsumerMgr.Data        <= (others => '0');
          Consumer2ConsumerMgr.WriteEnable <= '0';
          Consumer2ConsumerMgr.EventReady  <= '0';
          PhaMax                           <= (others => '0');
          PhaMaxTime                   <= (others => '0');
          PhaMin                           <= (others => '1');
          PhaFirst                           <= (others => '0');
          PhaLast                           <= (others => '0');
          MaxDerivative                           <= (others => '0');
          UserModule_state                 <= Idle;
        when Idle =>
          Baseline <= (others => '0');
          LoopI    <= 0;
          WaveformWriteCount <= (others => '0');

          if (hasEvent = '1') then
            -- Request write access to the Event Packet Buffer
            Consumer2ConsumerMgr.EventReady <= '1';
            UserModule_state <= WaitMgrGrant;
          end if;

        when WaitMgrGrant =>
          if (ConsumerMgr2Consumer.Grant = '1') then
            UserModule_state <= Send_1;
          end if;

        ---------------------------------------------
        -- Packet Format (version 20230823)
        ---------------------------------------------
        -- EVENT_PACKET_START_FLAG  (0xCAFE)
        -- NUM_WAVEFORM (n)
        -- WAVEFORM_0
        -- WAVEFORM_1
        -- ...
        -- WAVEFORM_(n-1)
        -- REALTIME_H
        -- REALTIME_M
        -- REALTIME_L
        -- TRIGGER_COUNT_H
        -- TRIGGER_COUNT_L
        -- CHANNEL_INDEX
        -- PHA_MAX
        -- PHA_MAX_TIME
        -- PHA_MIN
        -- PHA_MIN_TIME
        -- PHA_FIRST
        -- PHA_LAST
        -- MAX_DERIVATIVE
        -- BASELINE
        -- EVENT_PACKET_END_FLAG (0xBEEF)

        when Send_0 =>
          Consumer2ConsumerMgr.WriteEnable <= '1';
          -- Packet Word 0
          Consumer2ConsumerMgr.Data        <= x"CAFE";
          UserModule_state                 <= Send_2;

        when Send_1 =>
          -- Packet Word 1
          Consumer2ConsumerMgr.Data        <= ConsumerMgr2Consumer.EventPacket_NumberOfWaveform;
          -- Start reading EventBuffer
          WaveformBufferReadEnable <= '1';
          UserModule_state                 <= Send_2;

        when Send_2 =>
          Consumer2ConsumerMgr.Data <= WaveformBufferDataOut(15 downto 0);
          if (WaveformBufferDataOut(17 downto 16) = BUFFER_FIFO_CONTROL_FLAG_FIRST_DATA) then
            Consumer2ConsumerMgr.WriteEnable <= '1';
            WaveformWriteCount <= WaveformWriteCount + 1;

            -- Initialize PhaMax/PhaMin
            PhaMax      <= WaveformBufferDataOut(15 downto 0);
            PhaMaxTime  <= (others => '0');
            PhaMin      <= WaveformBufferDataOut(15 downto 0);
            PhaMinTime  <= (others => '0');
            PhaFirst    <= WaveformBufferDataOut(15 downto 0);
            PhaPrevious <= WaveformBufferDataOut(15 downto 0);
            MaxDerivative <= (others => '0');
            Baseline <= x"0000" & WaveformBufferDataOut(15 downto 0);

            UserModule_state   <= Send_3;
          else
            Consumer2ConsumerMgr.WriteEnable <= '0';
          end if;

        when Send_3 =>
            Consumer2ConsumerMgr.Data <= WaveformBufferDataOut(15 downto 0);
            if (WaveformBufferDataOut(17 downto 16) = BUFFER_FIFO_CONTROL_FLAG_DATA) then
              -- Write waveform data until the header flag appears or the number of waveform samples reaches the limit.
              if (WaveformWriteCount < ConsumerMgr2Consumer.EventPacket_NumberOfWaveform) then
                -- Continue writing waveform data
                Consumer2ConsumerMgr.WriteEnable <= '1';
              else
                Consumer2ConsumerMgr.WriteEnable <= '0';
              end if;

              -- Update PhaPrevious
              PhaPrevious <= WaveformBufferDataOut(15 downto 0);

              -- Analysis: PhaMax
              if (PhaMax < WaveformBufferDataOut(15 downto 0)) then
                PhaMax     <= WaveformBufferDataOut(15 downto 0);
                PhaMaxTime <= WaveformWriteCount;
              end if;

              -- Analysis: PhaMin
              if (PhaMin > WaveformBufferDataOut(15 downto 0)) then
                PhaMin     <= WaveformBufferDataOut(15 downto 0);
                PhaMinTime <= WaveformWriteCount;
              end if;

              -- Analysis: MaxDerivative
              if(WaveformBufferDataOut(15 downto 0)<PhaPrevious)then
                if (MaxDerivative < (PhaPrevious-WaveformBufferDataOut(15 downto 0))) then
                  MaxDerivative <= PhaPrevious-WaveformBufferDataOut(15 downto 0);
                end if;
              else
                if (MaxDerivative < (WaveformBufferDataOut(15 downto 0)-PhaPrevious)) then
                  MaxDerivative <= WaveformBufferDataOut(15 downto 0)-PhaPrevious;
                end if;
              end if;

              -- Analysis: Baseline
              if (WaveformWriteCount < NumberOf_BaselineSample) then
                Baseline <= Baseline + WaveformBufferDataOut(15 downto 0);
              end if;

            elsif (WaveformBufferDataOut(17 downto 16) = BUFFER_FIFO_CONTROL_FLAG_HEADER) then
              Consumer2ConsumerMgr.WriteEnable <= '1';
            elsif (WaveformBufferDataOut(17 downto 16) = BUFFER_FIFO_CONTROL_FLAG_END) then
              Consumer2ConsumerMgr.WriteEnable <= '0';
              WaveformBufferReadEnable <= '0';
              -- Analysis: PhaLast
              PhaLast <= PhaPrevious;
              -- Analysis: Baseline
              Baseline <= "00" & Baseline(31 downto 2);
              UserModule_state      <= Send_4;
            end if;

        when Send_4 =>
          Consumer2ConsumerMgr.Data(15 downto 3)  <= (others => '0');
          Consumer2ConsumerMgr.Data(2 downto 0)  <= ChNumber;
          UserModule_state          <= Send_5;
        when Send_5 =>
          Consumer2ConsumerMgr.Data <= PhaMax;
          UserModule_state          <= Send_6;
        when Send_6 =>
          Consumer2ConsumerMgr.Data <= PhaMaxTime;
          UserModule_state                 <= Send_7;
        when Send_7 =>
          Consumer2ConsumerMgr.Data <= PhaMin;
          UserModule_state                 <= Send_8;
        when Send_8 =>
          Consumer2ConsumerMgr.Data <= PhaMinTime;
          UserModule_state                 <= Send_9;
        when Send_9 =>
          Consumer2ConsumerMgr.Data <= PhaFirst;
          UserModule_state                 <= Send_10;
        when Send_10 =>
          Consumer2ConsumerMgr.Data <= PhaLast;
          UserModule_state                 <= Send_11;
        when Send_11 =>
          Consumer2ConsumerMgr.Data <= MaxDerivative;
          UserModule_state                 <= Send_12;
        when Send_12 =>
          Consumer2ConsumerMgr.Data <= Baseline(15 downto 0);
          UserModule_state                 <= Send_13;
        when Send_13 =>
          Consumer2ConsumerMgr.Data <= x"BEEF"; -- EVENT_PACKET_END_FLAG
          UserModule_state                 <= Finalize;

          --------------------------------------------
          --Finalize
          --------------------------------------------
        when Finalize =>
          Consumer2ConsumerMgr.WriteEnable <= '0';
          Consumer2ConsumerMgr.EventReady  <= '0';
          if (ConsumerMgr2Consumer.Grant = '0') then
            UserModule_state <= Idle;
          end if;
        when others =>
          UserModule_state <= Initialize;
      end case;
    end if;
  end process;

end Behavioral;
