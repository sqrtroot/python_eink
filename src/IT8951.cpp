/*
 * Copyright 2023 Robert Bezem (robert@sqrtroot.com)
 * SPDX-License-Identifier: Apache-2.0
*/
#include "IT8951.hpp"
#include <cassert>
#include "EndianConversion.h"
#include "log.hpp"
#include <algorithm>

#ifdef __linux__

#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>
#include <cstring>

#endif

/**
 * @warning ONLY USE THIS ON TYPES OF WHICH ALL MEMBERS ARE UINT32_T
 * @tparam T Struct of which all members are uint32_t and need to be swapped
 * @param t original value
 * @return new swapped value;
 */
template<typename T>
T host_to_be_uint32t_members(const T &t) {
    auto copy = t;
    auto copyptr = reinterpret_cast<uint32_t *>(&copy);
    for (int i = 0; i < sizeof(T) / sizeof(uint32_t); i++) {
        copyptr[i] = htobe32(copyptr[i]);
    }
    return copy;
}

/**
 * @warning ONLY USE THIS ON TYPES OF WHICH ALL MEMBERS ARE UINT32_T
 * @tparam T Struct of which all members are uint32_t and need to be swapped
 * @param t original value
 * @return new swapped value;
 */
template<typename T>
T be_to_host_uint32t_members(const T &t) {
    auto copy = t;
    auto copyptr = reinterpret_cast<uint32_t *>(&copy);
    for (int i = 0; i < sizeof(T) / sizeof(uint32_t); i++) {
        copyptr[i] = be32toh(copyptr[i]);
    }
    return copy;
}

void IT8951::wait_until_ready() const {
    // status == 0 means ready else TCon engine is busy
    while (read_register(0x18001224).value_or(0xFFFF) & 0xFFFF);
}

constexpr std::array<uint8_t, 16> make_command_cdb_data(uint32_t address,
                                                        uint8_t command) {
    // clang-format off
  return {{
      0xFE, 0x00,
      // Byte 2-5 register address BigEndian
      (uint8_t)((address >> 24) & 0xFF),
      (uint8_t)((address >> 16) & 0xFF),
      (uint8_t)((address >> 8) & 0xFF),
      (uint8_t)((address)&0xFF),
      command,  // IT8951 USB WriteReg
      0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  }};
  // clang-format on
}

bool IT8951::write_register(uint32_t address, uint32_t value) const {
    const auto cdb_data = make_command_cdb_data(address, 0x84);
    std::array<uint8_t, 4> data_big_endian{
            static_cast<unsigned char>(value >> 24), static_cast<unsigned char>(value >> 16),
            static_cast<unsigned char>(value >> 8), static_cast<unsigned char>(value)};
    return driver.write_data(cdb_data, data_big_endian);
}

std::optional<uint32_t> IT8951::read_register(uint32_t address) const {
    const auto cdb_data = make_command_cdb_data(address, 0x83);
    auto x = driver.get_data(1, cdb_data);
    if (!x || x->size() < 4) { return std::nullopt; }
    return be32toh(*reinterpret_cast<uint32_t *>(x->data()));
}

std::optional<IT8951SystemInfo> IT8951::get_system_info() {
    if (cached_system_info) { return cached_system_info; }
    // clang-format off
  const std::array<uint8_t, 16> cdb_data{{
                                             0xFE, 0x00, 0x38,  // Signature
                                             0x39, 0x35, 0x31, 0x80, 0x00,  // Version
                                             0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
                                         }};
  // clang-format on
    auto x = driver.get_data(sizeof(IT8951SystemInfo), cdb_data);
    if (!x.has_value() || x->empty()) { return std::nullopt; }
    const auto f = reinterpret_cast<const IT8951SystemInfo *>(x->data());
    cached_system_info = be_to_host_uint32t_members<IT8951SystemInfo>(*f);
    return cached_system_info;
}

