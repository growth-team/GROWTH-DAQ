----------------------------------------------------------------------------------
-- Company:
-- Engineer:
--
-- Create Date:    13:37:03 05/03/2010
-- Design Name:
-- Module Name:    spw_receive_module2 - Behavioral
-- Project Name:
-- Target Devices:
-- Tool versions:
-- Description:
--
-- Dependencies:
--
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 2010-12-24 Takayuki Yuasa
--  a bug in isTimecode was fixed.
----------------------------------------------------------------------------------

-- Takayuki Yuasa 20140703
-- - Original file is spw_receive_module.vhdl in SpaceWire-to-GigabitEther
-- - Renamed to camel case

library IEEE, work;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_ARITH.all;
use IEEE.std_logic_UNSIGNED.all;
use work.SSDTP2Library.all;

library UNISIM;
use UNISIM.VComponents.all;

entity SSDTP2SpaceWireToTCP is
  port(
    -- SpaceWireCODEC signals
    spwReceiveFIFOReadEnable : out std_logic;
    spwReceiveFIFODataOut    : in  std_logic_vector(8 downto 0);
    spwReceiveFIFOEmpty      : in  std_logic;
    spwReceiveFIFOFull       : in  std_logic;
    tickOutIn                : in  std_logic;
    timeOutIn                : in  std_logic_vector(5 downto 0);
    -- SocketVHDL signals
    tcpSendFIFODataOut       : out std_logic_vector(7 downto 0);
    tcpSendFIFOWriteEnable   : out std_logic;
    tcpSendFIFOFull          : in  std_logic;
    -- debug
    stateOut                 : out std_logic_vector(7 downto 0);
    -- clock and reset
    clock                    : in  std_logic;
    reset                    : in  std_logic
    );
end SSDTP2SpaceWireToTCP;

