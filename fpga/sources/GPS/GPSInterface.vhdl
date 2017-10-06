library IEEE;
use IEEE.STD_LOGIC_1164.all;
use ieee.std_logic_unsigned.all;

library UNISIM;
use UNISIM.VComponents.all;

entity GPSInterface is
  port(
    --clock and reset
    clock                        : in  std_logic;
    reset                        : in  std_logic;
    --signals from GPS
    gpsData                      : in  std_logic_vector(7 downto 0);  -- ASCII data from GPS
    gpsDataEnable                : in  std_logic;
    gps1PPS                      : in  std_logic;  -- 1PPS
    --output signals
    gpsDDMMYY                    : out std_logic_vector(47 downto 0);  -- date
    gpsHHMMSS_SSS                : out std_logic_vector(71 downto 0);  -- time
    gpsDateTimeUpdatedSingleShot : out std_logic;  -- single-shot pulse that indicates update of date/time
    gps1PPSSingleShot            : out std_logic  -- single-shot 1PPS pulse synchronized to the module's clock 
    );
end GPSInterface;

architecture Behavioral of GPSInterface is

  signal gpsDataEnable_previous   : std_logic             := '0';
  signal gpsDataEnable_singleshot : std_logic             := '0';
  signal gps1PPS_previous         : std_logic             := '0';
  signal gps1PPS_latched          : std_logic             := '0';
  signal gprmcByteCount           : integer range 0 to 47 := 0;
  signal gprmcByteCount_previous  : integer range 0 to 47 := 0;
  signal gprmcByteCountMax        : integer               := 47;

  signal receivedData_previous : std_logic_vector(39 downto 0);  -- used to detect GPRMC
  signal receivedData          : std_logic_vector(39 downto 0);  -- used to detect GPRMC

  -- single-shot pulse to start interpretation of GPRMC
  signal gprmcReceived : std_logic := '0';

  signal gpsHH     : std_logic_vector(15 downto 0) := (others => '0');
  signal gpsMM     : std_logic_vector(15 downto 0) := (others => '0');
  signal gpsSS_SSS : std_logic_vector(39 downto 0) := (others => '0');
  signal gpsDay    : std_logic_vector(15 downto 0) := (others => '0');
  signal gpsMonth  : std_logic_vector(15 downto 0) := (others => '0');
  signal gpsYear   : std_logic_vector(15 downto 0) := (others => '0');

