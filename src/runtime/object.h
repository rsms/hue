// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

#ifndef _HUE_OBJECT_INCLUDED
#define _HUE_OBJECT_INCLUDED

#include <stdint.h>
#include <stdlib.h>

// Memory
#define hue_alloc malloc
#define hue_realloc realloc
#define hue_dealloc free

namespace hue {

// Reference counter
typedef uint64_t Ref;

// Reference counter value that denotes the owner is not retainable.
// Calling retain or release on such an object has no effect.
// This is useful for globals that might be mixed with reference counted
// objects of the same kind.
static const Ref Unretainable = UINT64_MAX;

// Reference ownership rules
typedef enum {
  RetainReference = 0, // ownership is retained (reference count is increased by the receiver)
  TransferReference,   // ownership is transfered ("steals" a reference)
} RefRule;

// Implements the functions and data needed for a class to become reference counted.
// Messy, but it works...
#define HUE_OBJECT(T) \
public: \
  Ref refcount_; \
private: \
  static T* __alloc(size_t size = sizeof(T)) { \
    T* obj = (T*)hue_alloc(size); \
    obj->refcount_ = 1; \
    return obj; \
  } \
public: \
  inline T* retain() { \
    if (refcount_ != hue::Unretainable) __sync_add_and_fetch(&refcount_, 1); \
    return this; \
  } \
  inline void release() { \
    if (refcount_ != hue::Unretainable && __sync_sub_and_fetch(&refcount_, 1) == 0) { \
      dealloc(); \
      hue_dealloc(this); \
    } \
  } \
protected:


//class Object { HUE_OBJECT(Object) void dealloc() {} };

// -------------------------
// Debugging

// Increment or decrement a counter (*counter* must be a non-bit-field 32- or 64-bit integer)
#define HUE_DEBUG_COUNT_INC(counter) __sync_add_and_fetch(&(counter), 1)
#define HUE_DEBUG_COUNT_DEC(counter) __sync_sub_and_fetch(&(counter), 1)

} // namespace hue
#endif // _HUE_OBJECT_INCLUDED
