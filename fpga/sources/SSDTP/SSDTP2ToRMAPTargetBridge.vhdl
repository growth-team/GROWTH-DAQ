library IEEE, work;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_ARITH.all;
use IEEE.std_logic_UNSIGNED.all;
use work.SSDTP2Library.all;
use work.SpaceWireCODECIPPackage.all;
use work.RMAPTargetIPPackage.all;

library UNISIM;
use UNISIM.VComponents.all;

entity SSDTP2ToRMAPTargetBridge is
  generic (
    gBusWidth : integer range 8 to 32 := 32;  -- 8 = 8bit, 16 = 16bit, 32 = 32bit
    bufferDataCountWidth : integer := 10
    );  
  port(
    -- clock and reset
    clock                       : in  std_logic;
    reset                       : in  std_logic;
    ---------------------------------------------
    -- SocketVHDL signals
    ---------------------------------------------
    tcpSendFIFOData             : out std_logic_vector(7 downto 0);
    tcpSendFIFOWriteEnable      : out std_logic;
    tcpSendFIFOFull             : in  std_logic;
    tcpReceiveFIFOEmpty         : in  std_logic;
    tcpReceiveFIFOData          : in  std_logic_vector(7 downto 0);
    tcpReceiveFIFODataCount     : in  std_logic_vector(bufferDataCountWidth-1 downto 0);
    tcpReceiveFIFOReadEnable    : out std_logic;
    ---------------------------------------------
    -- RMAPTarget signals 
    ---------------------------------------------
    --Internal BUS 
    busMasterCycleOut           : out std_logic;
    busMasterStrobeOut          : out std_logic;
    busMasterAddressOut         : out std_logic_vector (31 downto 0);
    busMasterByteEnableOut      : out std_logic_vector ((gBusWidth/8)-1 downto 0);
    busMasterDataIn             : in  std_logic_vector (gBusWidth-1 downto 0);
    busMasterDataOut            : out std_logic_vector (gBusWidth-1 downto 0);
    busMasterWriteEnableOut     : out std_logic;
    busMasterReadEnableOut      : out std_logic;
    busMasterAcknowledgeIn      : in  std_logic;
    busMasterTimeOutErrorIn     : in  std_logic;
    -- RMAP Statemachine state                                     
    commandStateOut             : out commandStateMachine;
    replyStateOut               : out replyStateMachine;
    -- RMAP_User_Decode
    rmapLogicalAddressOut       : out std_logic_vector(7 downto 0);
    rmapCommandOut              : out std_logic_vector(3 downto 0);
    rmapKeyOut                  : out std_logic_vector(7 downto 0);
    rmapAddressOut              : out std_logic_vector(31 downto 0);
    rmapDataLengthOut           : out std_logic_vector(23 downto 0);
    requestAuthorization        : out std_logic;
    authorizeIn                 : in  std_logic;
    rejectIn                    : in  std_logic;
    replyStatusIn               : in  std_logic_vector(7 downto 0);
    -- RMAP Error Code and Status
    rmapErrorCode               : out std_logic_vector(7 downto 0);
    -- SSDTP2 state out
    stateOutSSDTP2TCPToSpaceWire: out std_logic_vector(7 downto 0);
    stateOutSSDTP2SpaceWireToTCP: out std_logic_vector(7 downto 0);
    -- statistics                                    
    statisticalInformationClear : in  std_logic;
    statisticalInformation      : out bit32X8Array
    );
end SSDTP2ToRMAPTargetBridge;

