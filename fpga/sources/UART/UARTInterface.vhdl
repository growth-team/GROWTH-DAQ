library IEEE, wor;
use IEEE.std_logic_1164.all;

entity UARTInterface is
  generic(
    InputClockPeriodInNanoSec : integer := 20;     -- ns
    BaudRate                  : integer := 115200  -- bps
    );
  port(
    Clock     : in  std_logic;  -- Clock input (tx/rx clocks will be internally generated)
    Reset     : in  std_logic;          -- Set '1' to reset this modlue
    TxSerial  : out std_logic;          -- Serial Tx output
    RxSerial  : in  std_logic;          -- Serial Rx input
    txDataIn  : in  std_logic_vector(7 downto 0);  -- Send data
    rxDataOut : out std_logic_vector(7 downto 0);  -- Received data
    txEnable  : in  std_logic;          -- Set '1' to send data in DataIn
    received  : out std_logic;          -- '1' when new DataOut is valid
    txReady   : out std_logic           -- '1' when Tx is not busy
    );
end UARTInterface;

architecture rtl of UARTInterface is
  
  component UART is
    generic(
      InputClockPeriodInNanoSec : integer := 20;     -- ns
      BaudRate                  : integer := 115200  -- bps
      );
    port(
      CLK      : in  std_logic;         -- クロック入力．周波数は50MHzを想定
      RST      : in  std_logic;         -- リセット動作: 1，通常動作: 0
      TXD      : out std_logic;         -- シリアル通信（送信）
      RXD      : in  std_logic;         -- シリアル通信（受信）
      DIN      : in  std_logic_vector(7 downto 0);   -- 送信データ入力
      DOUT     : out std_logic_vector(7 downto 0);   -- 受信データ出力
      EN_TX    : in  std_logic;         -- 送信有効: 1，送信無効: 0
      Received : out std_logic;
      STBY_TX  : out std_logic;         -- 待機中（初期状態）: 1，送信中: 0
      STBY_RX  : out std_logic);        -- 待機中（初期状態）: 1，受信中または受信待機中: 0
  end component;

  signal resetCount : integer range 0 to 7 :=0;
  signal internalReset : std_logic := '0';
  signal powerOnReset : std_logic := '0';

  signal iTxReady   : std_logic := '0';

