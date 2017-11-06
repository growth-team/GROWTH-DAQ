--UserModule_Fifo.vhdl
--
--SpaceWire Board / User FPGA / Modularized Structure Template
--UserModule / FIFO
--
--ver20071023 Takayuki Yuasa
--file created
--based on UserModule_Template_with_BusIFLite.vhdl (ver20071021)

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
entity UserModule_Fifo is
  port(
    --data
    DataIn         : in  std_logic_vector(15 downto 0);
    DataOut        : out std_logic_vector(15 downto 0);
    --controll
    ReadEnable     : in  std_logic;
    WriteEnable    : in  std_logic;
    --status
    Empty          : out std_logic;
    Full           : out std_logic;
    ReadDataCount  : out std_logic_vector(9 downto 0);
    WriteDataCount : out std_logic_vector(9 downto 0);
    --clock and reset
    ReadClock      : in  std_logic;
    WriteClock     : in  std_logic;
    GlobalReset    : in  std_logic
    );
end UserModule_Fifo;

---------------------------------------------------
--Behavioral description
---------------------------------------------------
architecture Behavioral of UserModule_Fifo is

  ---------------------------------------------------
  --Declarations of Components
  ---------------------------------------------------
  component usermodule_fifo_core
    port (
      din           : in  std_logic_vector(15 downto 0);
      rd_clk        : in  std_logic;
      rd_en         : in  std_logic;
      rst           : in  std_logic;
      wr_clk        : in  std_logic;
      wr_en         : in  std_logic;
      dout          : out std_logic_vector(15 downto 0);
      empty         : out std_logic;
      full          : out std_logic;
      rd_data_count : out std_logic_vector(9 downto 0);
      wr_data_count : out std_logic_vector(9 downto 0)
      );
  end component;

  ---------------------------------------------------
  --Declarations of Signals
  ---------------------------------------------------
  --Signals
  signal reset : std_logic := '0';
  --Registers

  --Counters

  --State Machines' State-variables

  ---------------------------------------------------
  --Beginning of behavioral description
  ---------------------------------------------------
begin

  ---------------------------------------------------
  --Instantiations of Components
  ---------------------------------------------------
  inst_fifo : usermodule_fifo_core
    port map(
      din           => DataIn,
      rd_clk        => ReadClock,
      rd_en         => ReadEnable,
      rst           => reset,
      wr_clk        => WriteClock,
      wr_en         => WriteEnable,
      dout          => DataOut,
      empty         => Empty,
      full          => Full,
      rd_data_count => ReadDataCount,
      wr_data_count => WriteDataCount
      );

  ---------------------------------------------------
  --Static relationships
  ---------------------------------------------------
  reset <= not GlobalReset;

  ---------------------------------------------------
  --Dynamic Processes with Sensitivity List
  ---------------------------------------------------
  
end Behavioral;
