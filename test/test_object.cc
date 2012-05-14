// clang++ -std=c++11 -Wall -g -o co_object_test experiments/co_object_test.cc  && ./co_object_test
// 
// #include <gperftools/tcmalloc.h>
// #include <gperftools/heap-profiler.h>
// #include <gperftools/profiler.h>

#include <hue/runtime/object.h>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include <iostream>
#include <stdexcept>
#include <bitset>

using std::cerr;
using std::endl;
using namespace hue;

static size_t live_toy_count = 0;
static size_t live_cat_count = 0;

class Toy { HUE_OBJECT(Toy)
public:
  int bar;
  bool baz;
  
  static Toy* create(int bar, bool baz) {
    Toy* obj = __alloc();
    obj->bar = bar;
    obj->baz = baz;
    __sync_add_and_fetch(&live_toy_count, 1);
    return obj;
  }
  
  void dealloc() {
    __sync_sub_and_fetch(&live_toy_count, 1);
  }
  
  inline Ref refcount() const { return __sync_fetch_and_add(&refcount_, 0); }
};

class Cat { HUE_OBJECT(Cat)
public:
  int age;
  char* name;
  Toy* toy;
  
  static Cat* create(int age, const char* name, Toy* toy) {
    Cat* obj = __alloc();
    obj->age = age;
    obj->name = name ? strdup(name) : 0;
    obj->toy = toy ? toy->retain() : 0;
    __sync_add_and_fetch(&live_cat_count, 1);
    return obj;
  }
  
  void dealloc() {
    if (name) free(name);
    if (toy) toy->release();
    __sync_sub_and_fetch(&live_cat_count, 1);
  }
  
  inline Ref refcount() const { return __sync_fetch_and_add(&refcount_, 0); }
};


int main() {
  //HeapProfilerStart("main");
  //ProfilerStart("main.prof");
  
  uint64_t i = 0;
  uint64_t N = 10000;
  
  for (; i < N; ++i) {
    Toy* toy = Toy::create(i, i % 2 == 0 );
    
    assert(live_cat_count == 0);
    assert(live_toy_count == 1);
    assert(toy->refcount() == 1);
    
    Cat* cat = Cat::create(4, "Zelda", toy);
    
    assert(live_cat_count == 1);
    assert(live_toy_count == 1);
    assert(cat->refcount() == 1);
    assert(cat->toy == toy);
    assert(cat->toy->refcount() == 2);
    
    toy->release();
    
    assert(live_toy_count == 1);
    assert(cat->toy->refcount() == 1);
    
    cat->release();
    
    assert(live_toy_count == 0);
    assert(live_cat_count == 0);
  }
  
  //ProfilerStop();
  //HeapProfilerStop();
    
  return 0;
}
