/*
 * Copyright 2023 Robert Bezem (robert@sqrtroot.com)
 * SPDX-License-Identifier: Apache-2.0
*/
#include "ScreenManager.hpp"
#include <thread>
#include "log.hpp"

ScreenManager::ScreenManager(IT8951&& it)
    : it(std::forward<IT8951&&>(it)),
      info(this->it.get_system_info().value_or<IT8951SystemInfo>({})) {
  log(LogLevel::Debug, "Created screen manager for screen {}x{}", info.uiWidth,
      info.uiHeight);
  log(LogLevel::Error, "Info: {}",
      fmt::join(std::span<uint32_t>((uint32_t*)&info, 28), ", "));
}
ScreenManager::ScreenManager(IT8951&& it, double vcom)
    : ScreenManager(std::forward<IT8951&&>(it)) {
  it.set_vcom(vcom);
}

std::optional<Mat> ScreenManager::load_image(const std::filesystem::path& image_path) {
  const auto imgpath = image_path.string();
  log(LogLevel::Info, "Trying to load {}", imgpath);
  Mat img = imread(imgpath, IMREAD_GRAYSCALE);
  if (img.empty()) {
    log(LogLevel::Error, "Image is empty\n");
    return std::nullopt;
  }
  log(LogLevel::Info, "Opened image, size: {}x{} ({}bpp)", img.cols, img.rows,
      img.elemSize());
  return img;
}

Mat ScreenManager::scale_image_to_display(const Mat& img) const {
  Mat resized_down = img;
  log(LogLevel::Info, "Resizing image from {}x{} to {}x{}", img.cols, img.rows, info.uiWidth, info.uiHeight);
  resize(resized_down, resized_down, Size(info.uiWidth, info.uiHeight));
  return resized_down;
}

void ScreenManager::display_image(const Mat& img, const IT8951DisplayArea& area) {
  // it.wait_until_ready(); // doesn't work on linux
  it.load_image_area({.address = area.address, .area = area.area},
                     std::span(img.data, img.size().area()));
    it.display_image_area(area);
    if(cleared){
        it.display_image_area(area);
        cleared = false;
    }
}

void ScreenManager::display(const std::filesystem::path& path) {
  const auto img = load_image(path);
  if (!img.has_value()) {
    log(LogLevel::Warning, "Couldn't load image {}", path.string());
    return;
  }
  Mat rotated;
  cv::rotate(*img, rotated, rotation);
  log(LogLevel::Info, "Rotating image {}", rotation);
  const auto scaled_img = scale_image_to_display(rotated);
  display_image(scaled_img, {.address    = info.uiImageBufBase,
                             .wavemode   = WaveMode::GC16,
                             .area       = {.x = (info.uiWidth - scaled_img.cols) / 2,
                                            .y = (info.uiHeight - scaled_img.rows) / 2,
                                            .w = static_cast<uint32_t>(scaled_img.cols),
                                            .h = static_cast<uint32_t>(scaled_img.rows)},
                             .wait_ready = 0});
}

void ScreenManager::clear_screen() {
  cleared = true;
  it.clear_area({.x = 0, .y = 0, .w = info.uiWidth, .h = info.uiHeight});
}
void ScreenManager::set_vcom(double vcom) { /*it.set_vcom(vcom);*/ }
void ScreenManager::set_rotation(int new_rotation) {
  this->rotation = new_rotation;  // should be from:
  // enum RotateFlags {
  //     ROTATE_90_CLOCKWISE = 0, //!<Rotate 90 degrees clockwise
  //     ROTATE_180 = 1, //!<Rotate 180 degrees clockwise
  //     ROTATE_90_COUNTERCLOCKWISE = 2, //!<Rotate 270 degrees clockwise
  // };
}
