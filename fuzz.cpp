#include "Seqlock.h"
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

struct Data {
  std::size_t a, b, c;
};

int main(int argc, char *argv[]) {
  Seqlock<Data> sl;
  std::atomic<std::size_t> ready(0);
  std::vector<std::thread> ts;

  for (int i = 0; i < 100; ++i) {
    ts.push_back(std::thread([&sl, &ready]() {
      while (ready == 0) {
      }
      for (std::size_t i = 0; i < 10000000; ++i) {
        Data copy = sl.load();
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

  return 0;
}
