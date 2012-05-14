#define DEBUG_Node_refcount
#define DEBUG_Vector_refcount
#include "../src/runtime/Vector.h"

using std::cerr;
using std::endl;
using namespace hue;

int main() {
  // This test starts with an empty vector, appends 1 000 000 values using
  // single operations, and finally confirms the values by accessing each item.
  
  Vector* v = Vector::Empty;
  
  uint64_t i = 0;
  uint64_t N = 1000000; // 1M includes 2 root overflows
  
  for (; i < N; ++i) {
    Vector* oldV = v;
    v = v->append((void*)(i * 10));
    oldV->release();
  }
  
  //cerr << "count(): " << v->count() << endl;
  assert(v->count() == N);

  for (i = 0; i < N; ++i) {
    uint64_t _ = (uint64_t)v->itemAt(i);
    if (_ != (uint64_t)(i * 10)) {
      cerr << "itemAt returned incorrect value -- got " << _
           << " but expected " << (uint64_t)(i * 10) << endl;
    }
    assert(_ == (uint64_t)(i * 10));
  }
  
  // Release the vector
  ((Vector*)v)->release();
  v = 0;
  
  // Verify that there are no leaks
  #ifdef DEBUG_LIVECOUNT_Node
  //cerr << "livecount of Node: " << DEBUG_LIVECOUNT_Node << endl;
  assert(DEBUG_LIVECOUNT_Node == 0);
  #endif
  
  #ifdef DEBUG_LIVECOUNT_Vector
  //cerr << "livecount of Vector: " << DEBUG_LIVECOUNT_Vector << endl;
  assert(DEBUG_LIVECOUNT_Vector == 0);
  #endif
  
  return 0;
}
