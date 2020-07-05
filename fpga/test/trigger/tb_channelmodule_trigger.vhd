library ieee, work;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;
use work.UserModule_Library.all;
use work.TestWaveform.all;

entity tb_channelmodule_trigger is

end tb_channelmodule_trigger;

architecture behavioral of tb_channelmodule_trigger is
    signal ChModule2InternalModule : Signal_ChModule2InternalModule;
    signal AdcDataIn               : std_logic_vector(ADCResolution-1 downto 0) := (others => '0');
    signal DelayedAdcDataIn        : std_logic_vector(ADCResolution-1 downto 0) := (others => '0');
    signal TriggerOut              : std_logic := '0';
    signal Veto                    : std_logic := '0';
    signal TriggerCountOut         : std_logic_vector(31 downto 0) := (others => '0');
    signal Clock                   : std_logic := '0';
    signal GlobalReset             : std_logic := '0';

    constant DEPTH_OF_DELAY : integer := 16;
    constant NUMBER_OF_SAMPLES_PER_TRIGGER : integer := 512;

    constant CLOCK_PERIOD : time := 20 ns;
    constant SIMULATION_DURATION : time := 30 us;
    signal elapsed_time : time:= 0 ns;

    signal clock_counter : integer := 0;

    component UserModule_ChModule_Trigger is
    port(
        ChModule2InternalModule : in  Signal_ChModule2InternalModule;
        -- Prompt ADC data
        AdcDataIn               : in  std_logic_vector(ADCResolution-1 downto 0);
        -- Delayed ADC data (used for baseline calculation in the baseline-corrected trigger mode)
        DelayedAdcDataIn        : in  std_logic_vector(ADCResolution-1 downto 0);
        --
        TriggerOut              : out std_logic;
        Veto                    : out std_logic;
        --
        TriggerCountOut         : out std_logic_vector(31 downto 0);
        --clock and reset
        Clock                   : in  std_logic;
        GlobalReset             : in  std_logic
        );
    end component;


begin
    GlobalReset <= '1'; -- active low signal. '1' means no reset.

    -- Trigger module setting
    ChModule2InternalModule.AdcPowerDown      <= '0';
    ChModule2InternalModule.TriggerMode       <= Mode_3_StartingTh_NumberOfSamples_ClosingTh;
    ChModule2InternalModule.CommonGateIn      <= '0'; -- Unused
    ChModule2InternalModule.CPUTrigger        <= '0'; -- Unused
    ChModule2InternalModule.TriggerBus         <= (others => '0');
    ChModule2InternalModule.TriggerBusMask    <= (others => '0');
    ChModule2InternalModule.DepthOfDelay      <= conv_std_logic_vector(DEPTH_OF_DELAY, WidthOfDepthOfDelay);
    ChModule2InternalModule.NumberOfSamples   <= conv_std_logic_vector(NUMBER_OF_SAMPLES_PER_TRIGGER, WidthOfNumberOfSamples);
    ChModule2InternalModule.ThresholdStarting <= conv_std_logic_vector(200, ADCResolution);
    ChModule2InternalModule.ThresholdClosing  <= conv_std_logic_vector(150, ADCResolution);
    ChModule2InternalModule.SizeOfHeader      <= (others => '0'); -- Not used in simulation
    ChModule2InternalModule.TriggerCount      <= (others => '0');
    ChModule2InternalModule.RealTime          <= (others => '0');
    ChModule2InternalModule.Veto              <= '0';

     -- Clock generation
    process
    begin
        if elapsed_time < SIMULATION_DURATION then
            wait for CLOCK_PERIOD / 2;
            Clock <= not Clock;
            elapsed_time <= elapsed_time + CLOCK_PERIOD / 2;
        else
            wait;
        end if;
    end process;

    -- Stimulus
    process(Clock)
    begin
        if rising_edge(Clock) then
            clock_counter <= clock_counter + 1;
            if clock_counter < TestWaveformGaus'length then
                AdcDataIn <= TestWaveformGaus(clock_counter);
                if clock_counter > DEPTH_OF_DELAY then
                    DelayedAdcDataIn <= TestWaveformGaus(clock_counter - DEPTH_OF_DELAY);
                else
                    DelayedAdcDataIn <= (others => '0');
                end if;
            end if;
        end if;
    end process;

    inst_trigger: UserModule_ChModule_Trigger
        port map(
        ChModule2InternalModule => ChModule2InternalModule,
        AdcDataIn               => AdcDataIn,
        DelayedAdcDataIn        => DelayedAdcDataIn,
        TriggerOut              => TriggerOut,
        Veto                    => Veto,
        TriggerCountOut         => TriggerCountOut,
        Clock                   => Clock,
        GlobalReset             => GlobalReset
        );

end behavioral;
