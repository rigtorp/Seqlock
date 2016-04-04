# Seqlock.h

[![Build Status](https://travis-ci.org/rigtorp/Seqlock.svg?branch=master)](https://travis-ci.org/rigtorp/Seqlock)

Implementation of [Seqlock](https://en.wikipedia.org/wiki/Seqlock) in
C++.

## Example

```
struct Data {
  std::size_t a, b, c;
};

Seqlock<Data> sl;
sl.store({1, 1, 1}); // write
auto copy = sl.load(); // read
```
