/*
 * Copyright 2023 Robert Bezem (robert@sqrtroot.com)
 * SPDX-License-Identifier: Apache-2.0
*/
#include "ScsiDriver.hpp"

ScsiDriver::ScsiDriver(const char* path) {
  hDev = CreateFile(path,                                  // file name
                    (GENERIC_READ | GENERIC_WRITE),        // access mode
                    (FILE_SHARE_READ | FILE_SHARE_WRITE),  // share mode
                    nullptr,                               // SD
                    OPEN_EXISTING,                         // how to create
                    0,                                     // file attributes
                    nullptr                                // handle to template file
  );
}
ScsiDriver::ScsiDriver(ScsiDriver&& other) {
  this->hDev = other.hDev;
  other.hDev = 0;
}
ScsiDriver& ScsiDriver::operator=(ScsiDriver&& other) {
  this->hDev = other.hDev;
  other.hDev = 0;
  return *this;
}
ScsiDriver::~ScsiDriver() {
  if (hDev != 0) CloseHandle(hDev);
}
std::optional<std::vector<uint8_t>> ScsiDriver::get_data(
    unsigned long                dataTransferLength,
    std::span<const uint8_t, 16> commandDescriptorBlock) const {
  std::vector<uint8_t>     buffer(dataTransferLength);
  unsigned long            dwReturnBytes = 0;
  SCSI_PASS_THROUGH_DIRECT scsiPassThroughDirect{
      .Length             = sizeof(SCSI_PASS_THROUGH_DIRECT),
      .ScsiStatus         = 0,
      .PathId             = 0,
      .TargetId           = 0,
      .Lun                = 0,
      .CdbLength          = 16,
      .SenseInfoLength    = 0,
      .DataIn             = SCSI_IOCTL_DATA_IN,
      .DataTransferLength = dataTransferLength,
      .TimeOutValue       = 5,
      .DataBuffer         = (void*)buffer.data(),
      .SenseInfoOffset    = 0,
  };

  std::memcpy(scsiPassThroughDirect.Cdb, commandDescriptorBlock.data(), 16);
  if (!DeviceIoControl(hDev, IOCTL_SCSI_PASS_THROUGH_DIRECT, &scsiPassThroughDirect,
                       sizeof(SCSI_PASS_THROUGH_DIRECT),  // sizeof( TSPTWBData),
                       &scsiPassThroughDirect,
                       sizeof(SCSI_PASS_THROUGH_DIRECT),  // sizeof( TSPTWBData),
                       &dwReturnBytes, nullptr)) {
    log(LogLevel::Error, "Couldn't retrieve data from SCSI device, Error: {}",
        format_last_error(GetLastError()));
    return std::nullopt;
  }

  buffer.resize(dwReturnBytes);
  log(LogLevel::Debug, "Data from scsi call {}",
      fmt::format("{:x}", fmt::join(buffer, ",")));
  return buffer;
}

bool ScsiDriver::write_data(std::span<const uint8_t, 16> commandDescriptorBlock,
                            std::span<const uint8_t>     data) const {
  unsigned long            dwReturnBytes = 0;
  SCSI_PASS_THROUGH_DIRECT scsiPassThroughDirect{
      .Length             = sizeof(SCSI_PASS_THROUGH_DIRECT),
      .ScsiStatus         = 0,
      .PathId             = 0,
      .TargetId           = 0,
      .Lun                = 0,
      .CdbLength          = 16,
      .SenseInfoLength    = 0,
      .DataIn             = SCSI_IOCTL_DATA_OUT,
      .DataTransferLength = data.size(),
      .TimeOutValue       = 5,
      .DataBuffer         = (void*)data.data(),
      .SenseInfoOffset    = 0,
  };
  std::memcpy(scsiPassThroughDirect.Cdb, commandDescriptorBlock.data(), 16);
  return DeviceIoControl(
      hDev,
      IOCTL_SCSI_PASS_THROUGH_DIRECT,  // IOCTL_SCSI_PASS_THROUGH_DIRECT,//IOCTL_SCSI_PASS_THROUGH,
      &scsiPassThroughDirect,
      sizeof(SCSI_PASS_THROUGH_DIRECT),  //+sizeof(gSPTDataBuf),
      &scsiPassThroughDirect,
      sizeof(SCSI_PASS_THROUGH_DIRECT),  //+sizeof(gSPTDataBuf),
      &dwReturnBytes, nullptr);
}
