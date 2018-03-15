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

#include "memory_chunk_size.hpp"

// TODO create class
const size_t sizeCntTh = 10;
const size_t sizeHashSamplingRate = 8;
const size_t sizeDataMaxSize = 8 * 256 * 1024 * 2;
int sizeHashIndexPos = 1;
pthread_mutex_t sizeHashMtx;
// NOTE: no overflow(if 64B)
size_t sizeMapCount[sizeHashMaxSize] = {};      // fill zero
size_t sizeMapIndices[sizeHashMaxSize] = {};    // fill zero
size_t sizeIndexMapSize[sizeHashMaxSize] = {};  // fill zero
std::hash<std::string> sizeHashFunc;
size_t sizeMapCount2[sizeDataMaxSize] = {};    // fill zero
size_t sizeMapIndices2[sizeDataMaxSize] = {};  // fill zero

bool isPower2(size_t n) { return !(n & (n - 1)); }
int roundupLog2(size_t x) {
  int64_t res;
  __asm__("bsrq %1, %0" : "=r"(res) : "r"(x - (x > 1)));
  return res + 1;
}

#ifdef TWO_SIZE_FLAG
size_t indexToSize(int index) { return 1UL << index; }
int sizeToIndex(size_t x) {
  // NOTE:0~8 => 8
  if (x <= 8) return 3;
  return roundupLog2(x);
}
#else
size_t indexToSize(int index) {
  const auto v = 1UL << (index / 2);
  return v + v / 2 * (index % 2);
}
int sizeToIndex(size_t x) {
  auto _index = roundupLog2(x);
  const auto v = 1UL << _index;
  return x <= (v - v / 4) ? _index * 2 - 1 : _index * 2;
}
#endif

size_t indexToSizeWithHash(int index) {
  if (index < N_SIZE_INDEX_ELEMENT_2_POW) return indexToSize(index);
  size_t size = sizeIndexMapSize[index - N_SIZE_INDEX_ELEMENT_2_POW];
  if (size == 0) return indexToSize(index);
  return size;
}

size_t sizeIndexToN(size_t sizeIndex) {
  thread_local static size_t sizeIndexToNumberMap[N_SIZE_INDEX_ELEMENT] = {};
  auto& n = sizeIndexToNumberMap[sizeIndex];
  n <<= 1;
  const size_t MAX_SIZE = 1 * (1 << 20);
  while ((n * indexToSize(sizeIndex)) > MAX_SIZE) n >>= 1;
  const size_t MIN_NUM = 1;
  n = std::max(n, MIN_NUM);
  if (indexToSize(sizeIndex) <= (1 << 18) && n < 32) n = 32;
  return n;
}

int sizeToIndexWithHash(size_t size, bool addFlag) {
  int ret = sizeToIndexWithHashImple(size, addFlag);
  if (ret == 0) ret = sizeToIndex(size);
  return ret;
}
int sizeToIndexWithHashImple(size_t size, bool addFlag) {
  if (size <= 8 || isPower2(size)) return 0;

  size_t originSize = size;
  size = (size + sizeHashSamplingRate - 1) / sizeHashSamplingRate *
         sizeHashSamplingRate;

  int sizeHash = size;
  if (sizeHash >= (int)sizeDataMaxSize) return 0;

  size_t cnt = sizeMapCount2[sizeHash];
  if (cnt < sizeCntTh) {
    if (addFlag) {
      sizeMapCount2[sizeHash]++;
    }
    return 0;
  }

  int sizeHashIndex = sizeMapIndices2[sizeHash];
  if (sizeHashIndex >= sizeHashMaxSize) return 0;

  if (sizeHashIndex == 0) {
    if (sizeHashIndexPos >= sizeHashMaxSize) {
      return 0;
    }
    int tmppos = sizeHashIndexPos;
    {
      SCOPED_LOCK(sizeHashMtx);
      if (sizeIndexMapSize[tmppos] == 0) {
        myprintf("origin size = %d, size = %d\n", (int)originSize, (int)size);
        sizeHashIndex = sizeHashIndexPos;
        sizeIndexMapSize[sizeHashIndex] = size;

        size_t nearSize = indexToSize(sizeToIndex(size) - 1);
        for (int i = 0; i < sizeHashIndexPos; i++)
          if (sizeIndexMapSize[i] < size)
            nearSize = std::max(nearSize, sizeIndexMapSize[i]);

        for (size_t index = nearSize + 1; index <= size; index++)
          sizeMapIndices2[index] = sizeHashIndexPos;

        sizeHashIndexPos++;
        myprintf("origin size = %d, size = %d, sizeHashIndex = %d (%d~%d)\n",
                 (int)originSize, (int)size, (int)sizeHashIndex, (int)nearSize,
                 (int)size);
      } else {
        myprintf("origin size = %d, size = %d, sizeHashIndex = %d (false)\n",
                 (int)originSize, (int)size, (int)sizeHashIndex);
      }
    }
  } else {
    // duplicate keys
    if (sizeIndexMapSize[sizeHashIndex] > size) return 0;
  }
  return N_SIZE_INDEX_ELEMENT_2_POW + sizeHashIndex;
}
