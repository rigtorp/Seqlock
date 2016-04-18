# Seqlock.h

[![Build Status](https://travis-ci.org/rigtorp/Seqlock.svg?branch=master)](https://travis-ci.org/rigtorp/Seqlock)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/rigtorp/Seqlock/master/LICENSE)

Implementation of [seqlock](https://en.wikipedia.org/wiki/Seqlock) in
C++11.

A seqlock can be used as an alternative to
[a readers-writer lock](https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock). It
will never block the writer and doesn't require any memory bus locks.

## Example

```cpp
struct Data {
  std::size_t a, b, c;
};
Seqlock<Data> sl;
sl.store({0, 0, 0});
auto t = std::thread([&] {
  for (;;) {
    auto d = sl.load();
    if (d.a + 100 == d.b && d.c == d.a + d.b) {
      return;
    }
  }
});
sl.store({100, 200, 300});
t.join();
```

## Usage

Create a seqlock:

```cpp
struct Data {
  std::size_t a, b, c;
};
Seqlock<Data> sl;
```

Store a value (can only be called from a single thread):

```cpp
sl.store({1, 2, 3});
```

Load a value (can be called from multiple threads):

```cpp
auto v = sl.load();
```

## Implementation

Implementing the seqlock in portable C++11 is quite tricky. The basic
seqlock implementation *unconditionally loads* the sequence number,
then *unconditionally loads* the protected data and finally
*unconditionally loads* the sequence number again. Since loading the
protected data is done unconditionally on the sequence number the
compiler is free to move these loads before or after the loads from
the sequence number.

```cpp
T load() const noexcept {
  T copy;
  size_t seq0, seq1;
  do {
    seq0 = seq_.load(std::memory_order_acquire);
    copy = value_;
    seq1 = seq_.load(std::memory_order_acquire);
  } while (seq0 != seq1 || seq0 & 1);
  return copy;
}
```

Compiling this code specialized for `int` with `g++-5.2 -std=c++11 -O3` yields
the following assembly:

```
0000000000401ad0 <_ZNK7rigtorp7SeqlockIiLm64EE4loadEv>:
  // copy = value_;
  401ad0:	8b 47 08             	mov    0x8(%rdi),%eax
  401ad3:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  // do {
  //   seq0 = seq_.load(std::memory_order_acquire);
  401ad8:	48 8b 0f             	mov    (%rdi),%rcx
  //   seq1 = seq_.load(std::memory_order_acquire);
  401adb:	48 8b 17             	mov    (%rdi),%rdx
  // } while (seq0 != seq1 || seq0 & 1);
  401ade:	48 39 ca             	cmp    %rcx,%rdx
  401ae1:	75 f5                	jne    401ad8 <_ZNK7rigtorp7SeqlockIiLm64EE4loadEv+0x8>
  401ae3:	83 e2 01             	and    $0x1,%edx
  401ae6:	75 f0                	jne    401ad8 <_ZNK7rigtorp7SeqlockIiLm64EE4loadEv+0x8>
  // return copy;
  401ae8:	f3 c3                	repz retq 
  401aea:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
```

We see that the compiler did indeed reorder the load of the protected
data outside the critical section and the data is no longer protected
from torn reads. Interestingly compiling using `clang++-3.7 -std=c++11
-O3` produces the correct assembly:

```
0000000000401520 <_ZNK7rigtorp7SeqlockIiLm64EE4loadEv>:
  // do {
  //   seq0 = seq_.load(std::memory_order_acquire);  
  401520:	48 8b 0f             	mov    (%rdi),%rcx
  //   copy = value_;
  401523:	8b 47 08             	mov    0x8(%rdi),%eax
  //   seq1 = seq_.load(std::memory_order_acquire);
  401526:	48 8b 17             	mov    (%rdi),%rdx
  // } while (seq0 != seq1 || seq0 & 1);
  401529:	f6 c1 01             	test   $0x1,%cl
  40152c:	75 f2                	jne    401520 <_ZNK7rigtorp7SeqlockIiLm64EE4loadEv>
  40152e:	48 39 d1             	cmp    %rdx,%rcx
  401531:	75 ed                	jne    401520 <_ZNK7rigtorp7SeqlockIiLm64EE4loadEv>
  // return copy;
  401533:	c3                   	retq   
  401534:	66 2e 0f 1f 84 00 00 	nopw   %cs:0x0(%rax,%rax,1)
  40153b:	00 00 00 
  40153e:	66 90                	xchg   %ax,%ax
```

There are two ways to fix this:

* Make the location of the protected data dependent on the sequence
  number by storing multiple instances of the data and selecting which
  one to read from based on the sequence number. This solution should
  be portable to all CPU architectures, but requires extra space.
* For x86 it's enough to insert a compiler barrier using
  `std::atomic_signal_fence(std::memory_order_acq_rel)`. This will
  only work on the [x86 memory model][x86-mm]. On
  [ARM memory model][arm-mm] you need to inserts a `dmb` memory
  barrier instruction, which is not possible in C++11.
  
Since my target architecture is x86 I've implemented the second
option:

```cpp
T load() const noexcept {
  T copy;
  size_t seq0, seq1;
  do {
    seq0 = seq_.load(std::memory_order_acquire);
    std::atomic_signal_fence(std::memory_order_acq_rel);
    copy = value_;
    std::atomic_signal_fence(std::memory_order_acq_rel);
    seq1 = seq_.load(std::memory_order_acquire);
  } while (seq0 != seq1 || seq0 & 1);
  return copy;
}
```

Compiled with `g++-5.2 -std=c++11 -O3` it produces the
following correct assembly:

```
00000000004014e0 <_ZNK7rigtorp7SeqlockIiE4loadEv>:
  4014e0:	48 8d 4f 08          	lea    0x8(%rdi),%rcx
  4014e4:	0f 1f 40 00          	nopl   0x0(%rax)
  // do {
  //   seq0 = seq_.load(std::memory_order_acquire);
  //   std::atomic_signal_fence(std::memory_order_acq_rel);
  4014e8:	48 8b 31             	mov    (%rcx),%rsi
  //   copy = value_;
  4014eb:	8b 07                	mov    (%rdi),%eax
  //   std::atomic_signal_fence(std::memory_order_acq_rel);
  //   seq1 = seq_.load(std::memory_order_acquire);
  4014ed:	48 8b 11             	mov    (%rcx),%rdx
  // } while (seq0 != seq1 || seq0 & 1);
  4014f0:	48 39 f2             	cmp    %rsi,%rdx
  4014f3:	75 f3                	jne    4014e8 <_ZNK7rigtorp7SeqlockIiE4loadEv+0x8>
  4014f5:	83 e2 01             	and    $0x1,%edx
  4014f8:	75 ee                	jne    4014e8 <_ZNK7rigtorp7SeqlockIiE4loadEv+0x8>
  // return copy;
  4014fa:	f3 c3                	repz retq 
  4014fc:	0f 1f 40 00          	nopl   0x0(%rax)
```

The store operation is implemented in a similar manner to the load
operation. Additionally the data and sequence counter is aligned and
padded to prevent false sharing with adjacent data.

References:

* [fast reader/writer lock for gettimeofday 2.5.30](http://lwn.net/Articles/7388/)
* [Can Seqlocks Get Along With Programming Language Memory Models?](http://www.hpl.hp.com/techreports/2012/HPL-2012-68.pdf)
  ([slides](http://safari.ece.cmu.edu/MSPC2012/slides_posters/boehm-slides.pdf))
* [x86-TSO: A Rigorous and Usable Programmer’s Model for x86 Multiprocessors][x86-mm]
* [A Tutorial Introduction to the ARM and POWER Relaxed Memory Models][arm-mm]

[x86-mm]: http://www.cl.cam.ac.uk/~pes20/weakmemory/cacm.pdf
[arm-mm]: http://www.cl.cam.ac.uk/~pes20/ppc-supplemental/test7.pdf

## Testing

Testing lock-free algorithms is hard. I'm using two approaches to test
the implementation:

* A test that `load()` and `store()` publish results in the correct
  order on a single thread.
* A multithreaded fuzz test that `load()` never see a partial
  `store()` (torn read).

## Potential improvements

Allow partial updates and reads using a visitor pattern.

Support for multiple writers can be achieved by using a CAS loop to
acquire the odd sequence number in the store operation. This would
have the same effect as wrapping the seqlock in a spinlock.

Trade-off space for reduced readers-writer contention by striping
writes across multiple seqlocks.

## About

This project was created by [Erik Rigtorp](http://rigtorp.se)
<[erik@rigtorp.se](mailto:erik@rigtorp.se)>.
