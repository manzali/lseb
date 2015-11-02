#include <chrono>
#include <numeric>
#include <iostream>
#include <cstdlib>

// compiler options: c++ -std=c++11 -O3 -DNDEBUG memory_bw.cpp

/*
 * numactl --cpunodebind=0 --membind=0 ./a.out
 * numactl --cpunodebind=0 --membind=1 ./a.out
 */

size_t const B = 1024 * 1024 * 1024;
size_t const Gb = 8;

int main() {
  // allocation
  char* m = new char[B];
  auto t1 = std::chrono::high_resolution_clock::now();
  // writing
  std::fill_n(m, B, 1);
  auto t2 = std::chrono::high_resolution_clock::now();
  // reading
  auto s = std::accumulate(m, m + B, 0);
  auto t3 = std::chrono::high_resolution_clock::now();
  std::cout
    << s
    << " bytes allocated\n"
    << "write: "
    << Gb / std::chrono::duration<double>(t2 - t1).count()
    << " Gb/s\n"
    << "read: "
    << Gb / std::chrono::duration<double>(t3 - t2).count()
    << " Gb/s\n";
  return EXIT_SUCCESS;
}
