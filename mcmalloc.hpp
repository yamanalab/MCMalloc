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
#include <string>  // for memset

#include "batch_mmap.hpp"
#include "debug.hpp"
#include "init_term.hpp"
#include "mcmalloc_impl.hpp"
#include "thread_util.hpp"

#ifdef __APPLE__
extern "C" {
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nmemb, size_t size);
int posix_memalign(void **memptr, size_t alignment, size_t size) throw();
void free(void *p);
}
#else
void *malloc(size_t size);
void *realloc(void *ptr, size_t size) throw();
void *calloc(size_t nmemb, size_t size);
int posix_memalign(void **memptr, size_t alignment, size_t size) throw();
void free(void *p);
#endif
