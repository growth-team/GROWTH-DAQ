--iBus_Library.vhdl
--
--SpaceWire Board / User FPGA / Modularized Structure Template
--internal Bus (iBus) / Library file
--
--ver20071021 Takayuki Yuasa
--deleted e2i and i2e signals (not used in e2iConnector)
--
--ver20071203 Takayuki Yuasa
--TimeoutLimit wo tsuika. iBus no freeze wo kaiketsu
--ver0.0
--20061222 Takayuki Yuasa
--based on InternalBusSignalsPackage.vhdl ver0.2
--20061228 Takayuki Yuasa
--added TransferError
--20070109 Takayuki Yuasa
--added e2i and i2e signals

---------------------------------------------------
--Declarations of Libraries
---------------------------------------------------
library ieee,work;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;
use work.iBus_AddressMap.all;

---------------------------------------------------
--Package for iBus
---------------------------------------------------
package iBus_Library is
	
	---------------------------------------------------
	--Constant variables used in iBus connection
	---------------------------------------------------
	constant NumberOfNodes	:	Integer	:= 14; -- 9 for 1ch test; 14 for 8ch implementation
	
	constant REGISTER_ALL_ZERO	: std_logic_vector(15 downto 0) := (others=>'0');
	constant REGISTER_ALL_ONE	: std_logic_vector(15 downto 0) := (others=>'1');
	
	constant BurstLimit		:	Integer	:= 10;
	constant TimeoutLimit	:	Integer	:= 30;
	
	---------------------------------------------------
	--BusIF's Output Signal Connected to BusController
	---------------------------------------------------
	type iBus_Signals_BusIF2BusController is record
		--Address and Data signals
		Address		: std_logic_vector(15 downto 0);
		Data			: std_logic_vector(15 downto 0);
		
		--signal to iBus_Controller
		Request		: std_logic;
		Respond		: std_logic;
		TransferError	: std_logic;
		
		--command signals
		Send			: std_logic;
		Read			: std_logic;

		--debug
		Debug				:	std_logic_vector(7 downto 0);
	end record;
	
	---------------------------------------------------
	--BusIF's Input Signal Connected to BusController
	---------------------------------------------------
	type iBus_Signals_BusController2BusIF is record
		--iBus busy
		BusBusy	:	std_logic;
		
		--Address and Data
		Address	:	std_logic_vector(15 downto 0);
		Data		:	std_logic_vector(15 downto 0);
		
		--signal from iBus_Controller
		RequestGrant	: std_logic;
		
		--command from Master
		Send		:	std_logic;
		Read		:	std_logic;
		
		--error from BusController(when Target's ReceiveFIFO is full)
		TransferError	: std_logic;

		--timeout
		TimedOut        : std_logic;
		
		--debug
		Debug				:	std_logic_vector(7 downto 0);
	end record;

	---------------------------------------------------
	--vector types of BusIF-BusController connecting signals
	---------------------------------------------------
	type iBus_Signals_BusIF2BusController_Vector is array (INTEGER range <>) of iBus_Signals_BusIF2BusController;
	type iBus_Signals_BusController2BusIF_Vector is array (INTEGER range <>) of iBus_Signals_BusController2BusIF;


	---------------------------------------------------
	--BusIF's Output Signal Connected to UserModule
	---------------------------------------------------
	type iBus_Signals_BusIF2UserModule is record
		BusIFBusy			:	std_logic;
		ReceivedAddress	:	std_logic_vector(15 downto 0);
		ReceivedData		:	std_logic_vector(15 downto 0);
		
		SendBufferFull			:	std_logic;
		SendBufferEmpty		:	std_logic;
		ReceiveBufferFull		:	std_logic;
		ReceiveBufferEmpty	:	std_logic;
		
		ReadAddress	:	std_logic_vector(15 downto 0);
		ReadData		:	std_logic_vector(15 downto 0);
		ReadDone			:	std_logic;
		
		beRead			:	std_logic;
		beReadAddress	:	std_logic_vector(15 downto 0);

		--debug
		Debug				:	std_logic_vector(7 downto 0);
	end record;

	---------------------------------------------------
	--BusIF's Input Signal from UserModule
	---------------------------------------------------
	type iBus_Signals_UserModule2BusIF is record
		SendAddress	:	std_logic_vector(15 downto 0);
		SendData		:	std_logic_vector(15 downto 0);
		SendEnable	:	std_logic;

		ReceiveEnable	:	std_logic;

		ReadAddress		:	std_logic_vector(15 downto 0);
		ReadGo			:	std_logic;

		beReadDone	:	std_logic;
		beReadData	:	std_logic_vector(15 downto 0);

		--debug
		Debug				:	std_logic_vector(7 downto 0);
	end record;

end iBus_Library;
