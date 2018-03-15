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

#include <cerrno>
#include <chrono>
#include <fstream>
#include <functional>
#include <mutex>
#include <new>
#include <thread>
#include <vector>

#include "batch_mmap.hpp"
#include "chunk.hpp"
#include "chunk_linked_array_list.hpp"
#include "debug.hpp"
#include "envar.hpp"
#include "memory_chunk_size.hpp"
#include "misc.hpp"
#include "stack.hpp"
#include "status.hpp"

#define LOCK_PARTITIONS_NUM 1

// #define NoPseudoFreePattern true

namespace mc {
template <unsigned N>
class MCMalloc {
 public:
  MCMalloc() {}
  // NOTE: set _Init() before calling constracter
  void _Init() {
    for (int j = 0; j < (int)N; j++) {
      int threadIndex = j;
      for (int i = 0; i < (int)N_SIZE_INDEX_ELEMENT; i++) {
        int sizeIndex = i;
        _stacks[threadIndex][sizeIndex]._Init();
      }
    }

    for (int i = 0; i < (int)N_SIZE_INDEX_ELEMENT * LOCK_PARTITIONS_NUM; i++) {
      _cts[i]._Init();
      _chunkStackMtx[i] = PTHREAD_MUTEX_INITIALIZER;
    }
  }

  bool FreeChunkMunmap(Chunk *chunk, int threadIndex) {
    // TODO
    return true;
  }
  bool FreeChunk(Chunk *chunk, int threadIndex) {
    eassert(chunk != nullptr, "chunk nullptr error: index = %d", threadIndex);

    size_t size = chunk->Size();
    _callStat[threadIndex].CallFree(size);

    int sizeIndex = chunk->SizeIndex();

#ifdef NoPseudoFreePattern
    // NOTE: at least more than PAGE_SIZE * 2
    if (size >= PAGE_SIZE * 4) {
      void *dataPtr = chunk->Ptr();
      void *pagePtr = (void *)ALIGN((uintptr_t)dataPtr, PAGE_SIZE);
      // NOTE: at least more than PAGE_SIZE * 1
      size_t length = ALIGN(size - PAGE_SIZE * 2, PAGE_SIZE);
      // NOTE: in the case of madivse pattern, no return
      if (length > 0) {
        int ret = madvise(pagePtr, length, MADV_DONTNEED);
        eassert(ret != -1, "madvise(MADV_DONTNEED) result is -1: errno=%d",
                errno);
      }

      // NOTE: munmap pattern
      // int ret = munmap(pagePtr, length);
      // eassert(ret != -1, "munmap(%p, %d) result is -1: errno=%d", pagePtr,
      //         (int)length, errno);
      // return ret != -1;
    }
#endif

    auto &ct = _stacks[threadIndex][sizeIndex].Container();
    size_t length = ct.Length();
    // NOTE: req: # of buffer size >=3 (to guarantee that there is a mid layer or are mid layers)
    if (length >= 2 && length * ChunkArrayContainerN * size >= 64 * (1 << 20)) {
      for (int i = 0; i < (int)(length / 2); i++) {
        auto buf = ct.PopMidBuffer();
        chunkPush(buf, threadIndex, sizeIndex);
      }
    }

    bool ret = _stacks[threadIndex][sizeIndex].Push(chunk);
    eassert(ret, "CANNOT free chunk to local stack:size=%d",
            (int)_stacks[threadIndex][sizeIndex].Size());
    return ret;
  }
  // void FreeChunkBuffer(int sizeIndex, int threadIndex) { return; }
  Chunk *MallocChunkFromLocal(int sizeIndex, int threadIndex) {
    Chunk *chunk = _stacks[threadIndex][sizeIndex].Pop();
    if (UNLIKELY(chunk == nullptr)) return nullptr;
    return chunk;
  }
  Chunk *MallocChunkFromOthers(int sizeIndex, int threadIndex) {
    auto buf = chunkPop(threadIndex, sizeIndex);
    if (buf != nullptr) {
      auto &ct = _stacks[threadIndex][sizeIndex].Container();
      ct.PushMidBuffer(buf);
      return MallocChunkFromLocal(sizeIndex, threadIndex);
    }
    return nullptr;
  }
  Chunk *MallocChunkMmap(int sizeIndex, int threadIndex, size_t _size) {
    // NOTE: _size means actual required size
    // NOTE: size means minimam powers of 2 number more than _size
    size_t size;
    sizeIndex = sizeToIndexWithHash(_size, true);
    size = indexToSizeWithHash(sizeIndex);

    size_t unitSize = Chunk::UnitSize(size);
    size_t n = sizeIndexToN(sizeToIndex(size));

    size_t mmapSize = ALIGN(unitSize * n, PAGE_SIZE);
    void *ptr = batchMmapWrapper(mmapSize);
    // NOTE: stack
    for (size_t i = 0; i < n; i++) {
      void *chunkp = (void *)((uintptr_t)ptr + unitSize * (n - 1 - i));
      Chunk *chunk = new (chunkp) Chunk(size, sizeIndex);
      chunk->SignatureAssert();
      _stacks[threadIndex][sizeIndex].Push(chunk);
    }

    Chunk *chunk = MallocChunk(size, threadIndex);
    if (UNLIKELY(chunk == nullptr)) return nullptr;
    return chunk;
  }
  Chunk *MallocChunk(size_t size, int threadIndex) {
    int sizeIndex = sizeToIndexWithHash(size);
    _callStat[threadIndex].CallMalloc(size);

    // NOTE: 1.local stack access
    // NOTE: 2.other queues use
    // NOTE: 3.mmap
    Chunk *chunk = nullptr;
    if (LIKELY(
            (chunk = MallocChunkFromLocal(sizeIndex, threadIndex)) != nullptr ||
            (chunk = MallocChunkFromOthers(sizeIndex, threadIndex)) !=
                nullptr ||
            (chunk = MallocChunkMmap(sizeIndex, threadIndex, size)) != nullptr))
      return chunk;

    eassert(false, "CANNOT MALLOC_CHUNK (ALLOCATE MEMORY)");
    return nullptr;
  }
  bool Free(void *ptr, int threadIndex) {
    Chunk *chunk = Chunk::NewFromBodyPtr(ptr);
    FreeChunk(chunk, threadIndex);
    return true;
  }

