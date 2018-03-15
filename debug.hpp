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

#include <pthread.h>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <stdexcept>

#include "mac.hpp"
#include "myprintf.hpp"

#define STATISTIC_FLAG false
#define CALL_STATISTIC_FLAG true
#define DEBUG_BUILD false

#define sizeHashMaxSize (32)

#define TWO_SIZE_FLAG

#ifdef TWO_SIZE_FLAG
#define N_SIZE_INDEX_ELEMENT_2_POW (64 + 1)
#else
#define N_SIZE_INDEX_ELEMENT_2_POW (128 + 1)
#endif
#define N_SIZE_INDEX_ELEMENT (N_SIZE_INDEX_ELEMENT_2_POW + sizeHashMaxSize)

#define UNUSED_PARAM(x) ((void)(x))

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

inline void ZERO_ASSIGNE() {
#pragma GCC diagnostic push

#ifdef __APPLE__
#pragma GCC diagnostic ignored "-Wunused-value"
#else
#pragma GCC diagnostic ignored "-Wunused-value"
#endif

  int* p;
  p = 0;
  *p = 0;
#pragma GCC diagnostic pop
}

#define CONST_STRING_CONCAT(str, ...) str #__VA_ARGS__

#define eassert(flag, fmt, ...)                               \
  {                                                           \
    bool ____ret = (flag);                                    \
    if (!____ret) {                                           \
      myprintf("%20s:%4d:%20s # " fmt "", __FILE__, __LINE__, \
               __PRETTY_FUNCTION__, ##__VA_ARGS__);           \
      myprintf("\n");                                         \
      ZERO_ASSIGNE();                                         \
      exit(1);                                                \
    }                                                         \
  }
