// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif //_WIN32
#include <stdio.h>
#include <cerrno>
#include <filesystem>
#include <string>

#ifndef O_BINARY
#define O_BINARY 0
#endif

#include <sys/stat.h> //O_CREAT mode defines
#ifdef _MSC_VER
#define MY_S_IREADWRITE (_S_IREAD | _S_IWRITE)
#else
#define MY_S_IREADWRITE (S_IRUSR | S_IWUSR)
#endif

/*!
 * The paths of the blocks are UTF-8 text. On Windows, a std::filesystem::path
 * made from a std::string uses the ANSI code page: make it from UTF-8.
 */
inline std::filesystem::path utf8Path(const std::string &s)
{
    return std::filesystem::path(std::u8string(s.begin(), s.end()));
}

inline int openFileForRead(const std::string &filepath)
{
    #ifdef _WIN32
    return _wopen(utf8Path(filepath).c_str(), O_RDONLY | O_BINARY);
    #else
    return open(filepath.c_str(), O_RDONLY | O_BINARY);
    #endif
}

inline int openFileForWrite(const std::string &filepath)
{
    #ifdef _WIN32
    return _wopen(utf8Path(filepath).c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, MY_S_IREADWRITE);
    #else
    return open(filepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, MY_S_IREADWRITE);
    #endif
}
