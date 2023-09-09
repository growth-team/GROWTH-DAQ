--UserModule_ConsumerMgr.vhdl
--
--SpaceWire Board / User FPGA / Modularized Structure Template
--UserModule / Consumer Manager
--
--ver20071022 Takayuki Yuasa
--file created
--based on UserModule_ChModule_Delay.vhdl (ver20071022)

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
entity UserModule_ConsumerManager_EventFIFO is
  generic(
    bufferDataCountWidth : integer := 11;
    InitialAddress       : std_logic_vector(15 downto 0);
    FinalAddress         : std_logic_vector(15 downto 0)
    );
  port(
    --signals connected to BusController
    BusIF2BusController         : out iBus_Signals_BusIF2BusController;
    BusController2BusIF         : in  iBus_Signals_BusController2BusIF;
    --signals connected to ConsumerModule
    Consumer2ConsumerMgr_vector : in  Signal_Consumer2ConsumerMgr_Vector(NumberOfConsumerNodes-1 downto 0);
    ConsumerMgr2Consumer_vector : out Signal_ConsumerMgr2Consumer_Vector(NumberOfConsumerNodes-1 downto 0);
    ---------------------------------------------
    -- Event FIFO signals
    ---------------------------------------------
    EventFIFOWriteData             : out std_logic_vector(15 downto 0);
    EventFIFOWriteEnable      : out std_logic;
    EventFIFOFull             : in  std_logic;
    EventFIFOReset      : out std_logic;
    --clock and reset
    Clock                       : in  std_logic;
    GlobalReset                 : in  std_logic
    );
end UserModule_ConsumerManager_EventFIFO;

---------------------------------------------------
--Behavioral description
---------------------------------------------------
architecture Behavioral of UserModule_ConsumerManager_EventFIFO is

  component fifo16x1k
    port (
      clk : in std_logic;
      srst : in std_logic;
      din : in std_logic_vector(15 downto 0);
      wr_en : in std_logic;
      rd_en : in std_logic;
      dout : out std_logic_vector(15 downto 0);
      full : out std_logic;
      empty : out std_logic;
      data_count : out std_logic_vector(9 downto 0)
    );
  end component;

  ---------------------------------------------------
  --Declarations of Signals
  ---------------------------------------------------
  --Signals used in iBus process
  signal BusIF2UserModule : iBus_Signals_BusIF2UserModule;
  signal UserModule2BusIF : iBus_Signals_UserModule2BusIF;
  --Fifo signals
  signal FifoWriteEnable  : std_logic                    := '0';
  signal FifoEmpty        : std_logic                    := '0';
  signal FifoFull         : std_logic                    := '0';
  signal FifoReadEnable   : std_logic                    := '0';
  signal FifoDataCount    : std_logic_vector(9 downto 0) := (others => '0');
  signal FifoDataIn       : std_logic_vector(EventPackeBufferDataWidth-1 downto 0);
  signal FifoDataOut      : std_logic_vector(EventPackeBufferDataWidth-1 downto 0);

  signal LoopI     : integer range 0 to NumberOfConsumerNodes := 0;
  signal Granting  : std_logic                                := '0';
  signal ResetDone : std_logic                                := '0';

  --Registers
  signal EventOutputDisableRegister            : std_logic_vector(15 downto 0) := (others => '0');
  signal GateSize_FastGate_Register            : std_logic_vector(15 downto 0) := x"0001";
  signal GateSize_SlowGate_Register            : std_logic_vector(15 downto 0) := x"0001";
  signal NumberOf_BaselineSample_Register      : std_logic_vector(15 downto 0) := x"0001";
  signal EventPacket_NumberOfWaveform_Register : std_logic_vector(15 downto 0) := x"0010";
  signal ResetRegister                         : std_logic_vector(15 downto 0) := x"0000";

  --ibus related signal
  signal ibusSendAddress      : std_logic_vector(15 downto 0);
  signal ibusSendData         : std_logic_vector(15 downto 0);
  signal ibusSendGo           : std_logic;
  signal ibusSendDone         : std_logic;
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

  type iBus_beWritten_StateMachine_State is
    (Initialize, Idle, DataReceive, WaitDone, WaitResetDone);
  signal iBus_beWritten_state : iBus_beWritten_StateMachine_State := Initialize;

  type iBus_beRead_StateMachine_State is
    (Initialize, Idle, WaitDone);
  signal iBus_beRead_state : iBus_beRead_StateMachine_State := Initialize;

  type UserModule_StateMachine_State is
    (Initialize, Initialize_2, Idle, WaitReset, Grant,  --
     TransferToEventFIFO0, TransferToEventFIFO1, TransferToEventFIFO2, Finalize);
  signal UserModule_state : UserModule_StateMachine_State := Initialize;

  signal Reset : std_logic := '0';

