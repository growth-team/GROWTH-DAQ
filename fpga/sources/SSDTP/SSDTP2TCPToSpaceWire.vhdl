----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    16:38:56 04/25/2010 
-- Design Name: 
-- Module Name:    spw_send_module - Behavioral 
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
--
----------------------------------------------------------------------------------
library IEEE, work;
use IEEE.STD_LOGIC_1164.all;
use IEEE.STD_LOGIC_ARITH.all;
use IEEE.STD_LOGIC_UNSIGNED.all;
use work.SSDTP2Library.all;

library UNISIM;
use UNISIM.VComponents.all;

entity SSDTP2TCPToSpaceWire is
  generic(
      bufferDataCountWidth : integer := 10
    );
  port(
    -- SocketVHDL signals
    tcpReceiveFIFOEmpty        : in  std_logic;
    tcpReceiveFIFODataOut      : in  std_logic_vector(7 downto 0);
    tcpReceiveFIFODataCount    : in  std_logic_vector(bufferDataCountWidth-1 downto 0);
    tcpReceiveFIFOReadEnable   : out std_logic;
    -- SpaceWireCODEC signals
    spwTransmitFIFOWriteEnable : out std_logic;
    spwTransmitFIFODataIn      : out std_logic_vector (8 downto 0);
    spwTransmitFIFOFull        : in  std_logic;
    tickInOut                  : out std_logic;
    timeInOut                  : out std_logic_vector(5 downto 0);
    txClockDivideCount         : out std_logic_vector(5 downto 0);
    -- debug
    stateOut                   : out std_logic_vector(7 downto 0);
    -- clock and reset
    clock                      : in  std_logic;
    reset                      : in  std_logic
    );
end SSDTP2TCPToSpaceWire;

architecture Behavioral of SSDTP2TCPToSpaceWire is

  type ustates is (
    init, clearTCPReceiveBuffer, idle, waitaclock, writeToTxFIFO, judgeFlag, selectPort, --
    receiveSize, setSize, selectFlag, incrementIndex, --
    retrievePacketThenSend, checkEOP, sendEop, sendTimeCode, delete2Bytes, changeTxSpeed
    );  
  signal ustate    : ustates := init;
  signal nextState : ustates := init;

  signal MAXIMUM_PACKET_SIZE : integer := 4096;
  signal packetSizeInByteVector : std_logic_vector(23 downto 0) := (others => '0');
  signal packetSizeInByte : integer range 0 to 1024*1024 := 0;
  signal sentSizeInBytes  : integer range 0 to 1024*1024 := 0;

  signal packetEnd : std_logic := '0';

  signal indexOfSizePart : integer range 0 to SSDTP_NDIGITS_SIZE_PART+2 := 0;

  signal eopType : std_logic := '0';

  signal flag : std_logic_vector(7 downto 0) := x"00";

  signal stateCount : integer range 0 to 31 := 0;

  signal controlAndTimecode : std_logic_vector(7 downto 0) := x"00";

  --this initial value generates 125/(11+12)=10.4 MHz Tx clock
  signal iTxClockDivideCount : std_logic_vector(5 downto 0) := conv_std_logic_vector(11, 6);

  --for size determination
  type   vectorOf8bits is array (integer range <>) of std_logic_vector(7 downto 0);
  signal sizeDigits : vectorOf8bits(2 downto 0);

  component fifo8x1k
    port (
      clk         : in  std_logic;
      rst         : in  std_logic;
      din         : in  std_logic_vector(7 downto 0);
      wr_en       : in  std_logic;
      rd_en       : in  std_logic;
      dout        : out std_logic_vector(7 downto 0);
      full        : out std_logic;
      almost_full : out std_logic;
      empty       : out std_logic;
      data_count  : out std_logic_vector(9 downto 0)
      );
  end component;

  signal itcpReceiveFIFOHasData : std_logic := '0';

  signal timeoutTimerExpires : std_logic := '0';
  signal ustate_previous : ustates := init;
  constant TimeoutCountMax: integer := 50e6; --1s @ 100MHz
  signal timeoutCount : integer range 0 to TimeoutCountMax := 0;
  
  constant INITIALIZE_WAIT_COUNT : integer := 1e6;
  signal initializeWaitCounter : integer range 0 to INITIALIZE_WAIT_COUNT := 0;

  -- For ILA