architecture Behavioral of SSDTP2SpaceWireToTCP is
  --buffer fifo from sdram
  signal fifoFromSpWDataIn      : std_logic_vector(15 downto 0) := (others => '0');
  signal fifoFromSpWDataOut     : std_logic_vector(15 downto 0) := (others => '0');
  signal fifoFromSpWWriteEnable : std_logic                     := '0';
  signal fifoFromSpWReadEnable  : std_logic                     := '0';
  signal fifoFromSpWEmpty       : std_logic                     := '0';
  signal fifoFromSpWFull        : std_logic                     := '0';

  --buffer fifo to tcp
  signal fifoToTCPDataIn      : std_logic_vector(15 downto 0) := (others => '0');
  signal fifoToTCPDataOut     : std_logic_vector(15 downto 0) := (others => '0');
  signal fifoToTCPWriteEnable : std_logic                     := '0';
  signal fifoToTCPReadEnable  : std_logic                     := '0';
  signal fifoToTCPEmpty       : std_logic                     := '0';
  signal fifoToTCPFull        : std_logic                     := '0';

  --buffer fifo send state machine (for header write)
  signal fifoSendsmDataIn      : std_logic_vector(15 downto 0) := (others => '0');
  signal fifoSendsmWriteEnable : std_logic                     := '0';

  --buffer fifo1
  signal fifoBuffer1DataIn      : std_logic_vector(15 downto 0) := (others => '0');
  signal fifoBuffer1DataOut     : std_logic_vector(15 downto 0) := (others => '0');
  signal fifoBuffer1WriteEnable : std_logic                     := '0';
  signal fifoBuffer1ReadEnable  : std_logic                     := '0';
  signal fifoBuffer1Empty       : std_logic                     := '0';
  signal fifoBuffer1Full        : std_logic                     := '0';

  --buffer fifo2
  signal fifoBuffer2DataIn      : std_logic_vector(15 downto 0) := (others => '0');
  signal fifoBuffer2DataOut     : std_logic_vector(15 downto 0) := (others => '0');
  signal fifoBuffer2WriteEnable : std_logic                     := '0';
  signal fifoBuffer2ReadEnable  : std_logic                     := '0';
  signal fifoBuffer2Empty       : std_logic                     := '0';
  signal fifoBuffer2Full        : std_logic                     := '0';

  --output to SocketVHDL
  signal tcpSendFIFOReadEnable : std_logic;
  signal tcpSendDone           : std_logic;
  signal tcpSendLength         : integer;
  signal tcpSentLength         : integer;
  signal tcpSendRequest        : std_logic;

  --
  constant sdramBuf1TCPBuf2 : std_logic := '0';
  constant sdramBuf2TCPBuf1 : std_logic := '1';
  signal   switch           : std_logic := sdramBuf1TCPBuf2;

  signal size      : integer range 0 to 4096 := 0;
  signal eopType   : std_logic               := '0';
  signal packetEnd : std_logic               := '0';

  signal sizeRegister  : integer range 0 to 4096 := 0;
  signal eopType_reg   : std_logic               := '0';
  signal packetEnd_reg : std_logic               := '0';

  --write header process related
  signal writeHeader     : std_logic := '0';
  signal writeHeaderDone : std_logic := '0';

  signal delay : std_logic_vector(15 downto 0) := (others => '0');

  --states
  type main_states is
    (SwitchBuffer, WaitForCompletion);
  signal main_state : main_states := WaitForCompletion;

  type receiveStates is
    (Initialize, Idle,
     FillBufferFIFOWithTimeCode,
     RetrieveAndFillBufferFIFO, RetrieveOneByte_LSB8bit, RetrieveOneByte_MSB8bit,
     RetrieveOneByte_FillMSB8bitWithZero, BufferFullOrPacketEnd,
     Wait1ClockFor_RxFIFO_Read,
     waitforswitch, Wait2Clocks, Wait3Clocks
     );
  signal receiveState                     : receiveStates := Initialize;
  signal nextStateWait1ClockforRxFIFORead : receiveStates := Initialize;

  type sendStates is
    (Initialize, Idle, WaitForCompletion_sendheader, WaitForCompletion, WaitwriteHeaderDone, WaitForSwitch);
  signal sendState : sendStates := Initialize;

  signal indexForSizePart : integer range 0 to SSDTP_NDIGITS_SIZE_PART := 0;

  type   writeHeaderStates is (initialize, idle, writeHeader_0, writeHeader_1, waitforcompletion);
  signal writeHeaderState : writeHeaderStates := initialize;

  signal tcstateCount   : integer range 0 to 31        := 0;  --state counter for TimeCode-related process
  signal iTimeOutIn     : std_logic_vector(5 downto 0) := "000000";
  signal isTimecode     : std_logic                    := '0';
  signal isTimecode_reg : std_logic                    := '0';

  signal spwReceiveFIFOHasData : std_logic := '0';

  type TCPSendStates is (
    Idle, Sending, OutputToSendFIFO, OutputToSendFIFO2, Wait1Clock, Finalize
    );
  signal tcpSendState     : TCPSendStates := Idle;
  signal tcpSendStateNext : TCPSendStates := Idle;

