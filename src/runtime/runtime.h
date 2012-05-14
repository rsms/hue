// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#ifndef _HUE_RUNTIME_INCLUDED
#define _HUE_RUNTIME_INCLUDED

#include <hue/Text.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

namespace hue {

// Types
typedef double   Float;
typedef int64_t  Int;
// UChar is defined by Text.h
typedef uint8_t  Byte;
typedef bool     Bool;
typedef struct DataS_ { Int length; Byte data[0]; }* DataS;
typedef struct TextS_ { Int length; UChar data[0]; }* TextS;

// Write a value to stdout. Mostly for testing and debugging.
void stdout_write(const Bool v);     // _ZN3hue12stdout_writeEb
void stdout_write(const Float v);    // _ZN3hue12stdout_writeEd
void stdout_write(const Int v);      // _ZN3hue12stdout_writeEx
void stdout_write(const UChar v);    // _ZN3hue12stdout_writeEj
void stdout_write(const Byte v);     // _ZN3hue12stdout_writeEh
void stdout_write(const DataS data); // _ZN3hue12stdout_writeEPNS_6DataS_E
void stdout_write(const TextS data); // _ZN3hue12stdout_writeEPNS_6TextS_E

} // namespace hue
#endif // _HUE_RUNTIME_INCLUDED
