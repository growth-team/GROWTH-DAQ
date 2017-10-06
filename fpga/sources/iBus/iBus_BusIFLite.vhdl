--iBus_BusIFLite.vhdl
--
--SpaceWire Board / User FPGA / Modularized Structure Template
--internal Bus (iBus) / Bus IF / Lite version
--
--ver20071021 Takayuki Yuasa
--file created

---------------------------------------------------
--Declarations of Libraries
---------------------------------------------------
library ieee,work;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;
use work.iBus_Library.all;
use work.iBus_AddressMap.all;

---------------------------------------------------
--Entity Declaration
---------------------------------------------------
entity iBus_BusIFLite is
	generic(
		InitialAddress	:	std_logic_vector(15 downto 0);
		FinalAddress	:	std_logic_vector(15 downto 0)
	);
	port(
		--connected to BusController
		BusIF2BusController	:	out	iBus_Signals_BusIF2BusController;
		BusController2BusIF	:	in		iBus_Signals_BusController2BusIF;
		
		--Connected to UserModule
		--UserModule master signal
		SendAddress			:	in 	std_logic_vector(15 downto 0);
		SendData				:	in 	std_logic_vector(15 downto 0);
		SendGo				:	in		std_logic;
		SendDone				:	out	std_logic;
		
		ReadAddress			:	in		std_logic_vector(15 downto 0);
		ReadData				:	out	std_logic_vector(15 downto 0);
		ReadGo				:	in		std_logic;
		ReadDone				:	out	std_logic;
		
		--UserModule target signal
		beReadAddress		:	out 	std_logic_vector(15 downto 0);
		beRead				:	out	std_logic;
		beReadData			:	in		std_logic_vector(15 downto 0);
		beReadDone			:	in		std_logic;
		
		beWrittenAddress		:	out 	std_logic_vector(15 downto 0);
		beWritten				:	out	std_logic;
		beWrittenData			:	out	std_logic_vector(15 downto 0);
		beWrittenDone			:	in		std_logic;

		Clock					:	in		std_logic;
		GlobalReset			:	in		std_logic
	);
end iBus_BusIFLite;

