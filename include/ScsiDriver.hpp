/*
 * Copyright 2023 Robert Bezem (robert@sqrtroot.com)
 * SPDX-License-Identifier: Apache-2.0
*/
#pragma once
#ifdef WIN32
#include <Windows.h>
#include <ntddscsi.h>

#undef min
#undef max
#endif

#include "log.hpp"
#include <stdint.h>
#include <array>
#include <optional>
#include <span>
#include <vector>

#define SPT_BUF_SIZE (60 * 1024)

class ScsiDriver {
#ifdef WIN32
    HANDLE hDev = nullptr;
#endif
#ifdef __linux__
    int fd = 0;
#endif
public:
    explicit ScsiDriver(const char *path);

    ScsiDriver(ScsiDriver &) = delete;

    ScsiDriver(ScsiDriver &&);

    ScsiDriver &operator=(ScsiDriver &) = delete;

    ScsiDriver &operator=(ScsiDriver &&);

    ~ScsiDriver();

    std::optional<std::vector<uint8_t>> get_data(
            unsigned long dataTransferLength,
            std::span<const uint8_t, 16> commandDescriptorBlock) const;

    bool write_data(std::span<const uint8_t, 16> commandDescriptorBlock,
                    std::span<const uint8_t> data) const;

#ifdef WIN32
    HANDLE get_handle() const { return hDev; }
#endif
#ifdef __linux__

    int get_fd() const { return fd; }

#endif
};
