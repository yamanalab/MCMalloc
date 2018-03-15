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

bool isPower2(size_t n);
int roundupLog2(size_t x) __attribute__((__const__));

size_t indexToSize(int index) __attribute__((__const__));
size_t indexToSizeWithHash(int index);
int sizeToIndex(size_t x) __attribute__((__const__));
size_t sizeIndexToN(size_t sizeIndex) __attribute__((__const__));

int sizeToIndexWithHash(size_t size, bool addFlag = false);
int sizeToIndexWithHashImple(size_t size, bool addFlag = false);
