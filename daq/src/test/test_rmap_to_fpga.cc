/*
 * test_rmap_to_fpga.cc
 *
 *  Created on: Jun 9, 2015
 *      Author: yuasa
 */

#include "../spacewireifoveruart.hh"
#include "SpaceWireRMAPLibrary/RMAP.hh"
#include "SpaceWireRMAPLibrary/SpaceWire.hh"

/*
 constant BA                                  : std_logic_vector(15 downto 0) := InitialAddress;  --Base Address
 constant AddressOf_TriggerModeRegister       : std_logic_vector(15 downto 0) := BA+x"0002";
 constant AddressOf_NumberOfSamplesRegister   : std_logic_vector(15 downto 0) := BA+x"0004";
 constant AddressOf_ThresholdStartingRegister : std_logic_vector(15 downto 0) := BA+x"0006";
 constant AddressOf_ThresholdClosingRegister  : std_logic_vector(15 downto 0) := BA+x"0008";
 constant AddressOf_AdcPowerDownModeRegister  : std_logic_vector(15 downto 0) := BA+x"000a";
 constant AddressOf_DepthOfDelayRegister      : std_logic_vector(15 downto 0) := BA+x"000c";
 constant AddressOf_LivetimeRegisterL         : std_logic_vector(15 downto 0) := BA+x"000e";
 constant AddressOf_LivetimeRegisterH         : std_logic_vector(15 downto 0) := BA+x"0010";
 constant AddressOf_CurrentAdcDataRegister    : std_logic_vector(15 downto 0) := BA+x"0012";
 constant AddressOf_CPUTrigger                : std_logic_vector(15 downto 0) := BA+x"0014";

 constant InitialAddressOf_ChModule_0  : std_logic_vector(15 downto 0) := x"1000";
 constant FinalAddressOf_ChModule_0    : std_logic_vector(15 downto 0) := x"10fe";
 constant InitialAddressOf_ChModule_1  : std_logic_vector(15 downto 0) := x"1100";
 constant FinalAddressOf_ChModule_1    : std_logic_vector(15 downto 0) := x"11fe";
 constant InitialAddressOf_ChModule_2  : std_logic_vector(15 downto 0) := x"1200";
 constant FinalAddressOf_ChModule_2    : std_logic_vector(15 downto 0) := x"12fe";
 constant InitialAddressOf_ChModule_3  : std_logic_vector(15 downto 0) := x"1300";
 constant FinalAddressOf_ChModule_3    : std_logic_vector(15 downto 0) := x"13fe";

 *
 */

void dump(u8* buffer) {
  using namespace std;
  for (size_t i = 0; i < 2; i++) {
    cout << "0x" << hex << right << setw(2) << setfill('0') << (u32)buffer[i] << " ";
  }
  cout << dec << endl;
}

int main(int argc, char* argv[]) {
  using namespace std;
  if (argc < 2) {
    cerr << "Provide serial port device name (e.g. /dev/tty.usb-abcd" << endl;
    exit(-1);
  }
  // initialize
  std::string deviceName(argv[1]);
  SpaceWireIFOverUART* spwif = new SpaceWireIFOverUART(deviceName);
  spwif->open();

  RMAPEngine* rmapEngine = new RMAPEngine(spwif);
  rmapEngine->start();

  CxxUtilities::Condition c;
  c.wait(1000);

  RMAPTargetNode target;
  target.setTargetLogicalAddress(0xFE);
  target.setInitiatorLogicalAddress(0xFE);
  target.setDefaultKey(0x00);
  target.setReplyAddress({});

  RMAPInitiator* rmapInitiator = new RMAPInitiator(rmapEngine);
  u8 buffer[2];
  try {
    rmapInitiator->read(&target, 0x00001112, 2, buffer);
    cout << "Read done" << endl;
    dump(buffer);
    rmapInitiator->read(&target, 0x00001030, 2, buffer);
    cout << "Read done" << endl;
    dump(buffer);
    rmapInitiator->read(&target, 0x00001006, 2, buffer);
    cout << "Read done" << endl;
    dump(buffer);
    // write
    u16 threshold = 540;
    buffer[1]          = threshold % 0x100;
    buffer[0]          = threshold / 0x100;
    rmapInitiator->write(&target, 0x00001006, buffer, 2);
    cout << "Write done" << endl;
    rmapInitiator->read(&target, 0x00001006, 2, buffer);
    cout << "Read done" << endl;
    dump(buffer);

  } catch (RMAPInitiatorException& e) {
    cout << "Exception in RMAPInitiator::read(): " << e.toString() << endl;
    exit(-1);
  }
  // finalize
  rmapEngine->stop();
  spwif->close();
  delete rmapEngine;
  delete spwif;
}