--  attribute mark_debug : string;
--  attribute keep : string;
--  attribute mark_debug of timeoutTimerExpires     : signal is "true";
--  attribute mark_debug of packetSizeInByte  : signal is "true";
--  attribute mark_debug of ustate  : signal is "true";
--  attribute mark_debug of tcpReceiveFIFOReadEnable  : signal is "true";
--  attribute mark_debug of tcpReceiveFIFOEmpty  : signal is "true";
--  attribute mark_debug of spwTransmitFIFOFull  : signal is "true";

begin

  itcpReceiveFIFOHasData <= '1' when tcpReceiveFIFOEmpty = '0' else '0';
  txClockDivideCount     <= iTxClockDivideCount;

--  --for test
--  stateOut <=
--    x"00" when ustate = init                   else
--    x"01" when ustate = clearTCPReceiveBuffer  else
--    x"02" when ustate = idle                   else
--    x"03" when ustate = waitaclock             else
--    x"04" when ustate = writeToTxFIFO          else
--    x"05" when ustate = judgeFlag              else
--    x"06" when ustate = setSize                else
--    x"07" when ustate = incrementIndex         else
--    x"08" when ustate = retrievePacketThenSend else
--    x"09" when ustate = sendEop                else 
--    x"11" when ustate = selectPort             else -- A
--    x"12" when ustate = selectFlag             else -- B
--    x"13" when ustate = checkEOP               else -- C
--    x"14" when ustate = sendTimeCode           else -- D
--    x"15" when ustate = delete2Bytes           else -- E
--    x"16" when ustate = changeTxSpeed          else -- F
--    x"17"; --G

