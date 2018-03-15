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

#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>

// NOTE: This value has no means.
#define SIGNATURE (0xDEADC0DE)

#define PAGE_SIZE 4096
#define ALIGN(v, n)                                         \
  ((((uintptr_t)v) % (n) == 0 ? ((uintptr_t)v) / (n)        \
                              : ((uintptr_t)v) / (n) + 1) * \
   (n))
#define ALIGN_CHECK(ptr, aligenment) \
  (((ptr) != 0) && ((uintptr_t)(ptr) % (aligenment)) == 0)

#define CONCAT2(x, y) x##y
#define CONCAT(x, y) CONCAT2(x, y)

#define defer(func) shared_ptr<void> CONCAT2(_, __LINE__)(nullptr, func);

namespace {
inline int _pthread_mutex_lock(pthread_mutex_t* mtx) {
  int r = pthread_mutex_lock(mtx);
  // eassert(r == 0, "can not lock : code = %d\n", r);
  return r;
}
inline int __pthread_mutex_unlock(pthread_mutex_t** mtx) {
  int r = pthread_mutex_unlock(*mtx);
  // eassert(r == 0, "can not unlock : code = %d\n", r);
  return r;
}

#define SCOPED_LOCK(m)                                      \
  _pthread_mutex_lock(&m);                                  \
  __attribute__((__cleanup__(__pthread_mutex_unlock)))      \
      pthread_mutex_t* CONCAT(scoped_lock_, __LINE__) = &m; \
  UNUSED_PARAM(CONCAT(scoped_lock_, __LINE__))
}  // namespace

// NOTE: EasyToRead(human readable library)
namespace etr {
inline std::string FileSize(size_t v, bool spaceFlag = true) {
  int index = 0;
  double d = v;
  while (v >= 1024) {
    v /= 1024;
    d = d != v ? d / 1024 : v;
    index++;
  }
  std::stringstream ss;
  if (spaceFlag) {
    ss << "% " << std::setw(8) << std::fixed << std::setprecision(3) << d << "["
       << (" KMGTPE"[index]) << "B]";
  } else {
    ss << "% " << std::setw(8) << std::fixed << std::setprecision(3) << d << "["
       << ("\0KMGTPE"[index]) << "B]";
  }
  ss << std::resetiosflags(std::ios_base::floatfield);
  return ss.str();
}
}  // namespace etr
