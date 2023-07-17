/*
 * Copyright 2023 Robert Bezem (robert@sqrtroot.com)
 * SPDX-License-Identifier: Apache-2.0
*/
#pragma once

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <chrono>
#include <cstdio>
#include <string_view>

enum class LogLevel {
  Debug = 0,
  Info = 1,
  Warning = 2,
  Error = 3,
};

extern volatile LogLevel maxLogLevel;

bool can_log(const LogLevel& ll);

fmt::text_style log_color(const LogLevel& lvl);

char log_char(const LogLevel& ll);

void log(LogLevel level, std::string_view message);

template <typename... T>
void log(LogLevel const level, fmt::format_string<T...> message, T&&... ts) {
  if (!can_log(level)) return;
  log(level, fmt::format(message, std::forward<T>(ts)...));
}

#ifdef WIN32
#include <Windows.h>
std::string format_last_error(DWORD lastError);
#endif