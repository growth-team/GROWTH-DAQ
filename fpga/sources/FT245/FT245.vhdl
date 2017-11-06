library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

entity FT245AsynchronousFIFO is
  port(
    -- Signals connected to FT245 (FT2232H)
    DATA     : inout std_logic_vector(7 downto 0);
    nRXF     : in    std_logic;
    nTXE     : in    std_logic;
    nRD      : out   std_logic;
    nWR      : out   std_logic;
    -- FT245 => FPGA
    rx_data  : out   std_logic_vector(7 downto 0);
    rx_valid : out   std_logic;
    rx_full  : in    std_logic;
    -- FPGA => FT245
    tx_data  : in    std_logic_vector(7 downto 0);
    tx_enable: in    std_logic;
    tx_done  : out   std_logic;
    tx_ready : out   std_logic;
    -- Cloc and reset
    clock    : in    std_logic;
    reset    : in    std_logic
    );          
end FT245AsynchronousFIFO;

architecture rtl of FT245AsynchronousFIFO is

  type states is (
    INITIALIZE, IDLE, READ0, READ1, READ2, READ3, READ4, READ5,
    WRITE0, WRITE1, WRITE2, WRITE3, WRITE4, WRITE5
    );
  signal state            : states := INITIALIZE;
  signal state_after_wait : states := INITIALIZE;
  signal data_latched     : std_logic_vector(7 downto 0) := (others => '0');

  signal i_nRXF : std_logic := '0';
  signal i_nTXE : std_logic := '0';

  -- For ILA
  attribute mark_debug : string;
  attribute keep : string;
  attribute mark_debug of tx_data  : signal is "true";
  attribute mark_debug of nRD  : signal is "true";
  attribute mark_debug of nWR  : signal is "true";
  attribute mark_debug of nTXE  : signal is "true";
  attribute mark_debug of nRXF  : signal is "true";
 
begin

  tx_ready <= (not i_nTXE) when state = IDLE else '0';

  process(clock)
  begin
    if(rising_edge(clock))then
      if(reset = '1')then
        state <= INITIALIZE;
      else
        -- Latch external signals
        i_nRXF <= nRXF;
        i_nTXE <= nTXE;
      
        case state is
          when INITIALIZE =>
            DATA  <= (others => 'Z');
            nRD   <= '1';
            nWR   <= '1';
            tx_done <= '0';
            state <= IDLE;
          when IDLE =>
            if(i_nRXF = '0' and rx_full = '0')then
              DATA  <= (others => 'Z');
              nRD   <= '0';
              state <= READ0;
            elsif(i_nTXE = '0' and tx_enable = '1')then
              DATA  <= tx_data;
              nRD   <= '1'; -- Disable Read (to make data pins of FT2232 to be high impedance)
              state <= WRITE0;
            else
              nRD   <= '1';
              nWR   <= '1';
              tx_done <= '0';
              DATA <= (others => 'Z');
            end if;
          when READ0 =>
            state <= READ1;
          when READ1 =>
            state <= READ2;
          when READ2 =>
            data_latched <= DATA;
            state <= READ3;
          when READ3 =>
            data_latched <= DATA;
            state    <= READ4;
          when READ4 =>
            rx_data <= data_latched;
            rx_valid <= '1';
            nRD      <= '1';
            state    <= READ5;
          when READ5 =>
            rx_valid <= '0';
            if(i_nRXF = '1')then
              state <= IDLE;
            end if;
          when WRITE0 =>
            DATA  <= tx_data;
            state <= WRITE1;
          when WRITE1 =>
            DATA  <= tx_data;
            nWR   <= '0';
            state <= WRITE2;
          when WRITE2 =>
            state <= WRITE3;
          when WRITE3 =>
            tx_done <= '1';
            state <= WRITE4;
          when WRITE4 =>
            state <= WRITE5;
          when WRITE5 =>
            tx_done <= '0';
            nWR   <= '1';
            DATA  <= (others => 'Z');
            if(i_nTXE = '1' and tx_enable='0')then
              state <= IDLE;
            end if;
        end case;
      end if;
    end if;
  end process;

end rtl;
