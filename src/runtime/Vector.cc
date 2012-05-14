// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#include "Vector.h"

namespace hue {

const Vector Vector::_Empty;
Vector* Vector::Empty = (Vector*)&Vector::_Empty;

const Vector::Node Vector::Node::_Empty;
Vector::Node* Vector::Node::Empty = (Vector::Node*)&Vector::Node::_Empty;


// ------------------------------------------------------
// Vector::Node

std::string Vector::Node::repr() const {
  std::ostringstream ss;
  ss << "<Node@" << this << " [";
  for (uint8_t i=0; i < length; ++i) {
    if (objectBitset[i] == true) {
      ss << getNode(i)->repr();
    } else {
      ss << ((size_t*)&data)[i];
    }
    if (i < length-1) ss << ", ";
  }
  ss << "]>";
  return ss.str();
}

} // namespace hue