  void *Malloc(size_t size, int threadIndex) {
    Chunk *chunk = MallocChunk(size, threadIndex);
    return chunk->Ptr();
  }

  void *Realloc(void *ptr, size_t size, int threadIndex) {
    if (UNLIKELY(ptr == nullptr)) return Malloc(size, threadIndex);
    if (UNLIKELY(size == 0)) {
      Free(ptr, threadIndex);
      return nullptr;
    }

    Chunk *chunk = Chunk::NewFromBodyPtr(ptr);
    size_t preSize = chunk->Size();
    if (UNLIKELY(size == preSize)) return ptr;
    // NOTE: shrink
    if (UNLIKELY(size < preSize)) return ptr;

    void *newPtr = Malloc(size, threadIndex);
    // NOTE: memcpy uses system call or not?
    memcpy(newPtr, ptr, preSize);
    chunk->SignatureAssert();

    bool ret = Free(ptr, threadIndex);
    if (LIKELY(ret)) return newPtr;
    eassert(false, "CANNOT REALLOC (ALLOCATE MEMORY)");
    return nullptr;
  }

  int PosixMemalign(void **memptr, size_t alignment, size_t size,
                    int threadIndex) {
    Chunk *chunk = MallocChunk(size + alignment, threadIndex);
    chunk->SetAlignment(alignment);
    *memptr = chunk->Ptr();
    // TODO: write the code for error case
    return 0;
  }

  void LogThreadStart(std::string outputDirPath) {
    _logThFlag = false;
    _logTh = std::thread([&, outputDirPath]() {
      std::string callStatFilePath = outputDirPath + "/call-stat.txt";
      std::string callStatTotalFilePath =
          outputDirPath + "/call-stat-total.txt";
      std::ofstream callStatLogger(callStatFilePath);
      std::ofstream callStatTotalLogger(callStatTotalFilePath);
      eassert(!!callStatLogger, "file open error");
      eassert(!!callStatTotalLogger, "file open error");

      auto logFunc = [&]() {
        const int maxMemoryIndex = 30;
        static std::vector<int64_t> nMallocFreeSubMaxVec(maxMemoryIndex);
        static int64_t nMallocFreeSubTotalMax = 0;

        int threadMaxN = (int)N;
        std::stringstream ssCall;
        std::stringstream ssTotalCall;
        std::stringstream ssMalloc;
        std::stringstream ssFree;
        std::stringstream ssMallocFreeSub;

        int64_t nMallocTotal = 0;
        int64_t nFreeTotal = 0;
        for (int i = 0; i <= maxMemoryIndex; i++) {
          int sizeIndex = i;
          int64_t nMalloc = 0;
          int64_t nFree = 0;
          for (int i = 0; i < threadMaxN; i++) {
            auto &&callStat = _callStat[i];
            nMalloc += callStat.NMallocAt(sizeIndex);
            nFree += callStat.NFreeAt(sizeIndex);
          }
          nMallocTotal += nMalloc;
          nFreeTotal += nFree;
          nMallocFreeSubMaxVec[i] =
              std::max(nMallocFreeSubMaxVec[i], nMalloc - nFree);
          ssCall << i << " " << nMalloc << " " << nFree << " "
                 << nMalloc - nFree << " " << nMallocFreeSubMaxVec[i]
                 << std::endl;
          ssMalloc << nMalloc << " ";
          ssFree << nFree << " ";
          ssMallocFreeSub << nMalloc - nFree << " ";
        }
        {
          int64_t sum = 0;
          for (auto &&v : nMallocFreeSubMaxVec) sum += v;
          nMallocFreeSubTotalMax = std::max(nMallocFreeSubTotalMax, sum);
        }
        // NOTE: statistics of each size
        callStatLogger << "# time=" << elapsedTime << std::endl
                       << ssCall.str() << std::endl
                       << std::endl;
        // NOTE: total statistics without size relation
        callStatTotalLogger << elapsedTime << " " << nMallocTotal << " "
                            << nFreeTotal << " " << nMallocTotal - nFreeTotal
                            << " " << nMallocFreeSubTotalMax << std::endl;

        myprintf("[Log]:time=%12d\n", (int)elapsedTime);
        myprintf("      malloc     :%s\n", ssMalloc.str().c_str());
        myprintf("      free       :%s\n", ssFree.str().c_str());
        myprintf("      malloc-free:%s\n", ssMallocFreeSub.str().c_str());
      };

      elapsedTime = 0;
      const size_t minInterval = 250000;
      const size_t initInterval = 250000;
      const int maxInterval = 250000;
      const double intevalGrowRatio = 1.0;

      int sleepInterval = initInterval;
      bool __logThFlag = false;

      std::chrono::system_clock::time_point start, end;
      start = std::chrono::system_clock::now();
      while (true) {
        end = std::chrono::system_clock::now();
        elapsedTime =
            (size_t)std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                          start)
                .count();
        logFunc();
        if (__logThFlag) break;
        for (int v = sleepInterval; v > 0; v -= 1E6) {
          std::this_thread::sleep_for(std::chrono::microseconds(v));
          {
            std::lock_guard<std::mutex> lock(_logThMtx);
            if (_logThFlag) {
              __logThFlag = true;
              continue;
            }
          }
        }

        {
          sleepInterval =
              (int)((double)sleepInterval * intevalGrowRatio + minInterval) /
              minInterval * minInterval;
          sleepInterval = std::min(sleepInterval, maxInterval);
        }
      }
      // TODO: error handling
    });
  }
  void LogThreadJoin() {
    {
      std::lock_guard<std::mutex> lock(_logThMtx);
      _logThFlag = true;
    }
    if (_logTh.joinable()) _logTh.join();
  }

