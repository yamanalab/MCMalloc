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

#include "debug.hpp"
#include "misc.hpp"

// NOTE: use signature or not
// #define SIGNATURE_FLAG

namespace mc {
class Chunk {
 public:
  Chunk();
  Chunk(size_t size, int sizeIndex);

  // NOTE: restoration from pointer of body part
  static Chunk *NewFromBodyPtr(void *ptr);
  // NOTE: requied buffer >= aligenment + size
  void SetAlignment(size_t aligenment);
  bool Assert(size_t requiredSize = 0);
  bool SignatureAssert();
  void Free();
  void *PtrWithoutOffset();
  void *Ptr();
  size_t Size();
  size_t SizeIndex();
  static size_t UnitSize(size_t size);

 private:
  size_t _size;
  size_t _sizeIndex;

#ifdef SIGNATURE_FLAG
  size_t _signature;
#endif

  size_t _offset;
  size_t _extraAreaForOffset;
};
}  // namespace mc
