#include <rigtorp/Seqlock.h>
#include <thread>

int main(int, char *[]) {
  using namespace rigtorp;

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

  return 0;
}