architecture Behavioral of SSDTP2ToRMAPTargetBridge is
  component SSDTP2TCPToSpaceWire is
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
  end component;

  component SSDTP2SpaceWireToTCP is
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
  end component;

  component RMAPTargetIPDecoder is
    generic (
      gBusWidth : integer range 8 to 32 := 32  -- 8 = 8bit, 16 = 16bit, 32 = 32bit,
      );
    port (
      clock                    : in  std_logic;
      reset                    : in  std_logic;
      -- FIFO
      transmitFIFOWriteEnable  : out std_logic;
      transmitFIFODataIn       : out std_logic_vector (8 downto 0);
      transmitFIFOFull         : in  std_logic;
      receiveFIFOReadEnable    : out std_logic;
      receiveFIFODataOut       : in  std_logic_vector (8 downto 0);
      receiveFIFODataCount     : in  std_logic_vector (5 downto 0);
      -- dma
      rmapAddress              : out std_logic_vector (31 downto 0);
      rmapDataLength           : out std_logic_vector (23 downto 0);
      dmaReadBufferWriteEnable : in  std_logic;
      dmaWriteBufferReadEnable : in  std_logic;
      dmaReadBufferWriteReady  : out std_logic;
      dmaReadBufferReadReady   : out std_logic;
      dmaReadBufferWriteData   : in  std_logic_vector (7 downto 0);
      dmaWriteBufferReadData   : out std_logic_vector (7 downto 0);
      dmaWriteBufferEmpty      : out std_logic;
      rmapCommand              : out std_logic_vector (3 downto 0);
      busAccessStart           : out std_logic;
      busAccessStop            : out std_logic;
      busAccessEnd             : in  std_logic;
      busMasterTimeOutErrorIn  : in  std_logic;
      -- RMAP State                                                 
      commandStateOut          : out commandStateMachine;
      replyStateOut            : out replyStateMachine;
      -- RMAP_User_Decode
      rmapLogicalAddressOut    : out std_logic_vector(7 downto 0);
      rmapKeyOut               : out std_logic_vector(7 downto 0);
      -- RMAP Transaction control
      requestAuthorization     : out std_logic;
      authorizeIn              : in  std_logic;
      rejectIn                 : in  std_logic;
      replyStatusIn            : in  std_logic_vector(7 downto 0);
      -- RMAP Error Code and Status
      rmapErrorCode            : out std_logic_vector(7 downto 0);
      errorIndication          : out std_logic;
      writeDataIndication      : out std_logic;
      readDataIndication       : out std_logic;
      rmwDataIndication        : out std_logic
      );
  end component;

  component RMAPTargetIPDMAController is
    generic (
      gBusWidth : integer range 8 to 32 := 32  -- 8 = 8bit, 16 = 16bit, 32 = 32bit,
      );
    port (
      clock                    : in  std_logic;
      reset                    : in  std_logic;
      -- dma i/f
      rmapAddress              : in  std_logic_vector (31 downto 0);
      rmapDataLength           : in  std_logic_vector (23 downto 0);
      dmaReadBufferWriteEnable : out std_logic;
      dmaWriteBufferReadEnable : out std_logic;
      dmaReadBufferWriteReady  : in  std_logic;
      dmaReadBufferWriteData   : out std_logic_vector (7 downto 0);
      dmaWriteBufferReadData   : in  std_logic_vector (7 downto 0);
      dmaWriteBufferEmpty      : in  std_logic;
      rmapCommand              : in  std_logic_vector (3 downto 0);
      busAccessStart           : in  std_logic;
      busAccessStop            : in  std_logic;
      busAccessEnd             : out std_logic;
      busMasterTimeOutErrorIn  : in  std_logic;
      -- bus i/f
      busMasterCycleOut        : out std_logic;
      busMasterStrobeOut       : out std_logic;
      busMasterAddressOut      : out std_logic_vector (31 downto 0);
      busMasterByteEnableOut   : out std_logic_vector (3 downto 0);
      busMasterDataIn          : in  std_logic_vector (31 downto 0);
      busMasterDataOut         : out std_logic_vector (31 downto 0);
      busMasterWriteEnableOut  : out std_logic;
      busMasterReadEnableOut   : out std_logic;
      busMasterAcknowledgeIn   : in  std_logic
      );
  end component;
  
  component fifo9x1k
    port (
      clk : IN STD_LOGIC;
      rst : IN STD_LOGIC;
      din : IN STD_LOGIC_VECTOR(8 DOWNTO 0);
      wr_en : IN STD_LOGIC;
      rd_en : IN STD_LOGIC;
      dout : OUT STD_LOGIC_VECTOR(8 DOWNTO 0);
      full : OUT STD_LOGIC;
      empty : OUT STD_LOGIC
    );
  end component;

  --
  signal transmitFIFOWriteEnable      : std_logic := '0';
  signal transmitFIFODataIn           : std_logic_vector (8 downto 0) := (others => '0');
  signal transmitFIFOFull             : std_logic := '0';
  signal receiveFIFOReadEnable        : std_logic := '0';
  signal receiveFIFODataOut           : std_logic_vector (8 downto 0) := (others => '0');
  signal receiveFIFODataCount         : std_logic_vector (5 downto 0) := (others => '0');
  --
  signal rmapAddress                  : std_logic_vector (31 downto 0) := (others => '0');
  signal dmaReadBufferWriteEnable     : std_logic := '0';
  signal dmaWriteBufferReadEnable     : std_logic := '0';
  signal dmaReadBufferWriteReady      : std_logic := '0';
  signal rmapDataLength               : std_logic_vector (23 downto 0) := (others => '0');
  signal dmaReadBufferWriteData       : std_logic_vector (7 downto 0) := (others => '0');
  signal dmaWriteBufferReadData       : std_logic_vector (7 downto 0) := (others => '0');
  signal dmaWriteBufferEmpty          : std_logic := '0';
  signal rmapCommand                  : std_logic_vector (3 downto 0) := (others => '0');
  signal busAccessStart               : std_logic := '0';
  signal busAccessStop                : std_logic := '0';
  signal busAccessEnd                 : std_logic := '0';
  --
  signal busMasterByteEnableOutSignal : std_logic_vector (3 downto 0) := (others => '0');
  signal busMasterDataOutSignal       : std_logic_vector (31 downto 0) := (others => '0');
  signal iBusMasterDataIn             : std_logic_vector (31 downto 0) := (others => '0');

  -- SpaceWireCODEC signals
  signal rmap2ssdtpFIFOReadEnable   : std_logic := '0';
  signal rmap2ssdtpFIFODataOut      : std_logic_vector(8 downto 0) := (others => '0');
  signal rmap2ssdtpFIFOEmpty        : std_logic := '0';
  signal rmap2ssdtpFIFOFull         : std_logic := '0';
  signal rmap2ssdtpFIFOWriteEnable : std_logic := '0';
  signal rmap2ssdtpFIFODataIn      : std_logic_vector (8 downto 0) := (others => '0');

  signal ssdtp2rmapFIFODataCount    : std_logic_vector(5 downto 0) := (others => '0');
  signal ssdtp2rmapFIFOReadEnable   : std_logic := '0';
  signal ssdtp2rmapFIFODataOut      : std_logic_vector(8 downto 0) := (others => '0');
  signal ssdtp2rmapFIFOEmpty        : std_logic := '0';
  signal ssdtp2rmapFIFOWriteEnable : std_logic := '0';
  signal ssdtp2rmapFIFODataIn      : std_logic_vector (8 downto 0) := (others => '0');
  signal ssdtp2rmapFIFOFull        : std_logic := '0';

  -- For ILA