--stateOut(0) <= '1' when ustate = writeToTxFIFO else '0';
--stateOut(1) <= spwTransmitFIFOFull;
--stateOut(2) <= tcpReceiveFIFOEmpty;
--stateOut(3) <= timeoutTimerExpires;

  stateOut <= sizeDigits(0);
  
  process (clock, reset)
  begin
    if (reset = '1') then
      spwTransmitFIFOWriteEnable <= '0';
      indexOfSizePart            <= 0;
      ustate                     <= init;
    elsif (clock'event and clock = '1') then

      ustate_previous <= ustate;
      if(ustate_previous/=ustate)then
        timeoutCount <= 0;
        timeoutTimerExpires <= '0';
      elsif(timeoutCount=TimeoutCountMax)then
        timeoutTimerExpires <= '1';
        timeoutCount <= 0;
      else
        timeoutCount <= timeoutCount + 1;  
        timeoutTimerExpires <= '0';
      end if;

      case ustate is
        when init =>
          stateCount                 <= 0;
          packetEnd                  <= '0';
          tcpReceiveFIFOReadEnable   <= '0';
          spwTransmitFIFOWriteEnable <= '0';
          spwTransmitFIFODataIn      <= (others => '0');
          initializeWaitCounter      <= 0;
          if(indexOfSizePart=0 or indexOfSizePart=1 or indexOfSizePart=2)then
            indexOfSizePart             <= indexOfSizePart + 1;
            sizeDigits(indexOfSizePart) <= x"00";
          else
            ustate <= clearTCPReceiveBuffer;
          end if;
        when clearTCPReceiveBuffer =>
          -- Wait a while, and then, clear tcpReceiveFIFO 
          if(initializeWaitCounter=INITIALIZE_WAIT_COUNT)then
            if(conv_integer(tcpReceiveFIFODataCount)/=0)then
              -- Read one byte from the FIFO
              tcpReceiveFIFOReadEnable <= '1';
            else
              -- If tcpReceiveFIFO is empty, stop reading
              tcpReceiveFIFOReadEnable <= '0';
              -- Proceed to the Idle state
              ustate <= idle;
            end if;
           else
            initializeWaitCounter <= initializeWaitCounter + 1;
           end if;
        when idle =>
          stateCount                 <= 0;
          packetEnd                  <= '0';
          indexOfSizePart            <= 1;
          packetSizeInByte           <= 0;
          sentSizeInBytes            <= 0;
          spwTransmitFIFOWriteEnable <= '0';
          if(
            SSDTP_NDIGITS_HEADER_PART
             <= conv_integer(tcpReceiveFIFODataCount)
            )then
            tcpReceiveFIFOReadEnable <= '1';
            nextState                <= judgeFlag;
            ustate                   <= waitaclock;
          else
            tcpReceiveFIFOReadEnable <= '0';
          end if;
        when judgeFlag =>
          flag                     <= tcpReceiveFIFODataOut;
                                        --delete following reserved byte                                
          tcpReceiveFIFOReadEnable <= '1';
          nextState                <= receiveSize;
          ustate                   <= waitaclock;
        when receiveSize =>
          if(indexOfSizePart = 11)then
            tcpReceiveFIFOReadEnable <= '0';
            packetSizeInByteVector   <= sizeDigits(2) & sizeDigits(1) & sizeDigits(0);
            ustate                   <= setSize;
          else
            tcpReceiveFIFOReadEnable <= '1';
            nextState                <= incrementIndex;
            ustate                   <= waitaclock;
          end if;
        when setSize =>
          if(conv_integer(packetSizeInByteVector)>MAXIMUM_PACKET_SIZE)then
            -- Something is wrong in the SSDTP packet header.
            -- Initialize the core.
            ustate <= init;
          else
            packetSizeInByte         <= conv_integer(packetSizeInByteVector);
            ustate                   <= selectFlag;
          end if;
        when incrementIndex =>
          indexOfSizePart <= indexOfSizePart + 1;
          if(indexOfSizePart = 10)then
            sizeDigits(0) <= tcpReceiveFIFODataOut;
          elsif(indexOfSizePart = 9)then
            sizeDigits(1) <= tcpReceiveFIFODataOut;
          elsif(indexOfSizePart = 8)then
            sizeDigits(2) <= tcpReceiveFIFODataOut;
          end if;
          ustate <= receiveSize;
        when selectFlag =>
          if(flag = SSDTP_FLAG_DATA_COMPLETE_EOP)then
            eopType   <= '0';
            packetEnd <= '1';
            ustate    <= retrievePacketThenSend;
          elsif(flag = SSDTP_FLAG_DATA_COMPLETE_EEP)then
            eopType   <= '1';
            packetEnd <= '1';
            ustate    <= retrievePacketThenSend;
          elsif(flag = SSDTP_FLAG_DATA_FLAGMENTED)then
            packetEnd <= '0';
            ustate    <= retrievePacketThenSend;
          elsif(flag = SSDTP_FLAG_CONTROL_SENDTIMECODE)then
            ustate <= sendTimeCode;
          elsif(flag = SSDTP_FLAG_CONTROL_GOTTIMECODE)then
            ustate <= delete2Bytes;
          elsif(flag = SSDTP_FLAG_CONTROL_CHANGETXSPEED)then
            ustate <= changeTxSpeed;
          else
            ustate <= init;
          end if;
          --data
        when retrievePacketThenSend =>
          spwTransmitFIFOWriteEnable <= '0';
          if(timeoutTimerExpires='1')then
            ustate <= init;
          elsif(sentSizeInBytes = packetSizeInByte)then
            ustate <= checkEOP;
          elsif((tcpReceiveFIFOEmpty = '0' and conv_integer(tcpReceiveFIFODataCount) /= 0) and spwTransmitFIFOFull = '0')then
            tcpReceiveFIFOReadEnable <= '1';
            nextState                <= writeToTxFIFO;
            ustate                   <= waitaclock;
          end if;
        when writeToTxFIFO =>
          spwTransmitFIFODataIn      <= '0' & tcpReceiveFIFODataOut;
          spwTransmitFIFOWriteEnable <= '1';
          sentSizeInBytes            <= sentSizeInBytes + 1;
          nextState                  <= retrievePacketThenSend;
          ustate                     <= waitaclock;
        when checkeop =>
          if(packetEnd = '1')then
            ustate <= sendeop;
          else
            ustate <= idle;
          end if;
        when sendeop =>
          if(spwTransmitFIFOFull = '0')then
            if(eopType = '0')then
                                        --EOP
              spwTransmitFIFODataIn <= '1' & x"00";
            else
                                        --EEP
              spwTransmitFIFODataIn <= '1' & x"01";
            end if;
            spwTransmitFIFOWriteEnable <= '1';
            ustate                     <= idle;
          end if;
          --timecode
        when sendTimeCode =>
          if(stateCount = 0)then
            if(itcpReceiveFIFOHasData = '1')then
              tcpReceiveFIFOReadEnable <= '1';
              stateCount               <= 1;
              nextState                <= sendTimeCode;
              ustate                   <= waitaclock;
            end if;
          elsif(stateCount = 1)then
            controlAndTimecode <= tcpReceiveFIFODataOut;
                                        --delete following reserved byte
            if(itcpReceiveFIFOHasData = '1')then
              tcpReceiveFIFOReadEnable <= '1';
              stateCount               <= 2;
            end if;
          elsif(stateCount = 2)then
            tcpReceiveFIFOReadEnable <= '0';
                                        --emit time code
            tickInOut                <= '1';
            timeInOut                <= controlAndTimecode(5 downto 0);
            stateCount               <= 3;
          elsif(stateCount = 3)then
            tickInOut <= '0';
            ustate    <= idle;
          end if;
        when delete2Bytes =>
          if(stateCount = 0)then
            if(itcpReceiveFIFOHasData = '1')then
              tcpReceiveFIFOReadEnable <= '1';
              stateCount               <= 1;
            end if;
          elsif(stateCount = 1)then
            if(itcpReceiveFIFOHasData = '1')then
              tcpReceiveFIFOReadEnable <= '1';
              stateCount               <= 2;
            else
              tcpReceiveFIFOReadEnable <= '0';
            end if;
          else
            tcpReceiveFIFOReadEnable <= '0';
            stateCount               <= 0;
            ustate                   <= idle;
          end if;
          --change Tx speed
        when changeTxSpeed =>
          if(stateCount = 0)then
            if(itcpReceiveFIFOHasData = '1')then
              tcpReceiveFIFOReadEnable <= '1';
              stateCount               <= 1;
              nextState                <= changeTxSpeed;
              ustate                   <= waitaclock;
            end if;
          elsif(stateCount = 1)then
                                        --change tx div count
            iTxClockDivideCount <= tcpReceiveFIFODataOut(5 downto 0);
                                        --delete following reserved byte
            if(itcpReceiveFIFOHasData = '1')then
              tcpReceiveFIFOReadEnable <= '1';
              stateCount               <= 2;
            end if;
          elsif(stateCount = 2)then
            tcpReceiveFIFOReadEnable <= '0';
            ustate                   <= idle;
          end if;
          --wait
        when waitaclock =>
          spwTransmitFIFOWriteEnable <= '0';
          tcpReceiveFIFOReadEnable   <= '0';
          ustate                     <= nextState;
        when others =>
          ustate <= init;
      end case;
    end if;
  end process;

end Behavioral;

