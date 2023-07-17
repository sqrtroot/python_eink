/*
 * Copyright 2023 Robert Bezem (robert@sqrtroot.com)
 * SPDX-License-Identifier: Apache-2.0
*/
#pragma once
#if defined(__cpp_lib_byteswap) && !defined(htobe16)
#include <bit>

#define htobe16(x) std::byteswap(x)
#define htole16(x) std::byteswap(x)
#define be16toh(x) std::byteswap(x)
#define le16toh(x) std::byteswap(x)

#define htobe32(x) std::byteswap(x)
#define htole32(x) std::byteswap(x)
#define be32toh(x) std::byteswap(x)
#define le32toh(x) std::byteswap(x)

#define htobe64(x) std::byteswap(x)
#define htole64(x) std::byteswap(x)
#define be64toh(x) std::byteswap(x)
#define le64toh(x) std::byteswap(x)

#elif (defined(_WIN16) || defined(_WIN32) || defined(_WIN64)) && !defined(__WINDOWS__)

#error "Todo implement this shit"

#elif defined(__linux__) || defined(__CYGWIN__)

#include <endian.h>

#else

#error platform not supported

#endif