--  attribute mark_debug : string;
--  attribute keep : string;
--  attribute mark_debug of ssdtp2rmapFIFOWriteEnable     : signal is "true";
--  attribute mark_debug of commandStateOut  : signal is "true";
--  attribute mark_debug of rmapAddress  : signal is "true";
--  attribute mark_debug of rmapDataLength  : signal is "true";
--  attribute mark_debug of rmapCommand  : signal is "true";
--  attribute mark_debug of rmapErrorCode  : signal is "true";
--  attribute mark_debug of busAccessStart  : signal is "true";
--  attribute mark_debug of busMasterCycleOut  : signal is "true";
--  attribute mark_debug of busMasterTimeOutErrorIn  : signal is "true";
--  attribute mark_debug of replyStateOut  : signal is "true";
--  attribute mark_debug of rmap2ssdtpFIFOFull  : signal is "true";
--  attribute mark_debug of rmap2ssdtpFIFOReadEnable  : signal is "true";
  
begin

  ---------------------------------------------
  -- Instantiation
  ---------------------------------------------
  ssdtp2_tcp2spw : SSDTP2TCPToSpaceWire
    generic map(
        bufferDataCountWidth => bufferDataCountWidth
      )
    port map(
      tcpReceiveFIFOEmpty        => tcpReceiveFIFOEmpty,
      tcpReceiveFIFODataOut      => tcpReceiveFIFOData,
      tcpReceiveFIFODataCount    => tcpReceiveFIFODataCount,
      tcpReceiveFIFOReadEnable   => tcpReceiveFIFOReadEnable,
      spwTransmitFIFOWriteEnable => ssdtp2rmapFIFOWriteEnable,
      spwTransmitFIFODataIn      => ssdtp2rmapFIFODataIn,
      spwTransmitFIFOFull        => ssdtp2rmapFIFOFull,
      tickInOut                  => open,
      timeInOut                  => open,
      txClockDivideCount         => open,
      stateOut                   => stateOutSSDTP2TCPToSpaceWire,
      clock                      => clock,
      reset                      => reset
      );

  ssdtp2_spw2tcp : SSDTP2SpaceWireToTCP
    port map(
      spwReceiveFIFOReadEnable => rmap2ssdtpFIFOReadEnable,
      spwReceiveFIFODataOut    => rmap2ssdtpFIFODataOut,
      spwReceiveFIFOEmpty      => rmap2ssdtpFIFOEmpty,
      spwReceiveFIFOFull       => rmap2ssdtpFIFOFull,
      tickOutIn                => '0',
      timeOutIn                => "000000",
      -- SocketVHDL signals
      tcpSendFIFODataOut       => tcpSendFIFOData,
      tcpSendFIFOWriteEnable   => tcpSendFIFOWriteEnable,
      tcpSendFIFOFull          => tcpSendFIFOFull,
      stateOut                 => stateOutSSDTP2SpaceWireToTCP,
      clock                    => clock,
      reset                    => reset
      );

  rmapDecoder : RMAPTargetIPDecoder
    generic map (
      gBusWidth => gBusWidth
      )
    port map (
      clock                    => clock,
      reset                    => reset,
      -- fifo
      transmitFIFOWriteEnable  => rmap2ssdtpFIFOWriteEnable,
      transmitFIFODataIn       => rmap2ssdtpFIFODataIn,
      transmitFIFOFull         => rmap2ssdtpFIFOFull,
      receiveFIFOReadEnable    => ssdtp2rmapFIFOReadEnable,
      receiveFIFODataOut       => ssdtp2rmapFIFODataOut,
      receiveFIFODataCount     => ssdtp2rmapFIFODataCount,
      -- dma
      rmapAddress              => rmapAddress,
      rmapDataLength           => rmapDataLength,
      dmaReadBufferWriteEnable => dmaReadBufferWriteEnable,
      dmaWriteBufferReadEnable => dmaWriteBufferReadEnable,
      dmaReadBufferWriteReady  => dmaReadBufferWriteReady,
      dmaReadBufferReadReady   => open,
      dmaReadBufferWriteData   => dmaReadBufferWriteData,
      dmaWriteBufferReadData   => dmaWriteBufferReadData,
      dmaWriteBufferEmpty      => dmaWriteBufferEmpty,
      rmapCommand              => rmapCommand,
      busAccessStart           => busAccessStart,
      busAccessStop            => busAccessStop,
      busAccessEnd             => busAccessEnd,
      busMasterTimeOutErrorIn  => busMasterTimeOutErrorIn,
      -- RMAP State       
      commandStateOut          => commandStateOut,
      replyStateOut            => replyStateOut,
      -- RMAP_User_Decode               
      rmapLogicalAddressOut    => rmapLogicalAddressOut,
      rmapKeyOut               => rmapKeyOut,
      -- RMAP Transaction control
      requestAuthorization     => requestAuthorization,
      authorizeIn              => authorizeIn,
      rejectIn                 => rejectIn,
      replyStatusIn            => replyStatusIn,
      -- RMAP Error Code and Status
      rmapErrorCode            => rmapErrorCode,
      errorIndication          => open,
      writeDataIndication      => open,
      readDataIndication       => open,
      rmwDataIndication        => open
      );

  rmapCommandOut    <= rmapCommand;
  rmapAddressOut    <= rmapAddress;
  rmapDataLengthOut <= rmapDataLength;

  rmapDMAController : RMAPTargetIPDMAController
    generic map (gBusWidth => gBusWidth)
    port map (
      clock                    => clock,
      reset                    => reset,
      -- dma i/f
      rmapAddress              => rmapAddress,
      rmapDataLength           => rmapDataLength,
      dmaReadBufferWriteEnable => dmaReadBufferWriteEnable,
      dmaWriteBufferReadEnable => dmaWriteBufferReadEnable,
      dmaReadBufferWriteReady  => dmaReadBufferWriteReady,
      dmaReadBufferWriteData   => dmaReadBufferWriteData,
      dmaWriteBufferReadData   => dmaWriteBufferReadData,
      dmaWriteBufferEmpty      => dmaWriteBufferEmpty,
      rmapCommand              => rmapCommand,
      busAccessStart           => busAccessStart,
      busAccessStop            => busAccessStop,
      busAccessEnd             => busAccessEnd,
      -- bus i/f
      busMasterCycleOut        => busMasterCycleOut,
      busMasterStrobeOut       => busMasterStrobeOut,
      busMasterAddressOut      => busMasterAddressOut,
      busMasterByteEnableOut   => busMasterByteEnableOutSignal,
      busMasterDataIn          => iBusMasterDataIn,
      busMasterDataOut         => busMasterDataOutSignal,
      busMasterWriteEnableOut  => busMasterWriteEnableOut,
      busMasterReadEnableOut   => busMasterReadEnableOut,
      busMasterAcknowledgeIn   => busMasterAcknowledgeIn,
      busMasterTimeOutErrorIn  => busMasterTimeOutErrorIn
      );

  BusWidth8 : if (gBusWidth = 8) generate
    iBusMasterDataIn       <= x"000000" & busMasterDataIn(7 downto 0);
    busMasterDataOut       <= busMasterDataOutSignal(7 downto 0);
    busMasterByteEnableOut <= busMasterByteEnableOutSignal(0 downto 0);
  end generate;

  BusWidth16 : if (gBusWidth = 16) generate
    iBusMasterDataIn       <= x"0000" & busMasterDataIn(15 downto 0);
    busMasterDataOut       <= busMasterDataOutSignal(15 downto 0);
    busMasterByteEnableOut <= busMasterByteEnableOutSignal(1 downto 0);
  end generate;

  BusWidth32 : if (gBusWidth = 32) generate
    iBusMasterDataIn       <= busMasterDataIn;
    busMasterDataOut       <= busMasterDataOutSignal;
    busMasterByteEnableOut <= busMasterByteEnableOutSignal;
  end generate;

  rmap2ssdtpFIFO : fifo9x1k
    port map(
      rst   => reset,
      clk   => clock,
      din   => rmap2ssdtpFIFODataIn,
      wr_en => rmap2ssdtpFIFOWriteEnable,
      rd_en => rmap2ssdtpFIFOReadEnable,
      dout  => rmap2ssdtpFIFODataOut,
      full  => rmap2ssdtpFIFOFull,
      empty => rmap2ssdtpFIFOEmpty
      );

  ssdtp2rmapFIFO : fifo9x1k
    port map(
      rst   => reset,
      clk   => clock,
      din   => ssdtp2rmapFIFODataIn,
      wr_en => ssdtp2rmapFIFOWriteEnable,
      rd_en => ssdtp2rmapFIFOReadEnable,
      dout  => ssdtp2rmapFIFODataOut,
      full  => ssdtp2rmapFIFOFull,
      empty => ssdtp2rmapFIFOEmpty
      );
    -- Below is to assert non-zero value to dataCount.
    -- (fifo9x1k does not have data count but RMAP Target IP requires it) 
    ssdtp2rmapFIFODataCount(0) <= not ssdtp2rmapFIFOEmpty;

end Behavioral;