---------------------------------------------------
--Architecture Declaration
---------------------------------------------------
architecture Behavioral of iBus_BusIFLite is

	---------------------------------------------------
	--component declaration
	---------------------------------------------------

	---------------------------------------------------
	--Declaration of Signals
	---------------------------------------------------
	signal BusBusy	:	std_logic	:=	'0';
	
	type BusIF_Master_Status is (
		Initialize,
		Idle,
		WaitSendRequestGrant,
		WaitReadRequestGrant,
		WaitSendRequestDone,
		WaitReadRequestDone,
		WaitRequestCleared
	);
	type BusIF_Target_Status is (
		Initialize,
		Idle,
		AccessFromBusController,
		WaitbeReadDoneFromUserModule,
		WaitbeWrittenDoneFromUserModule,
		WaitAccessFromBusControllerDone
	);
	signal BusIF_Master_State	:	BusIF_Master_Status :=Initialize;
	signal BusIF_Target_State	:	BusIF_Target_Status :=Initialize;
	
	signal BusIF2BusController_Data_Send	: std_logic_vector(15 downto 0) := (others=>'0');
	signal BusIF2BusController_Data_beRead	: std_logic_vector(15 downto 0) := (others=>'0');
	
	--for simulation
	signal addressin,address1,address2 : std_logic_vector(15 downto 0);
	signal address3,address4,addressinteger : integer ;
	signal temp : std_logic_vector(15 downto 0) := X"0000";

	---------------------------------------------------
	--Beginning of behavioral description
	---------------------------------------------------
	begin

	---------------------------------------------------
	--Instanciation of components
	---------------------------------------------------

	---------------------------------------------------
	--Static relationships
	---------------------------------------------------
	BusBusy <= BusController2BusIF.BusBusy;
	beWrittenAddress <= BusController2BusIF.Address;
	beReadAddress <= BusController2BusIF.Address;
	beWrittenData <= BusController2BusIF.Data;
	BusIF2BusController.Data <= BusIF2BusController_Data_beRead when BusIF_Target_State/=Idle else BusIF2BusController_Data_Send;
	--internal signals
	
	--for simulation
	
	---------------------------------------------------
	--Dynamic Processes with Sensitivity List
	----------- ----------------------------------------
	MasterProcess : process (Clock,GlobalReset)
		variable a,b,c,d:Integer;
	begin
		--is this process invoked with GlobalReset?
		if (GlobalReset='0') then
			BusIF_Master_State <= Initialize;
		--is this process invoked with Clock Event?
		elsif (Clock'Event and Clock='1') then
			if(BusController2BusIF.TimedOut='1')then
				BusIF_Master_State <= Initialize;
			else
				case BusIF_Master_State is
					when Initialize =>
						--initialize outputs to bus
						BusIF2BusController.Address <= x"0000";
						BusIF2BusController_Data_Send <= x"0000";
						BusIF2BusController.Request <= '0';
						BusIF2BusController.Send <= '0';
						BusIF2BusController.Read <= '0';
						--reset data and address signals
						ReadData <= x"0000";
						--reset done signals
						SendDone <= '0';
						ReadDone <= '0';
						--move to next state
						BusIF_Master_State <= Idle;
					when Idle =>
						--is Send set?
						if (SendGo='1' and BusBusy='0') then
							--set ON('1') the Request signal
							BusIF2BusController.Request <= '1';
							--set Send signal ON
							BusIF2BusController.Send <= '1';
							--tell Address/Data of Send command to BusController
							BusIF2BusController.Address <= SendAddress;
							BusIF2BusController_Data_Send <= SendData;
							--move to next state
							BusIF_Master_State <= WaitSendRequestGrant;
						--is there any request to Read data from another UserModule
						elsif (ReadGo='1' and BusBusy='0') then
							--tell Read request to BusController
							BusIF2BusController.Request <= '1';
							BusIF2BusController.Read <= '1';
							BusIF2BusController.Address <= ReadAddress;
							--move to next state(bRead process)
							BusIF_Master_State <= WaitReadRequestGrant;
						end if;
					when WaitSendRequestGrant =>
						--wait until write request is granted
						if (BusController2BusIF.RequestGrant='1') then
							--move to next state
							BusIF_Master_State <= WaitSendRequestDone;
						end if;
					when WaitReadRequestGrant =>
						--wait until read request is granted
						if (BusController2BusIF.RequestGrant='1') then
							--move to next state
							BusIF_Master_State <= WaitReadRequestDone;
						end if;
					when WaitSendRequestDone =>
						--is send request completed?
						if (BusController2BusIF.RequestGrant='0') then
							--tell request completion to UserModule
							SendDone <= '1';
							--move to next state
							BusIF_Master_State <= WaitRequestCleared;
						end if;
					when WaitReadRequestDone =>
						--is read request completed?
						if (BusController2BusIF.RequestGrant='0') then
							--latch the Read data from BusController
							ReadData <= BusController2BusIF.Data;
							--tell request completion to UserModule
							ReadDone <= '1';
							--move to next state
							BusIF_Master_State <= WaitRequestCleared;
						end if;
					when WaitRequestCleared =>
						--are SendGo or ReadGo cleared?
						if (SendGo='0' and ReadGo='0') then
							--move to next state
							BusIF_Master_State <= Initialize;
						end if;
					when others =>
						BusIF_Master_State <= Initialize;
				end case;
			end if;--timeout check
		end if;
	end process;

	TargetProcess : process (Clock,GlobalReset)
		variable a,b,c,d:Integer;
	begin
		--is this process invoked with GlobalReset?
		if (GlobalReset='0') then
			BusIF_Target_State <= Initialize;
		--is this process invoked with Clock Event?
		elsif (Clock'Event and Clock='1') then
			if(BusController2BusIF.TimedOut='1')then
				BusIF_Target_State <= Initialize;
			else
				case BusIF_Target_State is
					when Initialize =>
						--reset beWritten and beRead signals
						beWritten <= '0';
						beRead <= '0';
						--clear signal connected to Bus Controller
						BusIF2BusController.Respond <= '0';
						--move to next state
						BusIF_Target_State <= Idle;
					when Idle =>
						--is there any request from bus controller
						if ((BusController2BusIF.Send='1' or BusController2BusIF.Read='1')
								and BusBusy='1') then
							--move to next state
							BusIF_Target_State <= AccessFromBusController;
						end if;
					when AccessFromBusController =>
						a:=conv_integer(unsigned(BusController2BusIF.Address));
						address1 <= InitialAddress;
						address2 <= FinalAddress;
						temp <= BusController2BusIF.Address;
						if (conv_integer(InitialAddress)<=a and
								a<=conv_integer(FinalAddress)) then
							--if Send(Receive) command
							if (BusController2BusIF.Send='1') then
								--tell UserModule that there is write request from BusController
								beWritten <= '1';
								--move to next state
								BusIF_Target_State <= WaitbeWrittenDoneFromUserModule;
							--if Read command
							elsif (BusController2BusIF.Read='1') then
								--tell UserModule that there is read request from BusController
								beRead <= '1';
								--move to next state
								BusIF_Target_State <= WaitbeReadDoneFromUserModule;
							else
								BusIF_Target_State <= Initialize;
							end if;
						else
							--the request from Bus Controller is not for me
							--nothing to do
							--move to next state
							BusIF_Target_State <= WaitAccessFromBusControllerDone;
						end if;
					when WaitbeWrittenDoneFromUserModule =>
						if (beWrittenDone='1') then
							--turn off beRead
							beWritten <= '0';
							--tell BusController the end of beWritten(send) process
							BusIF2BusController.Respond <= '1';
							--move to next state
							BusIF_Target_State <= WaitAccessFromBusControllerDone;						
						end if;
					when WaitbeReadDoneFromUserModule =>
						if (beReadDone='1') then
							--turn off beRead
							beRead <= '0';
							--tell BusController the end of Read process
							BusIF2BusController.Respond <= '1';
							--return the read out data
							BusIF2BusController_Data_beRead <= beReadData;
							--move to next state
							BusIF_Target_State <= WaitAccessFromBusControllerDone;						
						end if;
					when WaitAccessFromBusControllerDone =>
						if (BusController2BusIF.Send='0' and BusController2BusIF.Read='0') then
							--set Respond OFF
							BusIF2BusController.Respond <= '0';
							--move to next state
							BusIF_Target_State <= Idle;
						end if;				
					when others =>
						BusIF_Target_State <= Initialize;
				end case;
			end if;--tiemout check
		end if;
	end process;

end Behavioral;