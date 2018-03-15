/*
 * Copyright 2017 Yamana Laboratory, Waseda University
 * Supported by JST CREST Grant Number JPMJCR1503, Japan.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

static int fd = -1;

inline bool fdopen(int& fd);

inline void repeat_char(char c, int n) {
  if (n <= 0) return;
  char s[n];
  memset(s, c, n);
  write(fd, (void*)s, n);
}

inline void mywrite(const char* s, int num, const char* _x) {
  char* x = (char*)_x;
  if (s[0] != 's')
    throw std::runtime_error("invalid format string: required %s");

  int length = 0;
  while (*x) {
    x++;
    length++;
  }
  repeat_char(' ', num - length);
  write(fd, (void*)&x[-length], length);
}
inline void mywrite(const char* s, int num, const int64_t _x) {
  int64_t x = _x;
  char _buf[128] = {};
  char* buf = (char*)_buf + 127;
  if (s[0] != 'd')
    throw std::runtime_error("invalid format string: required %d");

  int length = 0;
  buf[0] = '\0';
  if (x == 0) buf[-(++length)] = '0';
  bool revFlag = x < 0;
  if (revFlag) x = -x;
  while (x > 0) {
    buf[-(++length)] = '0' + (x % 10);
    x /= 10;
  }
  if (revFlag) buf[-(++length)] = '-';
  repeat_char(' ', num - length);
  write(fd, (void*)&buf[-length], length);
}
inline void mywrite(const char* s, int num, const int _x) {
  mywrite(s, num, (const int64_t)_x);
}

inline void mywrite(const char* s, int num, const long unsigned int _x) {
  long unsigned int x = _x;
  char _buf[128] = {};
  char* buf = (char*)_buf + 127;
  if (s[0] != 'u')
    throw std::runtime_error("invalid format string: required %u");

  int length = 0;
  buf[0] = '\0';
  if (x == 0) buf[-(++length)] = '0';
  while (x > 0) {
    buf[-(++length)] = '0' + (x % 10);
    x /= 10;
  }
  repeat_char(' ', num - length);
  write(fd, (void*)&buf[-length], length);
}

inline void mywrite(const char* s, int num, const unsigned char _x) {
  unsigned char x = _x;
  char _buf[128] = {};
  char* buf = (char*)_buf + 127;
  if (s[0] != 'x')
    throw std::runtime_error("invalid format string: required %x");

  int length = 0;
  buf[0] = '\0';
  if (x == 0) buf[-(++length)] = '0';
  while (x > 0) {
    char c = x % 16;
    buf[-(++length)] = c <= 9 ? '0' + c : 'a' + c - 10;
    x /= 16;
  }
  repeat_char(' ', num - length);
  write(fd, (void*)&buf[-length], length);
}

template <typename T>
inline void mywrite(const char* s, int num, const T* _x) {
  uintptr_t x = (uintptr_t)_x;
  char _buf[128] = {};
  char* buf = (char*)_buf + 127;
  if (s[0] != 'p')
    throw std::runtime_error("invalid format string: required %p");

  int length = 0;
  buf[0] = '\0';
  if (x == 0) buf[-(++length)] = '0';
  while (x > 0) {
    char c = x % 16;
    buf[-(++length)] = c <= 9 ? '0' + c : 'a' + c - 10;
    x /= 16;
  }
  buf[-(++length)] = 'x';
  buf[-(++length)] = '0';
  repeat_char(' ', num - length);
  write(fd, (void*)&buf[-length], length);
}

inline void myprintf(const char* s) {
  if (!fdopen(fd)) return;

  int length = 0;
  while (*s) {
    if (*s == '%') {
      write(fd, (void*)&s[-length], length);
      length = 0;
      s++;
      if (*s != '%') {
        throw std::runtime_error(
            "invalid format string: missing arguments after %");
      }
    }
    s++;
    length++;
  }
  write(fd, (void*)&s[-length], length);
}

template <typename T, typename... Args>
inline void myprintf(const char* s, const T& value, const Args&... args) {
  if (!fdopen(fd)) return;

  int length = 0;
  while (*s) {
    if (*s == '%') {
      write(fd, (void*)&s[-length], length);
      length = 0;
      s++;
      int num = 0;
      while ('0' <= *s && *s <= '9') {
        num *= 10;
        num += *s - '0';
        s++;
      }
      if (*s != '%') {
        mywrite(s, num, value);
        s++;
        myprintf(s, args...);
        return;
      }
    }
    s++;
    length++;
  }
  throw std::runtime_error("extra arguments provided to printf");
}

// NOTE: only first fdopen function call is for only single thread
inline bool fdopen(int& fd) {
  if (fd > 0) return true;
  if (fd < -1) return false;

  char* filename = getenv("MYPRINT_TTY");
  filename =
      (filename && filename[0] != '\0') ? filename : (char*)"/dev/stdout";
  if (strcmp(filename, "/dev/null") == 0) {
    fd = -2;
    return false;
  }
  fd = open(filename, O_RDWR | O_CREAT | O_TRUNC,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWOTH);
  myprintf("# output is %s\n", filename);
  return true;
}
