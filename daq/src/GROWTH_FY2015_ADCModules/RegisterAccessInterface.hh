#ifndef REGISTER_ACCESS_INTERFACE_HH_
#define REGISTER_ACCESS_INTERFACE_HH_

#include <GROWTH_FY2015_ADCModules/RMAPHandler.hh>

#include <memory>

class RegisterAccessInterface {
 public:
  RegisterAccessInterface(std::shared_ptr<RMAPHandler> rmapHandler, std::shared_ptr<RMAPTargetNode> rmapTargetNode)
      : rmapHandler_(rmapHandler), rmapTargetNode_(rmapTargetNode) {}
  virtual ~RegisterAccessInterface() = default;

  uint16_t read16(uint32_t address) const {
    std::array<uint8_t, 2> data{};
    rmapHandler_->read(rmapTargetNode_.get(), address, data);
    return (static_cast<uint16_t>(data[0]) << 8) + data[1];
  }

  uint32_t read32(uint32_t address) const {
    std::array<uint8_t, 4> data{};
    rmapHandler_->read(rmapTargetNode_.get(), address, data);
    const auto lower16 = (static_cast<uint32_t>(data[0]) << 8) + data[1];
    const auto upper16 = (static_cast<uint32_t>(data[2]) << 8) + data[3];
    return (upper16 << 16) + lower16;
  }

  uint64_t read48(uint32_t address) const {
    std::array<uint8_t, 6> data{};
    rmapHandler_->read(rmapTargetNode_.get(), address, data);
    const auto lower16 = (static_cast<uint64_t>(data[0]) << 8) + data[1];
    const auto middle16 = (static_cast<uint64_t>(data[2]) << 8) + data[3];
    const auto upper16 = (static_cast<uint64_t>(data[4]) << 8) + data[5];
    return (upper16 << 24) + (middle16 << 16) + lower16;
  }

  void write(uint32_t address, uint16_t value) {
    const std::array<uint8_t, 2> data{(value & 0xFF00) >> 8, value & 0xFF};
    rmapHandler_->write(rmapTargetNode_.get(), address, data);
  }

  void write(uint32_t address, uint32_t value) {
    const uint16_t lower16 = value & 0xFFFF;
    const uint16_t upper16 = value >> 16;
    const std::array<uint8_t, 4> data{
        (lower16 & 0xFF00) >> 8, lower16 & 0xFF,  //
        (upper16 & 0xFF00) >> 8, upper16 & 0xFF,
    };
    rmapHandler_->write(rmapTargetNode_.get(), address, data);
  }

 private:
  std::shared_ptr<RMAPHandler> rmapHandler_{};
  std::shared_ptr<RMAPTargetNode> rmapTargetNode_{};
};

#endif
