/*
 * Copyright 2023 Robert Bezem (robert@sqrtroot.com)
 * SPDX-License-Identifier: Apache-2.0
*/
#include "log.hpp"
#include <iostream>

#ifdef WIN32
#include <Windows.h>
std::string format_last_error(DWORD lastError) {
  char* strErrorMessage = nullptr;
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
                    FORMAT_MESSAGE_ARGUMENT_ARRAY |
                    FORMAT_MESSAGE_ALLOCATE_BUFFER,  // dwFlags
                nullptr,                             // lpSource
                lastError,                           // dwMessageId
                0,                                   // dwLanguageId
                strErrorMessage,                     // lpBuffer
                0,                                   // nSize
                nullptr                              // varargs args
  );
  deferred([&]() { LocalFree(strErrorMessage); });
  return {strErrorMessage};
}
#endif

volatile LogLevel maxLogLevel = LogLevel::Warning;

fmt::text_style log_color(const LogLevel& lvl) {
  switch (lvl) {
    case LogLevel::Debug: return fmt::fg(fmt::terminal_color::magenta);
    case LogLevel::Info: return fmt::fg(fmt::terminal_color::green);
    case LogLevel::Warning: return fmt::fg(fmt::terminal_color::bright_yellow);
    case LogLevel::Error: return fmt::fg(fmt::terminal_color::red);
  }
  return {};
}

void log(const LogLevel level, const std::string_view message) {
  if (!can_log(level)) return;
//  if (level == LogLevel::Error){__builtin_trap();}
  using namespace std::chrono;
  //  const auto now = zoned_time(current_zone(), system_clock::now())
  //                       .get_local_time()
  //                       .time_since_epoch();
  const auto now = system_clock::now().time_since_epoch();
  fmt::print(log_color(level), "[{}] {:%T} | {}\n", log_char(level), now,
             message);
  std::cout << std::flush;
}
bool can_log(const LogLevel& ll) {
  return ll >= maxLogLevel;
}

char log_char(const LogLevel& ll) {
  using enum LogLevel;
  switch (ll) {
    case Debug: return 'D';
    case Info: return 'I';
    case Warning: return 'W';
    case Error: return 'E';
  }
  return '?';
}