---------------------------------------------------
--Beginning of behavioral description
---------------------------------------------------
begin

  ---------------------------------------------------
  --Instantiations of Components
  ---------------------------------------------------

  Reset <= not GlobalReset;

  instanceOfFIFO : fifo16x1k
    port map (
      srst          => Reset,
      clk           => Clock,
      din           => FiFoDataIn,
      wr_en         => FifoWriteEnable,
      rd_en         => FifoReadEnable,
      dout          => FifoDataOut,
      full          => FifoFull,
      empty         => FifoEmpty,
      data_count    => FifoDataCount
      );

  ---------------------------------------------------
  --Static relationships
  ---------------------------------------------------
  FifoDataIn      <= Consumer2ConsumerMgr_vector(LoopI).Data        when LoopI /= NumberOfConsumerNodes else (others => '0');
  FifoWriteEnable <= Consumer2ConsumerMgr_vector(LoopI).WriteEnable when LoopI /= NumberOfConsumerNodes else '0';

  Connection : for I in 0 to NumberOfConsumerNodes-1 generate
    ConsumerMgr2Consumer_vector(I).GateSize_FastGate            <= GateSize_FastGate_Register;
    ConsumerMgr2Consumer_vector(I).GateSize_SlowGate            <= GateSize_SlowGate_Register;
    ConsumerMgr2Consumer_vector(I).EventPacket_NumberOfWaveform <= EventPacket_NumberOfWaveform_Register;
    ConsumerMgr2Consumer_vector(I).NumberOf_BaselineSample      <= NumberOf_BaselineSample_Register;
  end generate;

  EventFIFOReset <= ResetRegister(0);

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
          ResetDone        <= '0';
          LoopI            <= 0;
          UserModule_state <= Initialize_2;
        when Initialize_2 =>
          if (LoopI = NumberOfConsumerNodes) then
            LoopI            <= 0;
            UserModule_state <= Idle;
          else
            ConsumerMgr2Consumer_vector(LoopI).Grant <= '0';
            LoopI                                    <= LoopI + 1;
          end if;
        when Idle =>
          if (LoopI = NumberOfConsumerNodes) then
            LoopI <= 0;
          else
            if (ResetRegister(0) = '1') then
              ResetDone        <= '1';
              UserModule_state <= WaitReset;
            elsif (EventOutputDisableRegister(0) = '0' and Consumer2ConsumerMgr_vector(LoopI).EventReady = '1') then
              ConsumerMgr2Consumer_vector(LoopI).Grant <= '1';
              Granting                                 <= '1';
              UserModule_state                         <= Grant;
            else
              LoopI <= LoopI + 1;
            end if;
          end if;
        when WaitReset =>
          if (ResetRegister(0) = '0') then
            ResetDone        <= '0';
            UserModule_state <= Initialize;
          end if;
        when Grant =>
          if (Consumer2ConsumerMgr_vector(LoopI).EventReady = '0') then
            Granting                                 <= '0';
            ConsumerMgr2Consumer_vector(LoopI).Grant <= '0';
            UserModule_state                         <= TransferToEventFIFO0;
          end if;
        when TransferToEventFIFO0 =>
          EventFIFOWriteEnable <= '0';
          if (FifoEmpty = '1') then
            UserModule_state <= Finalize;
          else
            FifoReadEnable   <= '1';
            UserModule_state <= TransferToEventFIFO1;
          end if;
        when TransferToEventFIFO1 =>
          FifoReadEnable   <= '0';
          UserModule_state <= TransferToEventFIFO2;
        when TransferToEventFIFO2 =>
          EventFIFOWriteData <= FifoDataOut;
          if(EventFIFOFull = '0')then
            EventFIFOWriteEnable <= '1';
            UserModule_state       <= TransferToEventFIFO0;
          end if;
        when Finalize =>
          UserModule_state <= Idle;
        when others =>
          UserModule_state <= Initialize;
      end case;
    end if;
  end process;

  --change Register value by receiving data
  --from BusIF's ReceiveFIFO
  iBus_BeWritten_Process : process (Clock, GlobalReset)
  begin
    --is this process invoked with GlobalReset?
    if (GlobalReset = '0') then
      --Initialize StateMachine's state
      iBus_beWritten_state <= Initialize;
      --is this process invoked with Clock Event?
    elsif (Clock'event and Clock = '1') then
      case iBus_beWritten_state is
        when Initialize =>
          ibusBeWrittenDone    <= '0';
                                        --move to next state
          iBus_beWritten_state <= Idle;
        when Idle =>
          if (ibusBeWritten = '1') then
            iBus_beWritten_state <= DataReceive;
          end if;
        when DataReceive =>
          ibusBeWrittenDone        <= '1';
                                        --interpret the address of the received data
          case ibusBeWrittenAddress is
            when AddressOf_EventOutputDisableRegister =>
              EventOutputDisableRegister(0) <= iBusBeWrittenData(0);
                                        --move to next state
              iBus_beWritten_state <= WaitDone;
            when AddressOf_GateSize_FastGate_Register =>
              GateSize_FastGate_Register <= iBusBeWrittenData;
                                        --move to next state
              iBus_beWritten_state <= WaitDone;
            when AddressOf_GateSize_SlowGate_Register =>
              GateSize_SlowGate_Register <= iBusBeWrittenData;
                                        --move to next state
              iBus_beWritten_state <= WaitDone;
            when AddressOf_NumberOf_BaselineSample_Register =>
              NumberOf_BaselineSample_Register <= iBusBeWrittenData;
                                        --move to next state
              iBus_beWritten_state <= WaitDone;
            when AddressOf_ResetRegister =>
              ResetRegister(0)     <= '1';
                                        --move to next state
              iBus_beWritten_state <= WaitResetDone;
            when AddressOf_EventPacket_NumberOfWaveform_Register =>
              EventPacket_NumberOfWaveform_Register <= iBusBeWrittenData;
                                        --move to next state
              iBus_beWritten_state <= WaitDone;
            when others =>
                                        --no corresponding address or register
                                        --move to next state
              iBus_beWritten_state <= WaitDone;
          end case;
        when WaitDone =>                                                                                                            --wait until the "beRead" process completes
          if (ibusBeWritten = '0') then
            ibusBeWrittenDone        <= '0';
                                        --move to next state
            iBus_beWritten_state <= Idle;
          end if;
        when WaitResetDone =>
          if (ResetDone = '1') then
            ResetRegister(0)     <= '0';
            iBus_beWritten_state <= Initialize;
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
          if (iBusbeRead = '1') then
            case iBusbeReadAddress is
              when AddressOf_EventOutputDisableRegister =>
                iBusBeReadData(15 downto 1) <= (others => '0');
                iBusBeReadData(0)           <= EventOutputDisableRegister(0);
              when AddressOf_EventPacket_NumberOfWaveform_Register =>
                iBusBeReadData <= EventPacket_NumberOfWaveform_Register;
              when others =>
                                        --sonzai shina address heno yomikomi datta tokiha
                                        --0xabcd toiu tekitou na value wo kaeshite oku kotoni shitearu
                iBusBeReadData <= x"abcd";
            end case;
                                        --tell completion of the "beRead" process to iBus_BusIF
            iBusBeReadDone    <= '1';
                                        --move to next state
            iBus_beRead_state <= WaitDone;
          end if;
        when WaitDone =>
                                        --wait until the "beRead" process completes
          if (iBusbeRead = '0') then
            iBusBeReadDone    <= '0';
                                        --move to next state
            iBus_beRead_state <= Idle;
          end if;
        when others =>
                                        --move to next state
          iBus_beRead_state <= Initialize;
      end case;
    end if;
  end process;
  ---------------------------------------------
  -- Instantiate
  ---------------------------------------------
  instanceOfiBus_BusIFLite : entity work.iBus_BusIFLite
    generic map(
      InitialAddress => InitialAddress,
      FinalAddress   => FinalAddress
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
