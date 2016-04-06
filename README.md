# Seqlock.h

[![Build Status](https://travis-ci.org/rigtorp/Seqlock.svg?branch=master)](https://travis-ci.org/rigtorp/Seqlock)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/rigtorp/Seqlock/master/LICENSE)

Implementation of [Seqlock](https://en.wikipedia.org/wiki/Seqlock) in
C++11.

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

## About

This project was created by [Erik Rigtorp](http://rigtorp.se)
<[erik@rigtorp.se](mailto:erik@rigtorp.se)>.
