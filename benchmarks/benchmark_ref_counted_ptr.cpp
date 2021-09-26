// Copyright 2021 Francois Chabot

// Licensed under the Apache License, Version 2.0 (the "License")
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <benchmark/benchmark.h>

#include <cstdlib>
#include <ctime>

#include <memory>
#include "abu/mem.h"

namespace {
static void BM_shared_ptr_int_lifetime(benchmark::State& state) {
  std::srand(std::time(nullptr));
  int v = std::rand();

  for (auto _ : state) {
    (void)_;
    auto tmp = std::make_shared<int>(v);
    benchmark::DoNotOptimize(tmp);
  }
}
BENCHMARK(BM_shared_ptr_int_lifetime);

static void BM_shared_ptr_int_access(benchmark::State& state) {
  std::srand(std::time(nullptr));
  int v = std::rand();

  for (auto _ : state) {
    (void)_;
    auto tmp = std::make_shared<int>(v);
    int u = *tmp;
    benchmark::DoNotOptimize(u);
    benchmark::DoNotOptimize(tmp);
  }
}
BENCHMARK(BM_shared_ptr_int_access);

static void BM_ref_counted_int_lifetime(benchmark::State& state) {
  std::srand(std::time(nullptr));
  int v = std::rand();

  for (auto _ : state) {
    (void)_;
    auto tmp = abu::mem::make_ref_counted<int>(v);
    benchmark::DoNotOptimize(tmp);
  }
}
BENCHMARK(BM_ref_counted_int_lifetime);

static void BM_ref_counted_int_access(benchmark::State& state) {
  std::srand(std::time(nullptr));
  int v = std::rand();

  for (auto _ : state) {
    (void)_;
    auto tmp = abu::mem::make_ref_counted<int>(v);
    int u = *tmp;
    benchmark::DoNotOptimize(u);
    benchmark::DoNotOptimize(tmp);
  }
}
BENCHMARK(BM_ref_counted_int_access);

static void BM_shared_ptr_obj_lifetime(benchmark::State& state) {
  struct ObjType {};
  for (auto _ : state) {
    auto tmp = std::make_shared<ObjType>();
    benchmark::DoNotOptimize(tmp);
  }
}
BENCHMARK(BM_shared_ptr_obj_lifetime);

static void BM_ref_counted_obj_lifetime(benchmark::State& state) {
  struct ObjType {};
  for (auto _ : state) {
    auto tmp = abu::mem::make_ref_counted<ObjType>();
    benchmark::DoNotOptimize(tmp);
  }
}
BENCHMARK(BM_ref_counted_obj_lifetime);

static void BM_ref_counted_intrusive_obj_lifetime(benchmark::State& state) {
  struct ObjType : public abu::mem::ref_counted {};

  for (auto _ : state) {
    auto tmp = abu::mem::make_ref_counted<ObjType>();
    benchmark::DoNotOptimize(tmp);
  }
}
BENCHMARK(BM_ref_counted_intrusive_obj_lifetime);
}  // namespace

BENCHMARK_MAIN();