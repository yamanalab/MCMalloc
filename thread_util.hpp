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
#include <thread>

#include "debug.hpp"
#include "misc.hpp"
#include "myprintf.hpp"

// TODO: It is not essential but we should make below defination variable.
#define N_THREAD_MAX 256

namespace {
int64_t get_thread_id() {
  static_assert(sizeof(std::thread::id) == sizeof(int64_t),
                "this function only works if size of thead::id is equal to the "
                "size of uint64_t");
  auto id = std::this_thread::get_id();
  int64_t *ptr = (int64_t *)&id;
  return (*ptr);
}
}  // namespace

namespace threadutil {
// NOTE: # of Main Thread is 0
// NOTE: assumption: one thread call only once

const int64_t MAIN_THREAD_ID = get_thread_id();
pthread_mutex_t _mtx = PTHREAD_MUTEX_INITIALIZER;
int64_t threadFlags[N_THREAD_MAX] = {};

inline int NewCurrentThreadIndex() {
  int64_t id = get_thread_id();
  if (id == MAIN_THREAD_ID) return 0;

  {
    SCOPED_LOCK(_mtx);
    // NOTE: zero means new one mode
    int startIndex = 0;
    for (int i = 0; i < N_THREAD_MAX - 1; i++) {
      int index = (startIndex + i) % N_THREAD_MAX + 1;
      // NOTE: current thread is not 0
      if (threadFlags[index] <= 0) {
        threadFlags[index] = id;
        return index;
      }
    }
  }
  return -1;
}
inline bool DeleteCurrentThreadIndex() {
  int64_t id = get_thread_id();
  if (id == MAIN_THREAD_ID) return true;
  {
    SCOPED_LOCK(_mtx);
    for (int i = 1; i < N_THREAD_MAX; i++) {
      if (threadFlags[i] == id) {
        threadFlags[i] = -1;
        return true;
      }
    }
  }
  return false;
}
}  // namespace threadutil
