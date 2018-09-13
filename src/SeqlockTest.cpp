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

#include <atomic>
#include <iostream>
#include <rigtorp/Seqlock.h>
#include <thread>
#include <vector>

int main(int, char *[]) {
  using namespace rigtorp;

  // Basic test
  {
    Seqlock<int> sl;
    sl.store(1);
    if (sl.load() != 1) {
      std::terminate();
    }
    sl.store(2);
    if (sl.load() != 2) {
      std::terminate();
    }
  }

  // Fuzz test
  {
    struct Data {
      std::size_t a, b, c;
    };

    Seqlock<Data> sl;
    std::atomic<std::size_t> ready(0);
    std::vector<std::thread> ts;

    for (int i = 0; i < 100; ++i) {
      ts.push_back(std::thread([&sl, &ready]() {
        while (ready == 0) {
        }
        for (std::size_t i = 0; i < 10000000; ++i) {
          auto copy = sl.load();
          if (copy.a + 100 != copy.b || copy.c != copy.a + copy.b) {
            std::terminate();
          }
        }
        ready--;
      }));
    }

    std::size_t counter = 0;
    while (true) {
      Data data = {counter++, data.a + 100, data.b + data.a};
      sl.store(data);
      if (counter == 1) {
        ready += ts.size();
      }
      if (ready == 0) {
        break;
      }
    }
    std::cout << counter << std::endl;

    for (auto &t : ts) {
      t.join();
    }
  }

  return 0;
}
