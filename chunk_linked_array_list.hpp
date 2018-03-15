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

#include "batch_mmap.hpp"
#include "chunk.hpp"
#include "misc.hpp"

namespace mc {
// NOTE: # of allocate pages at once
// 1: too few
// 2,4: good
// 8~: too many

// #define ElementLinkedListPattern true
#ifdef ElementLinkedListPattern
#define ChunkArrayContainerPageN (1)
#define ChunkArrayContainerN (1)
#else
#define ChunkArrayContainerPageN (2)
#define ChunkArrayContainerN               \
  ((PAGE_SIZE * ChunkArrayContainerPageN - \
    sizeof(ChunkArrayContainer *) * 2) /   \
   sizeof(ChunkArrayContainer *))
#endif

class ChunkArrayContainer {
 public:
  ChunkArrayContainer() {}
  inline Chunk *&At(int index) { return Buffer()[index]; }
  inline ChunkArrayContainer *&Pre() { return _prePtr; }
  inline ChunkArrayContainer *&Next() { return _nextPtr; }
  inline static const int MaxSize() { return ChunkArrayContainerN; }
  inline Chunk **Buffer() {
    return (Chunk **)((uintptr_t)(this) + sizeof(ChunkArrayContainer *));
  }

  static ChunkArrayContainer *New() {
    size_t mmapSize = ALIGN(
        sizeof(ChunkArrayContainer) * ChunkArrayContainerPageN, PAGE_SIZE);
    void *ptr = batchMmapWrapper(mmapSize);
    eassert((ptr != (void *)-1), "mmap result is -1: errno=%d", errno);
    eassert(ALIGN_CHECK(ptr, PAGE_SIZE),
            "mmap ptr must be a multiple of the page size: addr=%p", ptr);
    // TODO: unmmap when term app
    ChunkArrayContainer *cptr = (ChunkArrayContainer *)ptr;
    cptr->_prePtr = nullptr;
    cptr->_nextPtr = nullptr;
    return cptr;
  }
  // NOTE: if no next one, extend
  inline ChunkArrayContainer *ForceNext() {
    if (Next() == nullptr) {
      auto tmp = ChunkArrayContainer::New();
      tmp->Pre() = this;
      Next() = tmp;
    }
    return Next();
  }

 private:
  ChunkArrayContainer *_prePtr;
#ifdef __APPLE__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-private-field"
#endif
  Chunk *_buffer[ChunkArrayContainerN];
#ifdef __APPLE__
#pragma GCC diagnostic pop
#endif
  ChunkArrayContainer *_nextPtr;
};
#ifndef ElementLinkedListPattern
static_assert(sizeof(ChunkArrayContainer) ==
                  PAGE_SIZE * ChunkArrayContainerPageN,
              "ChunkArrayContainer size is not correct");
#endif

class ChunkLinkedArrayListStack {
 public:
  // NOTE: There is a possibility that malloc(3) is used before main(). So, we must call this explicitly.
  void _Init() {
    _size = 0;
    _length = 1;
    _topArrayPtr = ChunkArrayContainer::New();
    _topArrayIndex = -1;
  }
  bool IsEmpty() { return _size == 0; };
  bool IsFull() { return false; }
  size_t Size() { return _size; }
  // NOTE: maximum value for convenience (It seems not necessary to set)
  size_t MaxSize() { return std::numeric_limits<size_t>::max(); }
  size_t Length() { return _length; }
  size_t ArrayMaxSize() { return ChunkArrayContainer::MaxSize(); }

  ChunkArrayContainer *PopMidBuffer() {
    if (_length <= 1) return nullptr;
    _size -= ArrayMaxSize();
    _length--;
    // NOTE: from:(bottom) ptr__ <-> ptr_ <-> ptr (top)
    // NOTE: to:(bottom) ptr__ <-> ptr (top)
    ChunkArrayContainer *ptr = _topArrayPtr;
    ChunkArrayContainer *ptr_ = ptr->Pre();
    ChunkArrayContainer *ptr__ = ptr_->Pre();
    ptr->Pre() = ptr__;
    if (ptr__ != nullptr) ptr__->Next() = ptr;
    ptr_->Pre() = ptr->Next() = nullptr;
    return ptr_;
  }
  void PushMidBuffer(ChunkArrayContainer *ptr_) {
    if (ptr_ == nullptr) return;
    _size += ArrayMaxSize();
    _length++;
    // NOTE: from:(bottom) ptr__ <-> ptr (top)
    // NOTE: to:(bottom) ptr__ <-> ptr_ <-> ptr (top)
    ChunkArrayContainer *ptr = _topArrayPtr;
    ChunkArrayContainer *ptr__ = ptr->Pre();
    ptr->Pre() = ptr_;
    ptr_->Next() = ptr;
    ptr_->Pre() = ptr__;
    if (ptr__ != nullptr) ptr__->Next() = ptr_;
  }

  // NOTE:requires: chunk is not nullptr
  bool PushTop(Chunk *chunk) {
    if (UNLIKELY(IsFull())) return false;

    _topArrayIndex++;
    if (UNLIKELY(_topArrayIndex == (int)ArrayMaxSize())) {
      _topArrayPtr = _topArrayPtr->ForceNext();
      _topArrayIndex = 0;
      _length++;
    }
    _topArrayPtr->At(_topArrayIndex) = chunk;
    _size++;
    return true;
  }
  Chunk *PopTop() {
    if (UNLIKELY(IsEmpty())) return nullptr;

    if (UNLIKELY(_topArrayIndex == -1)) {
      // NOTE:no need to check nullptr(always non-nullptr)
      _topArrayPtr = _topArrayPtr->Pre();
      _topArrayIndex = ArrayMaxSize() - 1;
      _length--;
    }
    const int stride = 4;
    // NOTE: to avoid if statement
    __builtin_prefetch(
        (const void *)(_topArrayPtr->At((_topArrayIndex - stride) >= 0
                                            ? (_topArrayIndex - stride)
                                            : _topArrayIndex)),
        1, 0);

    auto &chunk = _topArrayPtr->At(_topArrayIndex);
    _topArrayIndex--;
    _size--;
    return chunk;
  }

 private:
  // NOTE:to avoid cache false sharing, this class size have to be multiples of 64B
  size_t _size;    // # of element
  size_t _length;  // # of array(Buffered ones are not included.)
  ChunkArrayContainer *_topArrayPtr;
  int _topArrayIndex;
#ifdef __APPLE__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-private-field"
#endif
  uint8_t _dummy[64 - (8 + 8 + 8 + 4)];
#ifdef __APPLE__
#pragma GCC diagnostic pop
#endif
};
}  // namespace mc
