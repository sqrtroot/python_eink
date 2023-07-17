/*
 * Copyright 2023 Robert Bezem (robert@sqrtroot.com)
 * SPDX-License-Identifier: Apache-2.0
*/
#pragma once
#include <memory>
#include <vector>
#include "ScsiDriver.hpp"

struct IT8951SystemInfo {
  unsigned int uiStandardCmdNo;  // Standard command number2T-con Communication Protocol
  unsigned int uiExtendCmdNo;    // Extend command number
  unsigned int uiSignature;      // 31 35 39 38h (8951)
  unsigned int uiVersion;        // command table version
  unsigned int uiWidth;          // Panel Width
  unsigned int uiHeight;         // Panel Height
  unsigned int uiUpdateBufBase;  // Update Buffer Address
  unsigned int uiImageBufBase;   // Image Buffer Address
  unsigned int uiTemperatureNo;  // Temperature segment number
  unsigned int uiModeNo;         // Display mode number
  unsigned int uiFrameCount[8];  // Frame count for each mode(8).
  unsigned int uiNumImgBuf;      // Numbers of Image buffer
  unsigned int uiReserved[9];    // Don’t care
  void*        lpCmdDebugDatas[1];  // Command table pointer
};

struct IT8951Area {
  uint32_t x;
  uint32_t y;
  uint32_t w;
  uint32_t h;
};

struct IT8951ImgLoadArea {
  uint32_t   address;
  IT8951Area area;
};

enum class WaveMode : uint32_t {
  Init  = 0,
  DU    = 1,
  GC16  = 2,
  GL16  = 3,
  GLR16 = 4,
  GLD16 = 5,
  DU4   = 6,
  A2    = 7
};
struct IT8951DisplayArea {
  uint32_t   address;
  WaveMode   wavemode;
  IT8951Area area;
  uint32_t   wait_ready;
};

class IT8951 {
  ScsiDriver                      driver;
  std::optional<IT8951SystemInfo> cached_system_info;

 public:
  explicit IT8951(ScsiDriver&& driver) : driver(std::forward<ScsiDriver>(driver)){};
  IT8951(IT8951&& other) noexcept : driver(std::move(other.driver)) {
    log(LogLevel::Debug, "moved it8951");
    std::swap(this->cached_system_info, other.cached_system_info);
  }

  [[nodiscard]] bool is_it8951() const;

  std::optional<IT8951SystemInfo> get_system_info();

  void set_vcom(double vcom) const;

  [[nodiscard]] std::optional<uint32_t> read_register(uint32_t address) const;

  [[nodiscard]] bool write_register(uint32_t address, uint32_t value) const;

  void wait_until_ready() const;

  void load_image_area(const IT8951ImgLoadArea& area, std::span<uint8_t> pixelData) const;
  void load_image_area(const IT8951Area& area, std::span<uint8_t> pixelData);

  void display_image_area(const IT8951DisplayArea& area) const;
  void display_image_area(const IT8951Area& area, WaveMode wavemode);

  void clear_area(const IT8951Area& area) const;
};