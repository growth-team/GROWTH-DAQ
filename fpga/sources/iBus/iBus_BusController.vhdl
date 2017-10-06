--iBus_BusController.vhdl
--
--User FPGA iBus BusController Module
--
--ver20071102 Takayuki Yuasa
--remove "for" loop
--
--ver0.0
--implementation of Send process
--20061228 Takayuki Yuasa
--implementation of Read/beRead process
--20061229 Takayuki Yuasa
--simulation succeeded

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
entity iBus_BusController is
	generic(
		TimeoutConstant : integer := 65535; -- in units of clock cycle
		NumberOfNodes	:	Integer range 0 to 128
	);
	port(
		--connected to BusIFs
		BusIF2BusController		:	in		iBus_Signals_BusIF2BusController_Vector(NumberOfNodes-1 downto 0);
		BusController2BusIF		:	out	iBus_Signals_BusController2BusIF_Vector(NumberOfNodes-1 downto 0);
		Clock					:	in		std_logic;
		GlobalReset			:	in		std_logic
	);
end iBus_BusController;

---------------------------------------------------
--Architecture Declaration
---------------------------------------------------
architecture Behavioral of iBus_BusController is

	---------------------------------------------------
	--component declaration
	---------------------------------------------------

	---------------------------------------------------
	--Declaration of Signals
	---------------------------------------------------
	type BusController_Status is (
		Initialize,
		Idle,
		WaitCommandThenTellCommand,
		WaitRespondFromTargetBusIF_Send,
		WaitRespondFromTargetBusIF_Read,
		ReturnsReadData,
		WaitRequestSetOff,
		RequestTimeout
	);
	signal BusController_State	:	BusController_Status :=Initialize;
	
	signal BusBusy			:	std_logic :='0';
	
	signal SendAddress	:	std_logic_vector(15 downto 0)	:= x"0000";
	signal SendData		:	std_logic_vector(15 downto 0)	:= x"0000";
	
	signal ReadAddress	:	std_logic_vector(15 downto 0)	:= x"0000";
	signal ReadData		:	std_logic_vector(15 downto 0)	:= x"0000";
	
	signal BurstCount		: integer range 0 to BurstLimit		:= 0;
	signal TimeoutCount	: integer range 0 to TimeoutLimit	:= 0;
	signal LoopI			: integer range 0 to NumberOfNodes	:= 0;
	signal LoopO			: integer range 0 to NumberOfNodes	:= 0;
	
	--signals connected to Master/Target
	signal BusController2MasterBusIF	:	iBus_Signals_BusController2BusIF;
	signal MasterBusIF2BusController	:	iBus_Signals_BusIF2BusController;
	signal TargetBusIF2BusController	:	iBus_Signals_BusIF2BusController;
	signal BusController2AllBusIFs	:	iBus_Signals_BusController2BusIF;
	signal DummySignalOfBusController2BusIF	:	iBus_Signals_BusController2BusIF;
	signal DummySignalOfBusIF2BusController	:	iBus_Signals_BusIF2BusController;
	
	--variables for Master/Target selection
	signal MasterIndex	:	Integer range -1 to NumberOfNodes-1:=-1;
	signal TargetIndex	:	Integer range -1 to NumberOfNodes-1:=-1;
	constant MaximumOfMasterIndex	:	Integer := NumberOfNodes-1;
	constant MaximumOfTargetIndex	:	Integer := NumberOfNodes-1;
	signal RequestFlagArray	:	std_logic_vector(NumberOfNodes-1 downto 0)	:=	(others => '0');
	signal RespondFlagArray	:	std_logic_vector(NumberOfNodes-1 downto 0)	:=	(others => '0');
	signal TransferErrorFlagArray	:	std_logic_vector(NumberOfNodes-1 downto 0)	:=	(others => '0');
	
	signal timeoutCounter : integer range 0 to TimeoutConstant := TimeoutConstant;
	signal timeout : std_logic := '0';
	
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
	--make connection from BusController to Master BusIF
	MasterSelector:
	for I in 0 to (NumberOfNodes-1) generate
