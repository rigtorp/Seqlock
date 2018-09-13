/*
Copyright (c) 2018 Erik Rigtorp <erik@rigtorp.se>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#pragma once

#include <atomic>
#include <type_traits>

#ifndef NDEBUG
#define RIGTORP_SEQLOCK_NOINLINE __attribute__((noinline))
#else
#define RIGTORP_SEQLOCK_NOINLINE
#endif

namespace rigtorp {

template <typename T> class Seqlock {
public:
  static_assert(std::is_nothrow_copy_assignable<T>::value,
                "T must satisfy is_nothrow_copy_assignable");
  static_assert(std::is_trivially_copy_assignable<T>::value,
                "T must satisfy is_trivially_copy_assignable");

  Seqlock() : seq_(0) {}

  RIGTORP_SEQLOCK_NOINLINE T load() const noexcept {
    T copy;
    std::size_t seq0, seq1;
    do {
      seq0 = seq_.load(std::memory_order_acquire);
      std::atomic_signal_fence(std::memory_order_acq_rel);
      copy = value_;
      std::atomic_signal_fence(std::memory_order_acq_rel);
      seq1 = seq_.load(std::memory_order_acquire);
    } while (seq0 != seq1 || seq0 & 1);
    return copy;
  }

  RIGTORP_SEQLOCK_NOINLINE void store(const T &desired) noexcept {
    std::size_t seq0 = seq_.load(std::memory_order_relaxed);
    seq_.store(seq0 + 1, std::memory_order_release);
    std::atomic_signal_fence(std::memory_order_acq_rel);
    value_ = desired;
    std::atomic_signal_fence(std::memory_order_acq_rel);
    seq_.store(seq0 + 2, std::memory_order_release);
  }

private:
  static const std::size_t kFalseSharingRange = 128;

  // Align to prevent false sharing with adjecent data
  alignas(kFalseSharingRange) T value_;
  std::atomic<std::size_t> seq_;
  // Padding to prevent false sharing with adjecent data
  char padding_[kFalseSharingRange -
                ((sizeof(value_) + sizeof(seq_)) % kFalseSharingRange)];
  static_assert(
      ((sizeof(value_) + sizeof(seq_) + sizeof(padding_)) %
       kFalseSharingRange) == 0,
      "sizeof(Seqlock<T>) should be a multiple of kFalseSharingRange");
};
}