begin

  internalReset <= Reset or powerOnReset;
  txReady <= iTxReady when internalReset='0' else '0';

  process(clock)
  begin
    if(clock'event and clock = '1') then
      if(resetCount=7)then
        powerOnReset <= '0';
      else
        resetCount <= resetCount + 1;
        powerOnReset <= '1';
      end if;
    end if;
  end process;


  uartInstance : UART
    generic map(
      InputClockPeriodInNanoSec => InputClockPeriodInNanoSec,
      BaudRate                  => BaudRate
      )
    port map(
      CLK      => Clock,
      RST      => internalReset,
      TXD      => TxSerial,
      RXD      => RxSerial,
      DIN      => txDataIn,             -- in
      DOUT     => rxDataOut,            -- out
      EN_TX    => txEnable,             -- in
      received => received,
      STBY_TX  => iTxReady,
      STBY_RX  => open
      );
end;





-- =============================================
-- The following source code of UART is taken from
-- http://keitetsu.ninja-web.net/fpga_uart_vhd.html
-- Clock Generator part was modified to support
-- various input frequencies and baud rates.
-- =============================================

--------------------------------------------------------------------------------
-- UART Interface Unit
--  Start-stop synchronous communication (RS-232C)
--  115200bps, 8bit, no-parity, 1stop-bit
--------------------------------------------------------------------------------

library IEEE, work;
use IEEE.std_logic_1164.all;
use work.all;

entity UART is
  generic(
    InputClockPeriodInNanoSec : integer := 20;     -- ns
    BaudRate                  : integer := 115200  -- bps
    );
  port(
    CLK      : in  std_logic;           -- クロック入力．周波数は50MHzを想定
    RST      : in  std_logic;           -- リセット動作: 1，通常動作: 0
    TXD      : out std_logic;           -- シリアル通信（送信）
    RXD      : in  std_logic;           -- シリアル通信（受信）
    DIN      : in  std_logic_vector(7 downto 0);   -- 送信データ入力
    DOUT     : out std_logic_vector(7 downto 0);   -- 受信データ出力
    EN_TX    : in  std_logic;           -- 送信有効: 1，送信無効: 0
    Received : out std_logic;           -- 1バイト受信時1クロックだけ'1' 待機時='0'
    STBY_TX  : out std_logic;           -- 待機中（初期状態）: 1，送信中: 0
    STBY_RX  : out std_logic);          -- 待機中（初期状態）: 1，受信中または受信待機中: 0
end UART;

architecture rtl of UART is
  signal CLK_TX : std_logic;
  signal CLK_RX : std_logic;

  component clk_generator
    generic(
      InputClockPeriodInNanoSec : integer := 20;     -- ns
      BaudRate                  : integer := 115200  -- bps
      );
    port(
      clk    : in  std_logic;
      rst    : in  std_logic;
      clk_tx : out std_logic;
      clk_rx : out std_logic);
  end component;

  component tx is port(
    clk    : in  std_logic;
    rst    : in  std_logic;
    clk_tx : in  std_logic;
    txd    : out std_logic;
    din    : in  std_logic_vector(7 downto 0);
    en     : in  std_logic;
    stby   : out std_logic);
  end component;

  component rx is port(
    clk      : in  std_logic;
    rst      : in  std_logic;
    clk_rx   : in  std_logic;
    rxd      : in  std_logic;
    dout     : out std_logic_vector(7 downto 0);
    received : out std_logic;
    stby     : out std_logic);
  end component;

  
begin
  uclk_generator : clk_generator
    generic map(
      InputClockPeriodInNanoSec => InputClockPeriodInNanoSec,
      BaudRate                  => BaudRate
      )
    port map(
      clk    => CLK,
      rst    => RST,
      clk_tx => clk_tx,
      clk_rx => clk_rx
      );

  utx : tx
  port map(
    clk    => CLK,
    rst    => RST,
    clk_tx => clk_tx,
    txd    => TXD,
    din    => DIN,
    en     => EN_TX,
    stby   => STBY_TX
  );

  urx : rx
  port map(
    clk      => CLK,
    rst      => RST,
    clk_rx   => clk_rx,
    rxd      => RXD,
    dout     => DOUT,
    received => received,
    stby     => STBY_RX
  );

end rtl;


---------------------------------------------
-- Clock Generator
---------------------------------------------
library IEEE, work;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_unsigned.all;

-- clock generator module
entity clk_generator is
  generic(
    InputClockPeriodInNanoSec : integer := 20;     -- ns
    BaudRate                  : integer := 115200  -- bps
    );
  port(
    clk    : in  std_logic;
    rst    : in  std_logic;
    clk_tx : out std_logic;
    clk_rx : out std_logic
    );
end clk_generator;

architecture rtl of clk_generator is
  -- original
  -- (1 / 115200bps) / (1 / xx MHz) = 434
  -- modified
  -- (10^9 / BaudRate) / (InputClockPeriodInNanoSec) 
  constant txClockCounterMax : integer := (1000000000 / BaudRate) / InputClockPeriodInNanoSec;
  signal   cnt_tx            : integer range 0 to txClockCounterMax;
  -- original
  -- (1 / 115200bps) / (1 / xx MHz) / 16 = 27
  constant rxClockCounterMax : integer := (1000000000 / BaudRate) / InputClockPeriodInNanoSec / 16;
  signal   cnt_rx            : integer range 0 to rxClockCounterMax;
  
begin
  clk_tx <= '1' when(cnt_tx = txClockCounterMax) else '0';
  process(clk, rst)
  begin
    if(rst = '1') then
      cnt_tx <= 0;
    elsif(clk'event and clk = '1') then
      if(cnt_tx = txClockCounterMax) then
        cnt_tx <= 0;
      else
        cnt_tx <= cnt_tx + 1;
      end if;
    end if;
  end process;

  clk_rx <= '1' when(cnt_rx = rxClockCounterMax) else '0';
  process(clk, rst)
  begin
    if(rst = '1') then
      cnt_rx <= 0;
    elsif(clk'event and clk = '1') then
      if(cnt_rx = rxClockCounterMax) then
        cnt_rx <= 0;
      else
        cnt_rx <= cnt_rx + 1;
      end if;
    end if;
  end process;
  
end rtl;


---------------------------------------------
-- Transmitter Module
---------------------------------------------
library IEEE, work;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_unsigned.all;

-- transmitter module
entity tx is port(
  clk    : in  std_logic;
  rst    : in  std_logic;
  clk_tx : in  std_logic;
  txd    : out std_logic;
  din    : in  std_logic_vector(7 downto 0);
  en     : in  std_logic;
  stby   : out std_logic);
end tx;

architecture rtl of tx is
  signal en_tmp  : std_logic;
  signal cnt_bit : integer range 0 to 9;
  signal buf     : std_logic_vector(7 downto 0);
  
begin
  process(clk, rst)
  begin
    if(rst = '1') then
      txd     <= '1';
      en_tmp  <= '0';
      stby    <= '1';
      cnt_bit <= 0;
      buf     <= (others => '0');
    elsif(clk'event and clk = '1') then
      if(en = '1') then
        en_tmp <= '1';
        stby   <= '0';
        buf    <= din;
      elsif(clk_tx = '1' and en_tmp = '1') then
        case cnt_bit is
          when 0 =>
            txd     <= '0';
            cnt_bit <= cnt_bit + 1;
          when 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 =>
            txd     <= buf(0);
            buf     <= '1' & buf(7 downto 1);
            cnt_bit <= cnt_bit + 1;
          when others =>
            txd     <= '1';
            en_tmp  <= '0';
            stby    <= '1';
            cnt_bit <= 0;
        end case;
      end if;
    end if;
  end process;
end rtl;


---------------------------------------------
-- Receive Module
---------------------------------------------
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_unsigned.all;

-- receiver module
entity rx is port(
  clk      : in  std_logic;
  rst      : in  std_logic;
  clk_rx   : in  std_logic;
  rxd      : in  std_logic;
  dout     : out std_logic_vector(7 downto 0);
  received : out std_logic;
  stby     : out std_logic);
end rx;

architecture rtl of rx is
  type   state_type is (idle, detect, proc, stopbit);
  signal state        : state_type;
  signal dout_reg     : std_logic_vector(7 downto 0);
  signal cnt_bitnum   : integer range 0 to 7;
  signal cnt_bitwidth : integer range 0 to 15;
  signal buf          : std_logic;
  
begin
  dout <= dout_reg;

  process(clk, rst)
  begin
    if(rst = '1') then
      buf <= '0';
    elsif(clk'event and clk = '1') then
      buf <= rxd;
    end if;
  end process;

  process(clk, rst)
  begin
    if(rst = '1') then
      stby         <= '1';
      dout_reg     <= (others => '0');
      cnt_bitnum   <= 0;
      cnt_bitwidth <= 0;
      state        <= idle;
    elsif(clk'event and clk = '1') then
      case state is
        when idle =>
          received <= '0';
          if(buf = '0') then
            cnt_bitnum   <= 0;
            cnt_bitwidth <= 0;
            stby         <= '0';
            state        <= detect;
          else
            state <= state;
          end if;
        when detect =>
          if(clk_rx = '1') then
            if(cnt_bitwidth = 7) then
              cnt_bitwidth <= 0;
              state        <= proc;
            else
              cnt_bitwidth <= cnt_bitwidth + 1;
              state        <= state;
            end if;
          else
            state <= state;
          end if;
        when proc =>
          if(clk_rx = '1') then
            if(cnt_bitwidth = 15) then
              if(cnt_bitnum = 7) then
                cnt_bitnum <= 0;
                state      <= stopbit;
              else
                cnt_bitnum <= cnt_bitnum + 1;
                state      <= state;
              end if;
              dout_reg     <= rxd & dout_reg(7 downto 1);
              cnt_bitwidth <= 0;
            else
              cnt_bitwidth <= cnt_bitwidth + 1;
              state        <= state;
            end if;
          else
            state <= state;
          end if;
        when stopbit =>
          if(clk_rx = '1') then
            if(cnt_bitwidth = 15) then
              stby     <= '1';
              received <= '1';
              state    <= idle;
            else
              cnt_bitwidth <= cnt_bitwidth + 1;
              state        <= state;
            end if;
          else
            state <= state;
          end if;
        when others =>
          state <= idle;
      end case;
    end if;
  end process;
end rtl;
