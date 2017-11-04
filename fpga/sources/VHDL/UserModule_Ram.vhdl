--UserModule_Ram.vhdl
--
--SpaceWire Board / User FPGA / Modularized Structure Template
--UserModule / Ram
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
entity UserModule_Ram is
  port(
    Address     : in  std_logic_vector(9 downto 0);
    DataIn      : in  std_logic_vector(15 downto 0);
    DataOut     : out std_logic_vector(15 downto 0);
    WriteEnable : in  std_logic;
    Clock       : in  std_logic
    );
end UserModule_Ram;

---------------------------------------------------
--Behavioral description
---------------------------------------------------
architecture Behavioral of UserModule_Ram is

  ---------------------------------------------------
  --Declarations of Components
  ---------------------------------------------------
  component ram16x1024
    port (
      clka : in std_logic;
      wea : in std_logic_vector(0 downto 0);
      addra : in std_logic_vector(9 downto 0);
      dina : in std_logic_vector(15 downto 0);
      douta : out std_logic_vector(15 downto 0)
    );
  end component;
  
  ---------------------------------------------------
  --Declarations of Signals
  ---------------------------------------------------
  --Signals
  signal wea : std_logic_vector(0 downto 0);

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
  ram_core : ram16x1024
    port map(
      addra => Address,
      clka  => Clock,
      dina  => DataIn,
      douta => DataOut,
      wea   => wea
      );

  ---------------------------------------------------
  --Static relationships
  ---------------------------------------------------
  wea(0) <= WriteEnable;

  ---------------------------------------------------
  --Dynamic Processes with Sensitivity List
  ---------------------------------------------------
  
end Behavioral;
