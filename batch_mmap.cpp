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

#include "batch_mmap.hpp"

// #define NoBatchMallocPattern true

void batchMmapTerm() { batchMmapWrapper((size_t)~0); }

void* batchMmapWrapper(size_t length) {
  const int devZero = -1;
  return batchMmap(nullptr, length, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, devZero, 0);
}

void* batchMmap(void* addr, size_t length, int prot, int flags, int fd,
                off_t offset) {
  thread_local void* head = nullptr;
  thread_local size_t size = 0;

  if (length == (size_t)~0) {
    if (head != nullptr && size >= PAGE_SIZE) {
      // NOTE: basically, size is a multiple of PAGE_SIZE
      size_t munmapSize = ALIGN(size, PAGE_SIZE);
      int ret = munmap(head, munmapSize);
      eassert(ret != -1, "munmap result is -1: errno=%d", errno);
      head = nullptr;
      size = 0;
    }
    return nullptr;
  }
  if (length == 0) {
    errno = EINVAL;
    return nullptr;
  }

  void* p;
  // NOTE: If there is pool, mmap arg options are ignored.
  if (length <= size) {
    // NOTE: return from mmap pool
    p = head;
    head = (void*)((uintptr_t)head + length);
    size -= length;
    return p;
  }

// NOTE: extend
#ifdef NoBatchMallocPattern
  const size_t batchMinLength = 1;
  const size_t batchMaxLength = 1;
  const size_t batchMulNum = 1;
#else
  const size_t batchMinLength = 2 * 1024 * 1024;
  const size_t batchMaxLength = 64 * 1024 * 1024;
  const size_t batchMulNum = 16;
#endif
  // NOTE: overflow is unsupported(maybe no problem)
  // NOTE: range(min length ~ max length ~ length)
  size_t batchLength = length * batchMulNum;
  batchLength = std::max(batchLength, batchMinLength);
  batchLength = std::min(batchLength, batchMaxLength);
  batchLength = std::max(batchLength, length);

  p = mmap(addr, batchLength, prot, flags, fd, offset);
  eassert((p != (void*)-1), "mmap result is -1: errno=%d", errno);
  eassert(ALIGN_CHECK(p, PAGE_SIZE),
          "mmap ptr must be a multiple of the page size: addr=%p", p);

  if (head != nullptr && size >= PAGE_SIZE) {
    // NOTE: basically, size is a multiple of PAGE_SIZE
    size_t munmapSize = ALIGN(size, PAGE_SIZE);
    int ret = munmap(head, munmapSize);
    eassert(ret != -1, "munmap result is -1: errno=%d", errno);
  }

  head = p;
  size = batchLength;

  head = (void*)((uintptr_t)head + length);
  size -= length;
  return p;
}
