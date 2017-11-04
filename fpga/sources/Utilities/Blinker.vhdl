library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity Blinker is
	generic (
		-- LED blink duration in units of clock period
		LedBlinkDuration : integer := 3125000
	);
	port(
		clock : in std_logic;
		reset : in std_logic;
		triggerIn : in std_logic;
		blinkOut : out std_logic
	);
end Blinker;

architecture Behavioral of Blinker is

signal Counter : integer range 0 to LedBlinkDuration := 0;
signal BlinkerState : integer range 0 to 3 := 0;

begin
	process (clock, reset) begin
		if (reset = '1') then
			BlinkerState <= 0;
		elsif (clock'event and clock = '1') then
			case BlinkerState is
				when 0 =>
					if(triggerIn='1')then
						BlinkerState <= 1;
					end if;
				when 1 =>
					if(Counter=LedBlinkDuration)then
						Counter <= 0;
						blinkOut <= '0';
						BlinkerState <= 2;
					else
						blinkOut <= '1';
						Counter <= Counter + 1;
					end if;
				when 2 =>
					if(Counter=LedBlinkDuration)then
						Counter <= 0;
						blinkOut <= '0';
						BlinkerState <= 0;
					else
						Counter <= Counter + 1;
					end if;
				when others =>
					BlinkerState <= 0;
			end case;
		end if;
	end process;
end Behavioral;

