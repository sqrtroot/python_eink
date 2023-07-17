/*
 * Copyright 2023 Robert Bezem (robert@sqrtroot.com)
 * SPDX-License-Identifier: Apache-2.0
*/
#include "ScsiDriver.hpp"

#include <fcntl.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>
#include "unistd.h"

ScsiDriver::ScsiDriver(const char *path) {
    fd = open(path, O_RDWR | O_NONBLOCK);
    log(LogLevel::Debug, "constructed scsidriver for {}", path);
    if(fd < 0){
        throw std::runtime_error("Failed to open device");
    }
}

ScsiDriver::ScsiDriver(ScsiDriver &&other) {
    log(LogLevel::Debug, "move constructed scsidriver for fd {}", other.fd);
    this->fd = other.fd;
    other.fd = 0;
}

ScsiDriver &ScsiDriver::operator=(ScsiDriver &&other) {
    log(LogLevel::Debug, "move assigned scsidriver for fd {}", other.fd);
    this->fd = other.fd;
    other.fd = 0;
    return *this;
}

ScsiDriver::~ScsiDriver() {
    if (fd != 0) {
        close(fd);
        log(LogLevel::Debug, "closed fd");
    } else {
        log(LogLevel::Debug, "delete but didn't close fd");
    }
}

std::optional<std::vector<uint8_t>> ScsiDriver::get_data(
        unsigned long dataTransferLength,
        std::span<const uint8_t, 16> commandDescriptorBlock) const {
    std::vector<uint8_t> buffer(dataTransferLength);
    sg_io_hdr_t io_hdr{};
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = commandDescriptorBlock.size();
    io_hdr.cmdp = const_cast<uint8_t *>(commandDescriptorBlock.data());
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len = dataTransferLength;
    io_hdr.dxferp = buffer.data();
    io_hdr.timeout = 1000;
    if (ioctl(fd, SG_IO, &io_hdr) < 0) {
        log(LogLevel::Error, "SG_IO memory read failed {}", strerror(errno));
        return std::nullopt;
    }
    buffer.resize(dataTransferLength - io_hdr.resid);
    return buffer;
}

bool ScsiDriver::write_data(std::span<const uint8_t, 16> commandDescriptorBlock,
                            std::span<const uint8_t> data) const {
    sg_io_hdr_t io_hdr{};
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = commandDescriptorBlock.size();
    io_hdr.cmdp = const_cast<uint8_t *>(commandDescriptorBlock.data());
    io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
    io_hdr.dxfer_len = data.size();
    io_hdr.dxferp = const_cast<uint8_t *>(data.data());
    io_hdr.timeout = 10000;
    if (ioctl(fd, SG_IO, &io_hdr) < 0) {
        log(LogLevel::Error, "SG_IO memory write failed {}", strerror(errno));
        return false;
    }
    return true;
}