begin

  ---------------------------------------------
  -- Process
  ---------------------------------------------
  process(Clock, reset)
  begin
    if(reset = '1')then
      gprmcByteCount   <= 0;
      gps1PPS_previous <= '0';
    elsif(Clock = '1' and Clock'event)then
      -- single-shot 1PPS generation
      gps1PPS_latched  <= gps1PPS;
      gps1PPS_previous <= gps1PPS_latched;
      if(gps1PPS_previous = '0' and gps1PPS_latched = '1')then
        gps1PPSSingleShot <= '1';
      else
        gps1PPSSingleShot <= '0';
      end if;

      -- store Rx data
      gpsDataEnable_previous <= gpsDataEnable;
      if(gpsDataEnable = '1' and gpsDataEnable_previous = '0')then
        receivedData(39 downto 32) <= receivedData(31 downto 24);
        receivedData(31 downto 24) <= receivedData(23 downto 16);
        receivedData(23 downto 16) <= receivedData(15 downto 8);
        receivedData(15 downto 8)  <= receivedData(7 downto 0);
        receivedData(7 downto 0)   <= gpsData;
        gpsDataEnable_singleshot   <= '1';
      else
        gpsDataEnable_singleshot <= '0';
      end if;

      -- wait for GPRMC line
      receivedData_previous <= receivedData;
      if(
        receivedData_previous /= x"4750524D43"  -- 'GPRMC'
        and receivedData = x"4750524D43"        -- 'GPRMC'
        and gprmcReceived = '0'
        )then
        gprmcReceived <= '1';
      else
        gprmcReceived <= '0';
      end if;

      -- update date/time
      if(gprmcByteCount = 0)then
        if(gprmcReceived = '1')then
          gprmcByteCount <= 5;
        end if;
      elsif(gpsDataEnable_singleshot = '1')then
        if(gprmcReceived = '1')then
          gprmcByteCount <= 5;          --reset bute count
        else
          if(gprmcByteCount < gprmcByteCountMax)then
            gprmcByteCount <= gprmcByteCount + 1;
          else
            gprmcByteCount <= 0;
          end if;
        end if;

        case gprmcByteCount is
          -- Time
          when 6 =>
            gpsHH(15 downto 8) <= gpsData;
          when 7 =>
            gpsHH(7 downto 0) <= gpsData;
          when 8 =>
            gpsMM(15 downto 8) <= gpsData;
          when 9 =>
            gpsMM(7 downto 0) <= gpsData;
          when 10 =>
            gpsSS_SSS(39 downto 32) <= gpsData;
          when 11 =>
            gpsSS_SSS(31 downto 24) <= gpsData;
          when 13 =>
            gpsSS_SSS(23 downto 16) <= gpsData;
          when 14 =>
            gpsSS_SSS(15 downto 8) <= gpsData;
          when 15 =>
            gpsSS_SSS(7 downto 0) <= gpsData;
            -- Date
          when 33 =>
            gpsDay(15 downto 8) <= gpsData;
          when 34 =>
            gpsDay(7 downto 0) <= gpsData;
          when 35 =>
            gpsMonth(15 downto 8) <= gpsData;
          when 36 =>
            gpsMonth(7 downto 0) <= gpsData;
          when 37 =>
            gpsYear(15 downto 8) <= gpsData;
          when 38 =>
            gpsYear(7 downto 0) <= gpsData;
          when others =>
            --do nothing
        end case;
      end if;

      --generate gpsDateTimeUpdatedSingleShot
      gprmcByteCount_previous <= gprmcByteCount;
      if(gprmcByteCount_previous = 38 and gprmcByteCount = 39)then
        gpsDateTimeUpdatedSingleShot <= '1';
      else
        gpsDateTimeUpdatedSingleShot <= '0';
      end if;
    end if;
  end process;

  gpsDDMMYY     <= gpsDay & gpsMonth & gpsYear;
  gpsHHMMSS_SSS <= gpsHH & gpsMM & gpsSS_SSS;

-- $GPRMC,124846.086,V,,,,,0.00,0.00,020615,,,N*4E
--  01234567890123456789012345678901234567890
--  0         1         2         3         4
-- HHMMSS.SSS: digits 6-15
-- DDMMYY    : digits 33-38

  
end Behavioral;

--============================================
library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

entity GPSUARTInterface is
    generic(
      InputClockPeriodInNanoSec : integer := 20;     -- ns
      BaudRate                  : integer := 115200  -- bps
      );
  port(
    --clock and reset
    clock                        : in  std_logic;
    reset                        : in  std_logic;
    --signals from GPS
    gpsUARTIn                      : in  std_logic;  -- ASCII data from GPS
    gps1PPS                      : in  std_logic;  -- 1PPS
    --output signals
    gpsReceivedByte              : out std_logic_vector(7 downto 0); -- received byte
    gpsReceivedByteEnable        : out std_logic; -- byte enable
    gpsDDMMYY                    : out std_logic_vector(47 downto 0);  -- date
    gpsHHMMSS_SSS                : out std_logic_vector(71 downto 0);  -- time
    gpsDateTimeUpdatedSingleShot : out std_logic;  -- single-shot pulse that indicates update of date/time
    gps1PPSSingleShot            : out std_logic  -- single-shot 1PPS pulse synchronized to the module's clock 
    );
end GPSUARTInterface;

architecture Behavioral of GPSUARTInterface is
  component UARTInterface is
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
  end component;

signal TxSerial : std_logic;          
signal RxSerial :  std_logic;          
signal txDataIn :  std_logic_vector(7 downto 0);  
signal rxDataOut : std_logic_vector(7 downto 0);  
signal txEnable :  std_logic;          
signal received : std_logic;          
signal txReady : std_logic           ;

begin

instanceOfUARTInterface : UARTInterface
generic map(
  InputClockPeriodInNanoSec => InputClockPeriodInNanoSec,
  BaudRate => BaudRate
  )
port map(
 Clock => Clock,
 Reset => Reset,
 TxSerial => txSerial,
 RxSerial => gpsUARTIn,
 txDataIn => txDataIn,
 rxDataOut => rxDataOut,
 txEnable => txEnable,
 received => received,
 txReady => txReady 
);

instanceOfGPSInterface : entity work.GPSInterface
port map(
 clock => clock,
 reset => reset,
 gpsData => rxDataOut,
 gpsDataEnable => received,
 gps1PPS => gps1PPS,
 gpsDDMMYY => gpsDDMMYY,
 gpsHHMMSS_SSS => gpsHHMMSS_SSS,
 gpsDateTimeUpdatedSingleShot => gpsDateTimeUpdatedSingleShot,
 gps1PPSSingleShot => gps1PPSSingleShot 
);

end architecture;