bool IT8951::is_it8951() const {
    // clang-format off
  const auto cdb_data =
      std::array<uint8_t, 16>({0x12,  // Inquiry
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
  // clang-format on
    const auto x = driver.get_data(0x28, cdb_data);
    if (!x.has_value() || x->empty()) { return false; }
    const std::string_view expected = "Generic Storage RamDisc 1.00";
    return expected == std::string_view((const char *) x->data() + 8, expected.size());
}

void IT8951::load_image_area(const IT8951Area &area, std::span<uint8_t> pixelData) {
    load_image_area({.address = get_system_info()->uiImageBufBase, .area = area},
                    pixelData);
}

void IT8951::load_image_area(const IT8951ImgLoadArea &area,
                             std::span<uint8_t> pixelData) const {
    const auto size = area.area.w * area.area.h;
    assert(size <= pixelData.size());
    // Largest image in 1 packet is 247x247 pixels
    if (size > SPT_BUF_SIZE) {
        log(LogLevel::Debug, "Image is too big to send at once");

        uint32_t offset = 0;
        uint32_t lines = (SPT_BUF_SIZE - sizeof(IT8951ImgLoadArea)) / area.area.w;

        while (offset < size) {
            if (offset / area.area.w + lines > area.area.h) {
                lines = area.area.h - offset / area.area.w;
            }
            log(LogLevel::Debug, "Sending chunk of {} lines to line offset {}", lines,
            area.area.y + offset / area.area.w);
            load_image_area({.address = area.address,
                                    .area{.x = area.area.x,
                                            .y = area.area.y + offset / area.area.w,
                                            .w = area.area.w,
                                            .h = lines}},
                            pixelData.subspan(offset, lines * area.area.w));
            offset += lines * area.area.w;
        }
        return;
    }
    auto prepared_area = host_to_be_uint32t_members(area);
    // clang-format off
  const auto cdb_data =
      std::array<uint8_t, 16>({0xFE,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0xA2, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
  // clang-format on
    auto data_buffer =
            std::vector<uint8_t>(sizeof(prepared_area) + area.area.w * area.area.h);
    // order between memcpy and copy_n is different :/
    // dest src size
    std::memcpy(data_buffer.data(), &prepared_area, sizeof(prepared_area));
    // src size dest
    std::copy_n(pixelData.begin(), area.area.w * area.area.h,
                data_buffer.begin() + sizeof(prepared_area));
    driver.write_data(cdb_data, data_buffer);
    log(LogLevel::Debug, "Sent image of {}x{} to {},{}", area.area.w, area.area.h,
    area.area.x, area.area.y);
}

void IT8951::display_image_area(const IT8951DisplayArea &area) const {
    auto prepared_area = host_to_be_uint32t_members(area);
    // clang-format off
  const auto cdb_data =
      std::array<uint8_t, 16>({0xFE,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x94, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
  // clang-format on
    std::array<uint8_t, sizeof(prepared_area)> buf{};
    std::memcpy(buf.data(), &prepared_area, buf.size());
    driver.write_data(cdb_data, buf);
}

void IT8951::display_image_area(const IT8951Area &area, WaveMode wavemode) {
    display_image_area({.address    = get_system_info()->uiImageBufBase,
                               .wavemode   = wavemode,
                               .area       = area,
                               .wait_ready = 0});
}

void IT8951::clear_area(const IT8951Area &area) const {
    display_image_area(
            {.address = 0, .wavemode = WaveMode::Init, .area = area, .wait_ready = 0});
}

void IT8951::set_vcom(double vcom) const {
    const auto VCOM = (uint16_t) (std::abs(vcom) * 1000);
    log(LogLevel::Debug, "RAW VCOM value: {}", VCOM);
    // clang-format off
  const auto cdb_data =
      std::array<uint8_t, 16>({0xFE, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0xA3, // PMIC Command
                                (uint8_t)((VCOM >> 8) & 0xFF),
                                (uint8_t) (VCOM & 0xFF),
                                0x00, // Set VCOM
                                0x01,//Set Power
                                0x00,//Power on
                                0x00, 0x00, 0x00, 0x00});
  // clang-format on
    driver.write_data(cdb_data, {});
}