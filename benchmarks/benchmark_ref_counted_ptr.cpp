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