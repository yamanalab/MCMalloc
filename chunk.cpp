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

#include "chunk.hpp"

namespace mc {
Chunk::Chunk() {}

Chunk::Chunk(size_t size, int sizeIndex)
    : _size(size), _sizeIndex(sizeIndex), _offset(0), _extraAreaForOffset(0) {
#ifdef SIGNATURE_FLAG
  _signature = SIGNATURE;
#endif
}

// NOTE: restoration from pointer of body part
Chunk *Chunk::NewFromBodyPtr(void *ptr) {
  size_t offset = *(size_t *)((uintptr_t)ptr - sizeof(size_t));
  void *chunkp = (void *)((uintptr_t)ptr - sizeof(Chunk) - offset);
  // NOTE: placement new
  Chunk *chunk = new (chunkp) Chunk();
  chunk->SetAlignment(0);
  chunk->SignatureAssert();
  return chunk;
}

// NOTE: requied buffer >= aligenment + size
void Chunk::SetAlignment(size_t aligenment) {
  // NOTE: basically, 8Balignment
  if (aligenment <= 8) {
    _offset = 0;
    _extraAreaForOffset = 0;
    return;
  }
  // NOTE: in the case _offset < 8, _offset of field and body may be duplicated
  _offset = (uintptr_t)PtrWithoutOffset() % aligenment;
  _extraAreaForOffset = _offset;
  if (_offset == 0) return;
  size_t *ptr = (size_t *)((uintptr_t)Ptr() - sizeof(size_t));
  *ptr = _offset;
}

bool Chunk::Assert(size_t requiredSize) {
  if (DEBUG_BUILD) return true;

  auto &chunk = *this;
  chunk.SignatureAssert();
  eassert(chunk.Ptr() != nullptr, "chunk data ptr is nullptr");
  eassert(chunk.Size() >= requiredSize,
          "chunk data size must be larger than required size: "
          "chunk_size=%d, required_size=%d",
          (int)chunk.Size(), (int)requiredSize);
  return true;
}

bool Chunk::SignatureAssert() {
  if (DEBUG_BUILD) return true;
#ifdef SIGNATURE_FLAG
  bool ret = _signature == SIGNATURE;
  eassert(ret || Dump(), "Chunk signature is not correct:%p",
          (void *)_signature);
  return ret;
#else
  return true;
#endif
}

void Chunk::Free() {
// TODO: originally, munmap
#ifdef SIGNATURE_FLAG
  _signature = 0x12345678;
#endif
}

void *Chunk::PtrWithoutOffset() {
  return (void *)((uintptr_t)(this) + sizeof(Chunk));
}
void *Chunk::Ptr() {
  return (void *)((uintptr_t)(this) + sizeof(Chunk) + _offset);
}
size_t Chunk::Size() { return _size; }
size_t Chunk::SizeIndex() { return _sizeIndex; }

size_t Chunk::UnitSize(size_t size) {
  // NOTE: 16 or 64
  return ALIGN(sizeof(Chunk) + size, 16);
}

};  // namespace mc
