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
#include <cstdint>

namespace mc {
class Status {
 public:
  Status()
      : _currentAppUsedMemoyUsage(0),
        _currentUsedMemoryUsage(0),
        _currentBufferMemoryUsage(0) {}

  int64_t &CurrentAppUsedMemoryUsage() { return _currentAppUsedMemoyUsage; }
  int64_t &CurrentUsedMemoryUsage() { return _currentUsedMemoryUsage; }
  int64_t &CurrentBufferMemoryUsage() { return _currentBufferMemoryUsage; }

 private:
  int64_t
      _currentAppUsedMemoyUsage;  // raw memory usage which is not rounded up
  int64_t _currentUsedMemoryUsage;
  int64_t _currentBufferMemoryUsage;
};

class BufferStatus {
 public:
  BufferStatus() : _nBufferChunk(0), _nUsedChunk(0) {}

  int64_t &NBufferChunk() { return _nBufferChunk; }
  int64_t &NUsedChunk() { return _nUsedChunk; }

 private:
  int64_t _nBufferChunk;
  int64_t _nUsedChunk;
};

class CallStat {
 public:
  CallStat() {
    if (!CALL_STATISTIC_FLAG) return;
    for (int i = 0; i < 64; i++) {
      _nMalloc[i] = 0;
      _nFree[i] = 0;
    }
  }

  void CallMalloc(size_t size) {
    if (!CALL_STATISTIC_FLAG) return;
    _nMalloc[roundupLog2(size)]++;
  }
  void CallFree(size_t size) {
    if (!CALL_STATISTIC_FLAG) return;
    _nFree[roundupLog2(size)]++;
  }
  int64_t &NMallocAt(int i) { return _nMalloc[i]; }
  int64_t &NFreeAt(int i) { return _nFree[i]; }

 private:
  int64_t _nMalloc[64];
  int64_t _nFree[64];
};
}  // namespace mc
