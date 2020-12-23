#ifndef GROWTH_FPGA_REGISTER_ACCESS_INTERFACE_HH_
#define GROWTH_FPGA_REGISTER_ACCESS_INTERFACE_HH_

#include "types.h"
#include "growth-fpga/rmaphandleruart.hh"

#include <array>
#include <memory>

class RegisterAccessInterface {
 public:
  RegisterAccessInterface(std::shared_ptr<RMAPHandlerUART> rmapHandler, std::shared_ptr<RMAPTargetNode> rmapTargetNode)
      : rmapHandler_(std::move(rmapHandler)), rmapTargetNode_(std::move(rmapTargetNode)) {}
  virtual ~RegisterAccessInterface() = default;

  u16 read16(u32 address) const {
    std::array<u8, 2> data{};
    rmapHandler_->read(rmapTargetNode_.get(), address, 2, data.data());
    return (static_cast<u16>(data[0]) << 8) + data[1];
  }

  u32 read32(u32 address) const {
    std::array<u8, 4> data{};
    rmapHandler_->read(rmapTargetNode_.get(), address, 4, data.data());
    const auto lower16 = (static_cast<u32>(data[0]) << 8) + data[1];
    const auto upper16 = (static_cast<u32>(data[2]) << 8) + data[3];
    return (upper16 << 16) + lower16;
  }

  u64 read48(u32 address) const {
    std::array<u8, 6> data{};
    rmapHandler_->read(rmapTargetNode_.get(), address, 6, data.data());
    const auto lower16 = (static_cast<u64>(data[0]) << 8) + data[1];
    const auto middle16 = (static_cast<u64>(data[2]) << 8) + data[3];
    const auto upper16 = (static_cast<u64>(data[4]) << 8) + data[5];
    return (upper16 << 24) + (middle16 << 16) + lower16;
  }

  void read(u32 address, size_t numBytes, u8* buffer) const {
    rmapHandler_->read(rmapTargetNode_.get(), address, numBytes, buffer);
  }

  void write(u32 address, u16 value) {
    const std::array<u8, 2> data{static_cast<u8>((value & 0xFF00) >> 8), static_cast<u8>(value & 0xFF)};
    rmapHandler_->write(rmapTargetNode_.get(), address, 2, data.data());
  }

  void write(u32 address, u32 value) {
    const u16 lower16 = static_cast<u16>(value & 0xFFFF);
    const u16 upper16 = static_cast<u8>(value >> 16);
    const std::array<u8, 4> data{
        static_cast<u8>((lower16 & 0xFF00) >> 8), static_cast<u8>(lower16 & 0xFF),
        static_cast<u8>((upper16 & 0xFF00) >> 8), static_cast<u8>(upper16 & 0xFF),
    };
    rmapHandler_->write(rmapTargetNode_.get(), address, 4, data.data());
  }

 private:
  std::shared_ptr<RMAPHandlerUART> rmapHandler_{};
  std::shared_ptr<RMAPTargetNode> rmapTargetNode_{};
};

#endif