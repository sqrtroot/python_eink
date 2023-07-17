/*
 * Copyright 2023 Robert Bezem (robert@sqrtroot.com)
 * SPDX-License-Identifier: Apache-2.0
*/
#pragma once
#include <filesystem>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <optional>
#include <utility>
#include "IT8951.hpp"
using namespace cv;

class ScreenManager {
  bool cleared = true;
  IT8951                 it;
  const IT8951SystemInfo info;
  int                    rotation = 1;

  static std::optional<Mat> load_image(const std::filesystem::path& image_path);

  Mat scale_image_to_display(const Mat& img) const;

  void display_image(const Mat& img, const IT8951DisplayArea& area);

 public:
  ScreenManager(IT8951&& it);
  ScreenManager(IT8951&& it, double vCom);
  ScreenManager(const ScreenManager&)            = delete;
  ScreenManager& operator=(const ScreenManager&) = delete;
  ScreenManager(ScreenManager&& other) : it(std::move(other.it)), info(other.info), rotation(other.rotation){};
//    ScreenManager& operator=(ScreenManager&& other) {
//      std::swap(this->it, other.it);
//      const_cast<IT8951SystemInfo&>(info) = other.info;
//      return *this;
//    };

  void display(const std::filesystem::path& path);

  void clear_screen();
  void set_vcom(double vcom);
  void set_rotation(int rotation);
};