begin
  --
  fifoToTCPDataIn        <= fifoSendsmDataIn;
  fifoToTCPWriteEnable   <= fifoSendsmWriteEnable;
  --double buffer
  fifoBuffer1DataIn      <= fifoFromSpWDataIn      when switch = sdramBuf1TCPBuf2 else fifoToTCPDataIn;
  fifoBuffer2DataIn      <= fifoFromSpWDataIn      when switch = sdramBuf2TCPBuf1 else fifoToTCPDataIn;
  fifoFromSpWDataOut     <= fifoBuffer1DataOut     when switch = sdramBuf1TCPBuf2 else fifoBuffer2DataOut;
  fifoToTCPDataOut       <= fifoBuffer1DataOut     when switch = sdramBuf2TCPBuf1 else fifoBuffer2DataOut;
  fifoBuffer1WriteEnable <= fifoFromSpWWriteEnable when switch = sdramBuf1TCPBuf2 else fifoToTCPWriteEnable;
  fifoBuffer2WriteEnable <= fifoFromSpWWriteEnable when switch = sdramBuf2TCPBuf1 else fifoToTCPWriteEnable;
  fifoBuffer1ReadEnable  <= fifoFromSpWReadEnable  when switch = sdramBuf1TCPBuf2 else fifoToTCPReadEnable;
  fifoBuffer2ReadEnable  <= fifoFromSpWReadEnable  when switch = sdramBuf2TCPBuf1 else fifoToTCPReadEnable;
  fifoFromSpWEmpty       <= fifoBuffer1Empty       when switch = sdramBuf1TCPBuf2 else fifoBuffer2Empty;
  fifoToTCPEmpty         <= fifoBuffer1Empty       when switch = sdramBuf2TCPBuf1 else fifoBuffer2Empty;
  fifoFromSpWFull        <= fifoBuffer1Full        when switch = sdramBuf1TCPBuf2 else fifoBuffer2Full;
  fifoToTCPFull          <= fifoBuffer1Full        when switch = sdramBuf2TCPBuf1 else fifoBuffer2Full;

  -- tcp send control
  TCPSendProcess : process(clock, reset)
  begin
    if(reset = '1')then
      -- reset
      tcpSendState <= Idle;
    else
      if(clock = '1' and clock'event)then
        case tcpSendState is
          when Idle =>
			     	tcpSentLength <= 0;
            if(tcpSendRequest = '1')then
              tcpSendState <= Sending;
            end if;
            tcpSendStateNext       <= Idle;
            tcpSendDone            <= '0';
            tcpSendFIFOWriteEnable <= '0';
          when Sending =>
            tcpSendFIFOWriteEnable <= '0';
            if(tcpSentLength=tcpSendLength or fifoToTCPEmpty = '1')then
              tcpSendDone  <= '1';
              tcpSendState <= Finalize;
            else
              fifoToTCPReadEnable <= '1';
              tcpSendState        <= Wait1Clock;
              tcpSendStateNext    <= OutputToSendFIFO;
            end if;
          when OutputToSendFIFO =>
				if(tcpSendFIFOFull='0')then
					tcpSendFIFODataOut     <= fifoToTCPDataOut(7 downto 0);
					tcpSendFIFOWriteEnable <= '1';
					tcpSentLength <= tcpSentLength + 1;
					tcpSendState           <= Wait1Clock;
          tcpSendStateNext <= OutputToSendFIFO2;
				else
					tcpSendFIFOWriteEnable <= '0';
				end if;
          when OutputToSendFIFO2 =>
				if(tcpSentLength=tcpSendLength)then
					tcpSendFIFOWriteEnable <= '0';
					tcpSendState           <= Sending;
				elsif(tcpSendFIFOFull='0')then
					tcpSendFIFODataOut     <= fifoToTCPDataOut(15 downto 8);
					tcpSendFIFOWriteEnable <= '1';
					tcpSentLength <= tcpSentLength + 1;
					tcpSendState           <= Sending;
				else
					tcpSendFIFOWriteEnable <= '0';
				end if;
          when Finalize =>
            if(tcpSendRequest = '0')then
              tcpSendDone  <= '0';
              tcpSendState <= Idle;
            end if;
          when Wait1Clock =>
            tcpSendState        <= tcpSendStateNext;
            fifoToTCPReadEnable <= '0';
            tcpSendFIFOWriteEnable <= '0';
          when others =>
            tcpSendState <= Idle;
        end case;
      end if;
    end if;
  end process;

  MainStateMachine : process (clock, reset)
  begin
    if (reset = '1') then
      main_state <= WaitForCompletion;
    elsif (clock'event and clock = '1') then
      case main_state is
        when WaitForCompletion =>
          if(receiveState = WaitForSwitch and sendState = WaitForSwitch)then
            main_state <= SwitchBuffer;
          end if;
        when SwitchBuffer =>
          switch     <= not switch;
          main_state <= WaitForCompletion;
      end case;
    end if;
  end process;

  stateOut(3 downto 0) <=
    x"1" when receiveState = Idle                                else
    x"2" when receiveState = RetrieveAndFillBufferFIFO           else
    x"3" when receiveState = RetrieveOneByte_LSB8bit             else
    x"4" when receiveState = RetrieveOneByte_MSB8bit             else
    x"5" when receiveState = RetrieveOneByte_FillMSB8bitWithZero else
    x"6" when receiveState = BufferFullOrPacketEnd               else
    x"7" when receiveState = Wait1ClockFor_RxFIFO_Read           else
    x"8" when receiveState = waitforswitch                       else
    x"9" when receiveState = Wait2Clocks                         else
    x"a" when receiveState = Wait3Clocks                         else x"b";

  TimeCodeRelatedProcess : process (clock, reset)
  begin
    if (reset = '1') then
      tcstateCount <= 0;
    elsif (clock'event and clock = '1') then
      if(tcstateCount = 0)then
        --idle
        if(tickOutIn = '1')then         --if got tick_out
                                        --latch time_out value
          iTimeOutIn   <= timeOutIn;
                                        --move to next
          tcstateCount <= 1;
        end if;
      elsif(tcstateCount = 1)then
        --wait until got_timecode is sent to TCP/IP client
        if(receiveState = FillBufferFIFOWithTimeCode)then
          tcstateCount <= 2;
        end if;
      elsif(tcstateCount = 2)then
        if(tickOutIn = '0')then
          tcstateCount <= 0;
        end if;
      end if;
    end if;
  end process;


  ReceiveBytesFromSpWIFRxFIFOProcess : process (clock, reset)
  begin
    if (reset = '1') then
      spwReceiveFIFOReadEnable <= '0';
      delay                    <= (others => '0');
      receiveState             <= Initialize;
    elsif (clock'event and clock = '1') then
      if(spwReceiveFIFOEmpty = '0' or spwReceiveFIFOFull = '1')then
        spwReceiveFIFOHasData <= '1';
      else
        spwReceiveFIFOHasData <= '0';
      end if;
      case receiveState is
        when Initialize =>
          fifoFromSpWWriteEnable   <= '0';
          packetEnd                <= '0';
          size                     <= 0;
          isTimecode               <= '0';
          if(spwReceiveFIFOEmpty = '0')then
            spwReceiveFIFOReadEnable <= '1'; --discard data in SpaceWire ReceiveFIFO
          else
            spwReceiveFIFOReadEnable <= '0';
            receiveState <= Idle;
          end if;
          -- if(Simulation1OrImplementation0 = '0')then
          --   if(delay = x"ffff")then
          --     delay        <= (others => '0');
          --     receiveState <= Idle;
          --   else
          --     delay <= delay + 1;
          --   end if;
          -- else
          --   delay        <= x"ffff";
          --   receiveState <= Idle;
          -- end if;
        when Idle =>
          size                     <= 0;
          isTimecode               <= '0';
          packetEnd                <= '0';
          fifoFromSpWWriteEnable   <= '0';
          spwReceiveFIFOReadEnable <= '0';
          if(tcstateCount = 1)then
            receiveState <= FillBufferFIFOWithTimeCode;
          elsif(spwReceiveFIFOHasData = '1')then
            receiveState <= RetrieveAndFillBufferFIFO;
          end if;
        when FillBufferFIFOWithTimeCode =>
          fifoFromSpWDataIn      <= x"00" & "00" & iTimeOutIn;
          fifoFromSpWWriteEnable <= '1';
          size                   <= 2;
          isTimecode             <= '1';
          receiveState           <= WaitForSwitch;
        when RetrieveAndFillBufferFIFO =>
          fifoFromSpWWriteEnable <= '0';
          packetEnd              <= '0';
                                        --check buffer full first
          if(fifoFromSpWFull = '1')then
            receiveState <= BufferFullOrPacketEnd;
                                        --then check if any byte is received
          elsif(spwReceiveFIFOHasData = '1')then
            spwReceiveFIFOReadEnable         <= '1';
            nextStateWait1ClockforRxFIFORead <= RetrieveOneByte_LSB8bit;
            receiveState                     <= Wait3Clocks;
          end if;
        when RetrieveOneByte_LSB8bit =>
          if(spwReceiveFIFODataOut(8) = '1')then
                                        --eop/eep
            eopType      <= spwReceiveFIFODataOut(0);
            packetEnd    <= '1';
            receiveState <= BufferFullOrPacketEnd;
          else
                                        --normal character, then continue
            fifoFromSpWDataIn(7 downto 0) <= spwReceiveFIFODataOut(7 downto 0);
            if(spwReceiveFIFOHasData = '1')then
              size                             <= size + 1;
              spwReceiveFIFOReadEnable         <= '1';
              nextStateWait1ClockforRxFIFORead <= RetrieveOneByte_MSB8bit;
              receiveState                     <= Wait3Clocks;
            end if;
          end if;
        when RetrieveOneByte_MSB8bit =>
          if(spwReceiveFIFODataOut(8) = '1')then
                                        --eop/eep
            eopType      <= spwReceiveFIFODataOut(0);
            packetEnd    <= '1';
            receiveState <= RetrieveOneByte_FillMSB8bitWithZero;
          else
                                        --normal charactre, then fill buffer fifo
            size                           <= size + 1;
            fifoFromSpWDataIn(15 downto 8) <= spwReceiveFIFODataOut(7 downto 0);
            fifoFromSpWWriteEnable         <= '1';
            receiveState                   <= RetrieveAndFillBufferFIFO;
          end if;
        when RetrieveOneByte_FillMSB8bitWithZero =>
          fifoFromSpWDataIn(15 downto 8) <= (others => '0');
          fifoFromSpWWriteEnable         <= '1';
          receiveState                   <= BufferFullOrPacketEnd;
        when BufferFullOrPacketEnd =>
          fifoFromSpWWriteEnable <= '0';
          receiveState           <= WaitForSwitch;
        when WaitForSwitch =>
          sizeRegister           <= size;
          isTimecode_reg         <= isTimecode;
          packetEnd_reg          <= packetEnd;
          eopType_reg            <= eopType;
          fifoFromSpWWriteEnable <= '0';
          if(main_state = SwitchBuffer)then
            receiveState <= Idle;
          end if;
        when Wait1ClockFor_RxFIFO_Read =>
          spwReceiveFIFOReadEnable <= '0';
          receiveState             <= nextStateWait1ClockforRxFIFORead;
        when Wait2Clocks =>
          spwReceiveFIFOReadEnable <= '0';
          receiveState             <= Wait1ClockFor_RxFIFO_Read;
        when Wait3Clocks =>
          spwReceiveFIFOReadEnable <= '0';
          receiveState             <= Wait2Clocks;
      end case;
    end if;
  end process;

  stateOut(7 downto 4) <=
    x"1" when sendState = initialize                   else
    x"2" when sendState = idle                         else
    x"3" when sendState = WaitForCompletion_sendheader else
    x"4" when sendState = WaitForCompletion            else
    x"5" when sendState = WaitwriteHeaderDone          else
    x"6" when sendState = WaitForSwitch                else x"f";

  SendToTCP_StateMachine : process (clock, reset)
  begin
    if (reset = '1') then
      sendState <= Initialize;
    elsif (clock'event and clock = '1') then
      case sendState is
        when Initialize =>
--                                      if(delay=x"ffff")then
          sendState <= Idle;
--                                      end if;
        when Idle =>
          if(receiveState = WaitForSwitch)then
            writeHeader <= '1';
            sendState   <= WaitwriteHeaderDone;
          end if;
        when WaitwriteHeaderDone =>
          if(writeHeaderDone = '1')then
            writeHeader    <= '0';
            tcpSendLength  <= SSDTP_NDIGITS_HEADER_PART;
            tcpSendRequest <= '1';
            sendState      <= WaitForCompletion_sendheader;
          end if;
        when WaitForCompletion_sendheader =>
          if(tcpSendDone = '1')then
            tcpSendRequest <= '0';
            sendState      <= WaitForSwitch;
          end if;
        when WaitForSwitch =>
          if(main_state = SwitchBuffer)then
            tcpSendRequest <= '1';
            tcpSendLength  <= sizeRegister;
            sendState      <= WaitForCompletion;
          end if;
        when WaitForCompletion =>
          if(tcpSendDone = '1')then
            tcpSendRequest <= '0';
            sendState      <= Idle;
          end if;
      end case;
    end if;
  end process;

  WriteHeader_StateMachine : process (clock, Reset)
    variable sizeRegister_vector : std_logic_vector(15 downto 0) := (others => '0');
  begin
    if (reset = '1') then
      writeHeaderState <= Initialize;
    elsif (clock'event and clock = '1') then
      case writeHeaderState is
        when Initialize =>
          writeHeaderDone  <= '0';
          writeHeaderState <= Idle;
        when Idle =>
          if(writeHeader = '1')then
            writeHeaderState <= WriteHeader_0;
          end if;
        when WriteHeader_0 =>
                                        -- [D/C FLAG(2byte)][SIZE(10bytes)][DATA(SIZEbytes)]
                                        --D/C
          if(isTimecode_reg = '1')then
            fifoSendsmDataIn <= x"00" & SSDTP_FLAG_CONTROL_GOTTIMECODE;
          elsif(packetEnd_reg = '1' and eopType = '0')then
            fifoSendsmDataIn <= x"00" & SSDTP_FLAG_DATA_COMPLETE_EOP;
          elsif(packetEnd_reg = '1' and eopType = '1')then
            fifoSendsmDataIn <= x"00" & SSDTP_FLAG_DATA_COMPLETE_EEP;
          else
            fifoSendsmDataIn <= x"00" & SSDTP_FLAG_DATA_FLAGMENTED;
          end if;
          fifoSendsmWriteEnable <= '1';
          indexForSizePart      <= 0;
          writeHeaderState      <= WriteHeader_1;
        when WriteHeader_1 =>
          sizeRegister_vector := conv_std_logic_vector(sizeRegister, 16);
                                        --SIZE
          if(indexForSizePart = SSDTP_NDIGITS_SIZE_PART)then
            fifoSendsmWriteEnable <= '0';
            writeHeaderState      <= WaitForCompletion;
          else
            if(indexForSizePart = SSDTP_NDIGITS_SIZE_PART-2)then
              fifoSendsmDataIn(7 downto 0)  <= sizeRegister_vector(15 downto 8);
              fifoSendsmDataIn(15 downto 8) <= sizeRegister_vector(7 downto 0);
            else
              fifoSendsmDataIn <= (others => '0');
            end if;
            fifoSendsmWriteEnable <= '1';
            indexForSizePart      <= indexForSizePart+2;
          end if;
        when WaitForCompletion =>
          if(writeHeader = '0')then
            writeHeaderDone  <= '0';
            writeHeaderState <= Idle;
          else
            writeHeaderDone <= '1';
          end if;
      end case;
    end if;
  end process;

  ---------------------------------------------
  -- Instantiation
  ---------------------------------------------
  fifoBuffer1 : entity work.fifo16x2k
    port map(
      wr_clk => clock,
      rd_clk => clock,
      din    => fifoBuffer1DataIn,
      rd_en  => fifoBuffer1ReadEnable,
      rst    => reset,
      wr_en  => fifoBuffer1WriteEnable,
      dout   => fifoBuffer1DataOut,
      empty  => fifoBuffer1Empty,
      full   => fifoBuffer1Full
      );

  fifoBuffer2 : entity work.fifo16x2k
    port map(
      rst    => reset,
      wr_clk => clock,
      rd_clk => clock,
      din    => fifoBuffer2DataIn,
      rd_en  => fifoBuffer2ReadEnable,
      wr_en  => fifoBuffer2WriteEnable,
      dout   => fifoBuffer2DataOut,
      empty  => fifoBuffer2Empty,
      full   => fifoBuffer2Full
      );

end Behavioral;