  void Init() {
    if (!CALL_STATISTIC_FLAG) return;

    // NOTE:these functions are called other app. via env. var. ===> static
    static std::function<void(std::string)> doFunc = [&](std::string str) {
      LogThreadStart(str);
    };
    static std::function<void(void)> joinFunc = [&]() { LogThreadJoin(); };
    eassert(envar::SetPtr("MCMalloc_LOG_THREAD_DO_FUNC_PTR", &doFunc),
            "doFunc setenv error: errno=%d", errno);
    eassert(envar::SetPtr("MCMalloc_LOG_THREAD_JOIN_FUNC_PTR", &joinFunc),
            "joinFunc setenv error: errno=%d", errno);
  }

  bool chunkPush(ChunkArrayContainer *ptr, int threadIndex, int sizeIndex) {
    // NOTE: LOCK_PARTITIONS_NUM==1 version
    // SCOPED_LOCK(_chunkStackMtx[sizeIndex]);
    // auto &ct = _cts[sizeIndex];
    // ct.PushMidBuffer(ptr);

    int index =
        sizeIndex * LOCK_PARTITIONS_NUM + threadIndex % LOCK_PARTITIONS_NUM;
    SCOPED_LOCK(_chunkStackMtx[index]);
    auto &ct = _cts[index];
    ct.PushMidBuffer(ptr);
    return true;
  }
  ChunkArrayContainer *chunkPop(int threadIndex, int sizeIndex) {
    // NOTE: LOCK_PARTITIONS_NUM==1 version
    // SCOPED_LOCK(_chunkStackMtx[sizeIndex]);
    // auto &ct = _cts[sizeIndex];
    // return ct.PopMidBuffer();

    for (int i = 0; i < LOCK_PARTITIONS_NUM; i++) {
      int index = sizeIndex * LOCK_PARTITIONS_NUM + i;
      SCOPED_LOCK(_chunkStackMtx[index]);
      auto &ct = _cts[index];
      auto ptr = ct.PopMidBuffer();
      if (ptr != nullptr) return ptr;
    }
    return nullptr;
  }

 private:
  // global stack
  // NOTE: sizeof(std::mutex)==64
  // NOTE: sizeof(pthread_mutex_t)==64
  pthread_mutex_t _chunkStackMtx[N_SIZE_INDEX_ELEMENT * LOCK_PARTITIONS_NUM];
  ChunkLinkedArrayListStack _cts[N_SIZE_INDEX_ELEMENT * LOCK_PARTITIONS_NUM];
  Stack<Chunk *, ChunkLinkedArrayListStack> _stacks[N][N_SIZE_INDEX_ELEMENT];

  std::thread _logTh;
  std::mutex _logThMtx;
  bool _logThFlag;
  size_t elapsedTime;
  CallStat _callStat[N];
  BufferStatus _bufferStatuses[N][N_SIZE_INDEX_ELEMENT];
  Status _statuses[N];
};
}  // namespace mc
