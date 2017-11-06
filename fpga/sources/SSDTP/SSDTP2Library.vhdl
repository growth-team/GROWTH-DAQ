----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    05:29:37 04/16/2010 
-- Design Name: 
-- Module Name:    library - Behavioral 
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

-- Takayuki Yuasa 20140703
-- - Renamed from spw2tcp_library.vhdl in SpaceWire-to-GigabitEther.

---------------------------------------------------
--Declarations of Libraries
---------------------------------------------------
library ieee, work;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

library UNISIM;
use UNISIM.VComponents.all;

package SSDTP2Library is

  constant PACKET_DESCRIPTOR_SIZE           : integer                      := 15;
  constant LogPACKET_DESCRIPTOR_SIZE        : integer                      := 6;
  constant PACKET_DESCRIPTOR_MAX_ID         : std_logic_vector(5 downto 0) := "001111";
  constant MaxPacketSize                    : integer                      := 1024*1024;
  constant LogMaxPacketSize                 : integer                      := 20;
  constant NDIGIT_MAX_PACKET_SIZE           : integer                      := LogMaxPacketSize;
  constant NDIGITS_SDRAM_ADDRESS            : integer                      := 24;
  constant PACKET_DESCRIPTOR_ID_FIFO_EMPTY  : std_logic_vector(6 downto 0) := "0000000";
  --SSDTP
  constant SSDTP_FLAG_DATA_COMPLETE_EOP     : std_logic_vector(7 downto 0) := x"00";
  constant SSDTP_FLAG_DATA_COMPLETE_EEP     : std_logic_vector(7 downto 0) := x"01";
  constant SSDTP_FLAG_DATA_FLAGMENTED       : std_logic_vector(7 downto 0) := x"02";
  constant SSDTP_FLAG_CONTROL_SENDTIMECODE  : std_logic_vector(7 downto 0) := x"30";
  constant SSDTP_FLAG_CONTROL_GOTTIMECODE   : std_logic_vector(7 downto 0) := x"31";
  constant SSDTP_FLAG_CONTROL_CHANGETXSPEED : std_logic_vector(7 downto 0) := x"38";
  constant SSDTP_FLAG_CONTROL_READCOMMAND   : std_logic_vector(7 downto 0) := x"40";
  constant SSDTP_FLAG_CONTROL_READREPLY     : std_logic_vector(7 downto 0) := x"41";
  constant SSDTP_FLAG_CONTROL_WRITECOMMAND  : std_logic_vector(7 downto 0) := x"50";
  constant SSDTP_FLAG_CONTROL_WRITEREPLY    : std_logic_vector(7 downto 0) := x"51";

  constant SSDTP_NDIGITS_SIZE_PART   : integer := 10;
  constant SSDTP_NDIGITS_HEADER_PART : integer := 2+SSDTP_NDIGITS_SIZE_PART;

  constant Simulation1OrImplementation0 : std_logic := '0';

  type linkStates is (
    LS_ERRRESET,
    LS_ERRWAIT,
    LS_READY,
    LS_STARTED,
    LS_CONNECTING,
    LS_RUN
    );

  ---------------------------------------------------
  -- Packet descriptors
  ---------------------------------------------------
  type PACKET_DESCRIPTROR is record
    sdram_address : std_logic_vector (NDIGITS_SDRAM_ADDRESS-1 downto 0);
    size          : std_logic_vector (LogMaxPacketSize downto 0);
    eoptype       : std_logic;
    channelid     : std_logic_vector(1 downto 0);
  end record;
  type PACKET_DESCRIPTROR_VECTOR is array (integer range <>) of PACKET_DESCRIPTROR;
  
end SSDTP2Library;
