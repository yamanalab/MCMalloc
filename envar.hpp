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

#include <cassert>  // assert
#include <cstring>  // strcmp
#include <iostream>

namespace {
inline bool strcmps_(const char *str1, const char *str2) {
  if (str1 == nullptr && str2 == nullptr) return true;
  if (str1 == nullptr || str2 == nullptr) return false;
  return strcmp(str1, str2) == 0;
}
template <typename First, typename Second, typename... Rest>
inline bool strcmps_(const First &first, const Second &second,
                     const Rest &... rest) {
  return strcmps_(first, second) || strcmps_(first, rest...);
}

inline bool isStrBlank_(const char *ptr) {
  const char *CONST_NULL_PTR = nullptr;
  return strcmps_(ptr, CONST_NULL_PTR, "");
}
}  // namespace

namespace envar {
inline int GetInt(const char *key, int defVal) {
  const char *val = getenv(key);
  if (isStrBlank_(val)) return defVal;
  return std::atoi(val);
}
inline long GetLong(const char *key, long defVal) {
  const char *val = getenv(key);
  if (isStrBlank_(val)) return defVal;
  return std::atol(val);
}
inline long long GetLongLong(const char *key, long long defVal) {
  const char *val = getenv(key);
  if (isStrBlank_(val)) return defVal;
  return std::atoll(val);
}

inline bool GetBool(const char *key, bool defVal) {
  const char *val = getenv(key);
  if (isStrBlank_(val)) return defVal;
  if (strcmps_(val, "TRUE", "T", "true", "t", "1")) return true;
  if (strcmps_(val, "FALSE", "F", "false", "f", "0")) return false;
  assert(false ||
         !(std::cerr << "wrong env : env key = " << key << ", env val = " << val
                     << std::endl
                     << "select TRUE, T, true, t, 1, FALSE, F, false, f, 0"
                     << std::endl));
  return false;
}
template <typename T>
inline T GetPtr(const char *key, T defVal = nullptr) {
  const char *val = getenv(key);
  if (isStrBlank_(val)) return defVal;
  return (T)std::atoll(val);
}

template <typename T>
inline T Get(const char *key, bool defVal) {
  return GetBool(key, defVal);
}
template <typename T>
inline T Get(const char *key, long long defVal) {
  return GetLongLong(key, defVal);
}
template <typename T>
inline T Get(const char *key, long defVal) {
  return GetLong(key, defVal);
}
template <typename T>
inline T Get(const char *key, int defVal) {
  return GetInt(key, defVal);
}
template <typename T>
inline T Get(const char *key, const char *defVal) {
  const char *val = getenv(key);
  if (isStrBlank_(val)) return defVal;
  return val;
}
template <typename T>
inline T Get(const char *key, T defVal) {
  assert(false || !(std::cerr << "undefined type"));
  return defVal;
}

template <typename T>
inline bool SetPtr(const char *key, T x) {
  char tmpBuf[32] = {0};
  std::sprintf((char *)tmpBuf, "%lld", (long long)x);

  // NOTE
  // 0:overwrite no
  // !0:overwrite yes
  const int change_flag = 1;
  // NOTE There are a few malloc calls. (It depends on whether the key originally exists in the environment variable.)
  // 1~3 times
  int ret = setenv(key, tmpBuf, change_flag);
  if (ret == -1) return false;
  return true;
}
}  // namespace envar
