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

#include "mcmalloc.hpp"

const bool mcmallocDebugFlag = false;

// NOTE: 1:main thread + 1:HElib thread pool control thread + 8:HElib thread pool threds + 72:parallel processing threads
const int nThread = 1 + 1 + 8 + 72;
mc::MCMalloc<nThread> mcmalloc;

bool initFlag = false;

thread_local struct ThreadLocalData {
  inline int &Index() { return index_; }
  inline bool &MainFlag() { return mainFlag_; }
  inline bool &InitFlag() { return initFlag_; }

  int index_;
  bool mainFlag_;
  bool initFlag_;
} threadLocalData = {0, false, false};

#define _threadInit()                            \
  if (UNLIKELY(!threadLocalData.InitFlag())) {   \
    threadLocalData.InitFlag() = true;           \
    _init();                                     \
    if (threadLocalData.MainFlag() && !initFlag) \
      initFlag = true, mcmalloc.Init();          \
  }

void mainInit() {
  threadLocalData.MainFlag() = true;
  mcmalloc._Init();
}
void mainTerm() {}

void threadInit() {
  if (!threadLocalData.MainFlag())
    threadLocalData.Index() = threadutil::NewCurrentThreadIndex();
  eassert(threadLocalData.Index() != -1 && threadLocalData.Index() < nThread,
          "required: threadIndex != -1 && threadIndex(%d) < nThread(%d)",
          threadLocalData.Index(), nThread);
}
void threadTerm() {
  if (!threadLocalData.MainFlag()) {
    bool ret = threadutil::DeleteCurrentThreadIndex();
    eassert(ret, "[mcmalloc threadutil::DeleteCurrentThreadIndex failed]");
    // TODO: error processing
    ret = true;
    // NOTE: unmap mmap buffer
    batchMmapTerm();
  }
}

#ifdef __APPLE__
extern "C" {
#endif

void *malloc(size_t size) {
  if (UNLIKELY(size == 0)) return nullptr;

  const int threadIndex = threadLocalData.Index();
  if (UNLIKELY(!threadLocalData.InitFlag())) {
    threadLocalData.InitFlag() = true;
    _init();
    if (threadLocalData.MainFlag() && !initFlag)
      initFlag = true, mcmalloc.Init();
    return malloc(size);
  }

  void *ptr = mcmalloc.Malloc(size, threadIndex);
  if (mcmallocDebugFlag)
    myprintf("#====malloc: size=%8d, ptr=%p\n", (int)size, ptr);
  if (UNLIKELY(ptr == nullptr)) errno = ENOMEM;
  return ptr;
}

void free(void *ptr) {
  if (mcmallocDebugFlag) myprintf("#====free: ptr=%p\n", ptr);
  if (UNLIKELY(ptr == nullptr)) return;

  _threadInit();
  bool ret = mcmalloc.Free(ptr, threadLocalData.Index());
  eassert(ret, "[mcmallocmaloc free failed]");
  return;
}

void *calloc(size_t nmemb, size_t size) {
  if (UNLIKELY(nmemb == 0 || size == 0)) return nullptr;
  _threadInit();

  const size_t total_size = nmemb * size;
  void *ptr = malloc(total_size);
  if (ptr != nullptr) memset(ptr, 0, total_size);
  if (mcmallocDebugFlag)
    myprintf("#====calloc: nmemb=%d, size=%8d, ptr=%p\n", (int)nmemb, (int)size,
             ptr);
  return ptr;
}

#ifdef __APPLE__
void *realloc(void *ptr, size_t size) {
#else
void *realloc(void *ptr, size_t size) throw() {
#endif
  if (UNLIKELY(ptr == nullptr)) return malloc(size);
  _threadInit();

  void *newPtr = mcmalloc.Realloc(ptr, size, threadLocalData.Index());
  if (mcmallocDebugFlag)
    myprintf("#====realloc: ptr=%p, size=%8d, newPtr=%p\n", ptr, (int)size,
             newPtr);
  return newPtr;
}

int posix_memalign(void **memptr, size_t alignment, size_t size) throw() {
  if (UNLIKELY(size == 0)) {
    *memptr = nullptr;
    return 0;
  }
  _threadInit();

  if (mcmallocDebugFlag)
    myprintf("#====posix_memalign: memptr=%p, alignment=%8d, size=%d\n",
             (void *)memptr, (int)alignment, (int)size);

  int ret =
      mcmalloc.PosixMemalign(memptr, alignment, size, threadLocalData.Index());
  // FYI:
  // // 0:success
  // // 12:ENOMEM          12      /* Out of memory */
  // // 22:EINVAL          22      /* Invalid argument */
  return ret;
}

// TODO: memalign
// TODO: valloc

#ifdef __APPLE__
}
#endif
