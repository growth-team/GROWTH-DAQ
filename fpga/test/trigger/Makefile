
SRC_DIR = ../../sources/VHDL
IBUS_DIR = ../../sources/iBus

WORK_DIR_OPT = --workdir=workdir_channelmodule_trigger

all: tb_channelmodule_trigger.vcd

tb_channelmodule_trigger.vcd: tb_channelmodule_trigger.vhd $(SRC_DIR)/UserModule_ChannelModule_Trigger.vhdl $(SRC_DIR)/UserModule_Library.vhdl
	install -d workdir_channelmodule_trigger
	ghdl -a $(WORK_DIR_OPT) --ieee=synopsys $(SRC_DIR)/UserModule_Library.vhdl
	ghdl -a $(WORK_DIR_OPT) --ieee=synopsys $(IBUS_DIR)/iBus_AddressMap.vhdl
	ghdl -a $(WORK_DIR_OPT) --ieee=synopsys $(IBUS_DIR)/iBus_Library.vhdl
	ghdl -a $(WORK_DIR_OPT) --ieee=synopsys -fexplicit $(SRC_DIR)/UserModule_ChannelModule_Trigger.vhdl
	ghdl -a $(WORK_DIR_OPT) --ieee=synopsys testwaveform.vhd
	ghdl -a $(WORK_DIR_OPT) --ieee=synopsys tb_channelmodule_trigger.vhd

	ghdl -e $(WORK_DIR_OPT) --ieee=synopsys -fexplicit tb_channelmodule_trigger
	ghdl -r $(WORK_DIR_OPT) tb_channelmodule_trigger --vcd=$@ --stop-time=600us

clean:
	rm -rf workdir_channelmodule_trigger
	rm -rf tb_channelmodule_trigger
	rm -rf *.o
