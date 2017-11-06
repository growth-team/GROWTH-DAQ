-- Takayuki Yuasa 20140703
-- - Created.

---------------------------------------------------
--Declarations of Libraries used in this UserModule
---------------------------------------------------
library ieee, work;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;
use work.iBus_Library.all;
use work.iBus_AddressMap.all;

---------------------------------------------------
--Entity Declaration of this UserModule
---------------------------------------------------

entity iBus_RMAPConnector is
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
    rmapBusMasterAddressOut     : in  std_logic_vector (31 downto 0);
    rmapBusMasterByteEnableOut  : in  std_logic_vector (1 downto 0);
    rmapBusMasterDataIn         : out std_logic_vector (15 downto 0);
    rmapBusMasterDataOut        : in  std_logic_vector (15 downto 0);
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

end iBus_RMAPConnector;

architecture Behavioral of iBus_RMAPConnector is

  component iBus_BusIFLite is
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
      GlobalReset : in std_logic        -- negative logic
      );
  end component;

  type RMAPReadProcessStates is (
    Idle, Sending, Reading, Finalize
    );
  signal rmapProcessState : RMAPReadProcessStates := Idle;

  --ibus related signal
  signal ibusSendAddress          : std_logic_vector(15 downto 0);
  signal ibusSendData             : std_logic_vector(15 downto 0);
  signal ibusSendGo               : std_logic;
  signal ibusSendDone             : std_logic;
  signal ibusReadAddress      : std_logic_vector(15 downto 0);
  signal ibusReadData         : std_logic_vector(15 downto 0);
  signal ibusReadGo           : std_logic;
  signal ibusReadDone         : std_logic;
  signal ibusBeReadAddress    : std_logic_vector(15 downto 0);
  signal ibusBeRead           : std_logic;
  signal ibusBeReadData       : std_logic_vector(15 downto 0);
  signal ibusBeReadDone       : std_logic;
  signal ibusBeWrittenAddress : std_logic_vector(15 downto 0);
  signal ibusBeWritten        : std_logic;
  signal ibusBeWrittenData    : std_logic_vector(15 downto 0);
  signal ibusBeWrittenDone    : std_logic;

  constant iBusBusWidth : integer := 16;

begin
  rmapProcessStateInteger <=  --
    0 when rmapProcessState=Idle else --
    1 when rmapProcessState=Sending else --
    2 when rmapProcessState=Reading else --
    3 when rmapProcessState=Finalize else 4;

  RMAPReadProcess : process(clock, GlobalReset)
  begin
    if(GlobalReset = '0')then
      rmapProcessState <= Idle;
      rmapBusMasterTimeOutErrorIn <= '0';
    else
      if(clock = '1' and clock'event)then

        --state machine
        if(BusController2BusIF.TimedOut='1')then
          ibusReadGo <= '0';
          ibusSendGo <= '0';
          rmapProcessState <= Idle;
          rmapBusMasterTimeOutErrorIn <= '1';
        else -- if not timeout
          rmapBusMasterTimeOutErrorIn <= '0';
          case rmapProcessState is
            when Idle =>
              if(rmapBusMasterCycleOut='1')then
                if(rmapBusMasterReadEnableOut = '1')then
                  ibusReadAddress  <= rmapBusMasterAddressOut(15 downto 0);
                  ibusReadGo       <= '1';
                  rmapProcessState <= Reading;
                elsif(rmapBusMasterWriteEnableOut = '1')then
                  ibusSendAddress  <= rmapBusMasterAddressOut(15 downto 0);
                  ibusSendData     <= rmapBusMasterDataOut(iBusBusWidth-1 downto 0);
                  ibusSendGo       <= '1';
                  rmapProcessState <= Sending;
                end if;
              else
                ibusReadGo <= '0';
                ibusSendGo <= '0';
              end if;
              rmapBusMasterAcknowledgeIn <= '0';
            when Reading =>
              if(ibusReadDone = '1')then
                ibusReadGo                                   <= '0';
                rmapBusMasterAcknowledgeIn                   <= '1';
                rmapBusMasterDataIn(iBusBusWidth-1 downto 0) <= ibusReadData;
                rmapProcessState                             <= Finalize;
              end if;
            when Sending =>
              if(ibusSendDone = '1')then
                ibusSendGo                 <= '0';
                rmapBusMasterAcknowledgeIn <= '1';
                rmapProcessState           <= Finalize;
              end if;
            when Finalize =>
              rmapBusMasterAcknowledgeIn <= '0';
              if(rmapBusMasterCycleOut='0')then
                rmapProcessState           <= Idle;
              end if;
            when others =>
              rmapProcessState <= Idle;
          end case;
        end if; --timeout
      end if;
    end if;
  end process;

  ---------------------------------------------
  -- Instantiate
  ---------------------------------------------
  instanceOfiBus_BusIFLite : iBus_BusIFLite
	 generic map(
		InitialAddress => InitialAddress,
		FinalAddress => FinalAddress
		)
    port map(
      BusIF2BusController => BusIF2BusController,
      BusController2BusIF => BusController2BusIF,
      SendAddress         => ibusSendAddress,
      SendData            => ibusSendData,
      SendGo              => ibusSendGo,
      SendDone            => ibusSendDone,
      ReadAddress         => ibusReadAddress,
      ReadData            => ibusReadData,
      ReadGo              => ibusReadGo,
      ReadDone            => ibusReadDone,
      beReadAddress       => ibusBeReadAddress,
      beRead              => ibusBeRead,
      beReadData          => ibusBeReadData,
      beReadDone          => ibusBeReadDone,
      beWrittenAddress    => ibusBeWrittenAddress,
      beWritten           => ibusBeWritten,
      beWrittenData       => ibusBeWrittenData,
      beWrittenDone       => ibusBeWrittenDone,
      Clock               => Clock,
      GlobalReset         => GlobalReset
      );

end Behavioral;
