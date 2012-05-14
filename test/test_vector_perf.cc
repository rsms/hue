#include "../src/runtime/Vector.h"
#include <stdlib.h>
#include <time.h>

using std::cerr;
using std::endl;
using namespace hue;

int main(int argc, char **argv) {
  // This test starts with an empty vector, appends N (argv[0] as int) values using
  // single operations, and finally confirms the values by accessing each item.
  
  Vector* v = Vector::Empty;
  
  uint64_t i = 0;
  uint64_t N = (argc > 1) ? atoll(argv[1]) : 1000000;
  
  clock_t start1 = clock();
  
  for (; i < N; ++i) {
    Vector* oldV = v;
    v = v->append((void*)(i * 2));
    oldV->release();
  }
  
  double ms1 = ((double)(clock() - start1)) / CLOCKS_PER_SEC * 1000.0;
  cerr << "Inserting " << N << " values: " << ms1 << " ms (avg " << ((ms1 / N) * 1000000.0) << " ns/insert)" << endl;
  
  assert(v->count() == N);
  
  clock_t start2 = clock();

  for (i = 0; i < N; ++i) {
    uint64_t _ = (uint64_t)v->itemAt(i);
    _ += 1; // avoid stripping
  }
  
  double ms2 = ((double)(clock() - start2)) / CLOCKS_PER_SEC * 1000.0;
  cerr << "Accessing all " << N << " values: " << ms2 << " ms (avg " << ((ms2 / N) * 1000000.0) << " ns/access)" << endl;
  
  // Release the vector
  ((Vector*)v)->release();
  v = 0;
  
  return 0;
}

//
// Numbers from "make test_vector_perf" on a 3.4 GHz Intel Core i7:
//
// clang++ -Wall -std=c++11 -fno-rtti -O3 -DNDEBUG -o test/test_vector_perf.img test/test_vector_perf.cc
// ./test/test_vector_perf.img 100
// Inserting 100 values: 0.028 ms (avg 280 ns/insert)
// Accessing all 100 values: 0.001 ms (avg 10 ns/access)
// ./test/test_vector_perf.img 100000
// Inserting 100000 values: 20.567 ms (avg 205.67 ns/insert)
// Accessing all 100000 values: 0.554 ms (avg 5.54 ns/access)
// ./test/test_vector_perf.img 1000000
// Inserting 1000000 values: 202.387 ms (avg 202.387 ns/insert)
// Accessing all 1000000 values: 5.302 ms (avg 5.302 ns/access)
// ./test/test_vector_perf.img 10000000
// Inserting 10000000 values: 1999.92 ms (avg 199.993 ns/insert)
// Accessing all 10000000 values: 62.378 ms (avg 6.2378 ns/access)
// ./test/test_vector_perf.img 100000000
// Inserting 100000000 values: 20013.7 ms (avg 200.137 ns/insert)
// Accessing all 100000000 values: 705.795 ms (avg 7.05795 ns/access)
//
