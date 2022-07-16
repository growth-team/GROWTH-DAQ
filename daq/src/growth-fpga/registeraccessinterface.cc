#include "growth-fpga/registeraccessinterface.hh"

#include <array>
#include <chrono>
#include <memory>
#include <exception>

RegisterAccessInterface::RegisterAccessInterface(RMAPInitiatorPtr rmapInitiator,
                                                 std::shared_ptr<RMAPTargetNode> rmapTargetNode)
    : rmapInitiator_(std::move(rmapInitiator)), rmapTargetNode_(std::move(rmapTargetNode)) {}

u16 RegisterAccessInterface::read16(u32 address) const {
  std::array<u8, 2> data{};
  size_t nRetries = 0;
  while (nRetries < MAX_RETRIES) {
    try {
      rmapInitiator_->read(rmapTargetNode_.get(), address, 2, data.data());
      return (static_cast<u16>(data[0]) << 8) + data[1];
    } catch (const RMAPInitiatorException& e) {
      if (e.getStatus() == RMAPInitiatorException::Timeout) {
        nRetries++;
        if (nRetries < MAX_RETRIES) {
          spdlog::warn("Read (address = 0x{:08x}, length = {} bytes) timed out. Retrying... ({})", address, 2,
                       nRetries);
          continue;
        }
      }
      throw;
    }
  }
  throw std::runtime_error("register access did not success within the retry limit");
}

u32 RegisterAccessInterface::read32(u32 address) const {
  std::array<u8, 4> data{};
  size_t nRetries = 0;
  while (nRetries < MAX_RETRIES) {
    try {
      rmapInitiator_->read(rmapTargetNode_.get(), address, 4, data.data());
      const auto lower16 = (static_cast<u32>(data[0]) << 8) + data[1];
      const auto upper16 = (static_cast<u32>(data[2]) << 8) + data[3];
      return (upper16 << 16) + lower16;
    } catch (const RMAPInitiatorException& e) {
      if (e.getStatus() == RMAPInitiatorException::Timeout) {
        nRetries++;
        if (nRetries < MAX_RETRIES) {
          spdlog::warn("Read (address = 0x{:08x}, length = {} bytes) timed out. Retrying... ({})", address, 4,
                       nRetries);
          continue;
        }
      }
      throw;
    }
  }
  throw std::runtime_error("register access did not success within the retry limit");
}

u64 RegisterAccessInterface::read48(u32 address) const {
  std::array<u8, 6> data{};
  size_t nRetries = 0;
  while (nRetries < MAX_RETRIES) {
    try {
      rmapInitiator_->read(rmapTargetNode_.get(), address, 6, data.data());
      const auto lower16 = (static_cast<u64>(data[0]) << 8) + data[1];
      const auto middle16 = (static_cast<u64>(data[2]) << 8) + data[3];
      const auto upper16 = (static_cast<u64>(data[4]) << 8) + data[5];
      return (upper16 << 24) + (middle16 << 16) + lower16;
    } catch (const RMAPInitiatorException& e) {
      if (e.getStatus() == RMAPInitiatorException::Timeout) {
        nRetries++;
        if (nRetries < MAX_RETRIES) {
          spdlog::warn("Read (address = 0x{:08x}, length = {} bytes) timed out. Retrying... ({})", address, 6,
                       nRetries);
          continue;
        }
      }
      throw;
    }
  }
  throw std::runtime_error("register access did not success within the retry limit");
}

void RegisterAccessInterface::read(u32 address, size_t numBytes, u8* buffer) const {
  size_t nRetries = 0;
  while (nRetries < MAX_RETRIES) {
    try {
      rmapInitiator_->read(rmapTargetNode_.get(), address, numBytes, buffer);
      return;
    } catch (const RMAPInitiatorException& e) {
      if (e.getStatus() == RMAPInitiatorException::Timeout) {
        nRetries++;
        if (nRetries < MAX_RETRIES) {
          spdlog::warn("Read (address = 0x{:08x}, length = {} bytes) timed out. Retrying... ({})", address, numBytes,
                       nRetries);
          continue;
        }
      }
      throw;
    }
  }
  throw std::runtime_error("register access did not success within the retry limit");
}

void RegisterAccessInterface::write(u32 address, u16 value) {
  size_t nRetries = 0;
  while (nRetries < MAX_RETRIES) {
    try {
      const std::array<u8, 2> data{static_cast<u8>((value & 0xFF00) >> 8), static_cast<u8>(value & 0xFF)};
      rmapInitiator_->write(rmapTargetNode_.get(), address, data.data(), 2);
      return;
    } catch (const RMAPInitiatorException& e) {
      if (e.getStatus() == RMAPInitiatorException::Timeout) {
        nRetries++;
        if (nRetries < MAX_RETRIES) {
          spdlog::warn("Write (address = 0x{:08x}, length = {} bytes) timed out. Retrying... ({})", address, 2,
                       nRetries);
          continue;
        }
      }
      throw;
    }
  }
  throw std::runtime_error("register access did not success within the retry limit");
}

void RegisterAccessInterface::write(u32 address, u32 value) {
  size_t nRetries = 0;
  while (nRetries < MAX_RETRIES) {
    try {
      const u16 lower16 = static_cast<u16>(value & 0xFFFF);
      const u16 upper16 = static_cast<u8>(value >> 16);
      const std::array<u8, 4> data{
          static_cast<u8>((lower16 & 0xFF00) >> 8),
          static_cast<u8>(lower16 & 0xFF),
          static_cast<u8>((upper16 & 0xFF00) >> 8),
          static_cast<u8>(upper16 & 0xFF),
      };
      rmapInitiator_->write(rmapTargetNode_.get(), address, data.data(), 4);
      return;
    } catch (const RMAPInitiatorException& e) {
      if (e.getStatus() == RMAPInitiatorException::Timeout) {
        nRetries++;
        if (nRetries < MAX_RETRIES) {
          spdlog::warn("Write (address = 0x{:08x}, length = {} bytes) timed out. Retrying... ({})", address, 4,
                       nRetries);
          continue;
        }
      }
      throw;
    }
  }
  throw std::runtime_error("register access did not success within the retry limit");
}

void RegisterAccessInterface::timeoutSleep() { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); }