--		BusController2BusIF(I).BusBusy	<= BusController2MasterBusIF.BusBusy when MasterIndex=I else BusController2AllBusIFs.BusBusy;
--		BusController2BusIF(I).Address	<= BusController2MasterBusIF.Address when MasterIndex=I else BusController2AllBusIFs.Address;
--		BusController2BusIF(I).Data		<= BusController2MasterBusIF.Data when MasterIndex=I else BusController2AllBusIFs.Data;
--		BusController2BusIF(I).RequestGrant 	<= BusController2MasterBusIF.RequestGrant when MasterIndex=I else BusController2AllBusIFs.RequestGrant;
--		BusController2BusIF(I).Send
--			<= BusController2MasterBusIF.Send when MasterIndex=I
--				else BusController2AllBusIFs.Send;
--		BusController2BusIF(I).Read
--			<= BusController2MasterBusIF.Read when MasterIndex=I
--				else BusController2AllBusIFs.Read;
--		BusController2BusIF(I).TransferError
--			<= BusController2MasterBusIF.TransferError when MasterIndex=I
--				else BusController2AllBusIFs.TransferError;
		BusController2BusIF(I)
			<= BusController2MasterBusIF when MasterIndex=I
				else BusController2AllBusIFs;
	end generate MasterSelector;
	
	MasterBusIF2BusController
		<= DummySignalOfBusIF2BusController when MasterIndex=-1
			else BusIF2BusController(MasterIndex);

	TargetBusIF2BusController
		<= DummySignalOfBusIF2BusController when TargetIndex=-1
			else BusIF2BusController(TargetIndex);
	
	BusController2AllBusIFs.BusBusy <= BusBusy;
	BusController2MasterBusIF.BusBusy <= BusBusy;
	
	--set RequestFlagArray/RespondFlagArray to decide Master/Target BusIFModule
	RequestFlagAndRespondFlagArraySet :
	for I in (NumberOfNodes-1) downto 0 generate
		RequestFlagArray(I)
			<=	'1' when BusIF2BusController(I).Request='1'
				else '0';
		
		RespondFlagArray(I)
			<=	'1' when BusIF2BusController(I).Respond='1'
				else '0';
		
		TransferErrorFlagArray(I)
			<=	'1' when BusIF2BusController(I).TransferError='1'
				else '0';
	end generate RequestFlagAndRespondFlagArraySet;
	
	---------------------------------------------------
	--Dynamic Processes with Sensitivity List
	----------- ----------------------------------------
	process (Clock,GlobalReset)
	begin
		--is this process invoked with GlobalReset?
		if (GlobalReset='0') then
			timeout <= '0';
			timeoutCounter <= TimeoutConstant;
			--move to next state
			BusController_State <= Initialize;
		--is this process invoked with Clock Event?
		elsif (Clock'Event and Clock='1' and GlobalReset='1') then			

		     --timeout controller
	        if(BusController_State=Initialize or BusController_State=Idle)then
	          timeout <= '0';
	          timeoutCounter <= TimeoutConstant;
	        else
	          if(timeoutCounter=0)then
	            --timeout
	            timeout <= '1';
	            timeoutCounter <= TimeoutConstant;
	          else
	            timeout <= '0';
	            timeoutCounter <= timeoutCounter -1;
	          end if;
	        end if;

	        --state machine
	        if(timeout='1')then
	        	BusController2MasterBusIF.TimedOut <= '1';
	        	BusController2AllBusIFs.TimedOut <= '1';
	        	BusController_State <= Initialize;
	        else
	        	BusController2MasterBusIF.TimedOut <= '0';
	        	BusController2AllBusIFs.TimedOut <= '0';
	        	--if not timed out
				case BusController_State is
					when Initialize =>
						--initialize outputs to bus
						BusController2AllBusIFs.Send <= '0';
						BusController2AllBusIFs.Read <= '0';
						BusController2AllBusIFs.RequestGrant <= '0';
						BusController2MasterBusIF.RequestGrant <= '0';
						--unset BusBusy
						BusBusy <= '0';
						--Loop variables
						LoopI <= 0;
						LoopO <= 0;
						--BurstCount and TiemoutCounter
						BurstCount <= 0;
						TimeoutCount <= 0;
						--move to next state
						BusController_State <= Idle;
						
					when Idle =>
						--is there any access from BusIF
						TimeoutCount <= 0;
						if (LoopI=NumberOfNodes) then
							LoopI <= 0;
						else
							if (RequestFlagArray(LoopI)='1' and BurstCount<BurstLimit) then
								BusBusy <= '1';
								BurstCount <= BurstCount + 1;
								--set MasterIndex to select Master
								MasterIndex <= LoopI;
								BusBusy <= '1';
								BusController2MasterBusIF.RequestGrant <= '1';
								--Reset LoopI
								LoopO <= 0;
								--move to next state
								BusController_State <= WaitCommandThenTellCommand;
							else
								BurstCount <= 0;
								LoopI <= LoopI + 1;
							end if;
						end if;
						
					when WaitCommandThenTellCommand =>
						--wait command from Master BusIF
						if (MasterBusIF2BusController.Send='1') then
							--got Send command, then tell Send command to all UMs
							BusController2AllBusIFs.Send <= '1';
							--broadcast Address/Data to all BusIF
							BusController2AllBusIFs.Address <= MasterBusIF2BusController.Address;
							BusController2AllBusIFs.Data <= MasterBusIF2BusController.Data;
							--move to next state
							BusController_State <= WaitRespondFromTargetBusIF_Send;
						elsif (MasterBusIF2BusController.Read='1') then
							--got Read command, then tell Read command to all UMs
							BusController2AllBusIFs.Read <= '1';
							--broadcast Address to all BusIF
							BusController2AllBusIFs.Address <= MasterBusIF2BusController.Address;
							--move to next state
							BusController_State <= WaitRespondFromTargetBusIF_Read;
						end if;
						
					-----------------------------------------------------
					--Send/Receive process
					when WaitRespondFromTargetBusIF_Send =>
						--is there any Respond signal from BusIF?
						--or timeout?
						if (conv_integer(RespondFlagArray)/=0) then
							--there is Respond from Target BusIF,
							--then tell "Command Done" to Master BusIF
							BusController2MasterBusIF.RequestGrant <= '0';
							BusController2MasterBusIF.TransferError <= '0';
							BusController2AllBusIFs.Send <= '0';
							--move to next state
							BusController_State <= WaitRequestSetOff;
						elsif (conv_integer(TransferErrorFlagArray)/=0) then
							--there is TransferError signal from Target BusIF,
							--then tell "Command Done(with error)" to Master BusIF
							BusController2MasterBusIF.RequestGrant <= '0';
							BusController2MasterBusIF.TransferError <= '1';
							BusController2AllBusIFs.Send <= '0';
							--move to next state
							BusController_State <= WaitRequestSetOff;
						else
							if (TimeoutCount=TimeoutLimit) then
								TimeoutCount <= 0;
								BusController_State <= RequestTimeout;
							else
								TimeoutCount <= TimeoutCount + 1;
							end if;
						end if;
						
					-----------------------------------------------------
					--beRead process
					when WaitRespondFromTargetBusIF_Read =>
						--is there any Respond from Target BusIF candidates
						for I in MaximumOfTargetIndex downto 0 loop
							if (RespondFlagArray(I)='1') then
								--there is Respond from Target BusIF,
								TargetIndex <= I;
								--move to next state
								BusController_State <= ReturnsReadData;
							end if;
						end loop;
					
					when ReturnsReadData =>
						--return read out Data
						BusController2MasterBusIF.Data <= TargetBusIF2BusController.Data;
						--tell "Command Done" to Master BusIF
						BusController2MasterBusIF.RequestGrant <= '0';
						--move to next state
						BusController_State <= WaitRequestSetOff;

					-----------------------------------------------------
					--Finilization of BusProcess
					when WaitRequestSetOff =>
						--wait for setting off of Request signal
						if (MasterBusIF2BusController.Request='0') then
							--clear BusBusy
							BusBusy <= '0';
							--deselect Master and Target
							MasterIndex <= -1;
							TargetIndex <= -1;
							--clear TransferError if set
							BusController2MasterBusIF.TransferError <= '0';
							--initialize outputs to bus
							BusController2AllBusIFs.Send <= '0';
							BusController2AllBusIFs.Read <= '0';
							BusController2AllBusIFs.RequestGrant <= '0';
							BusController2MasterBusIF.RequestGrant <= '0';
							--move to next state
							BusController_State <= Idle;
						end if;
					when RequestTimeout =>
						--finish burst
						BurstCount <= BurstLimit;
						--clear BusBusy
						BusBusy <= '0';
						--deselect Master and Target
						MasterIndex <= -1;
						TargetIndex <= -1;
						--clear TransferError if set
						BusController2MasterBusIF.TransferError <= '0';
						--initialize outputs to bus
						BusController2AllBusIFs.Send <= '0';
						BusController2AllBusIFs.Read <= '0';
						BusController2AllBusIFs.RequestGrant <= '0';
						BusController2MasterBusIF.RequestGrant <= '0';
						--move to next state
						BusController_State <= Idle;

					when others =>
						BusController_State <= Initialize;

				end case;
			end if; --timeout check
		end if;
	end process;

end Behavioral;