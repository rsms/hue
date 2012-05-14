#include "co_object.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <bitset>

using std::cerr;
using std::endl;


#define DEBUG_Node_refcount
#define DEBUG_Vector_refcount


#ifdef DEBUG_Node_refcount
static size_t live_node_count = 0;
#define DEBUG_LIVECOUNT_Node live_node_count
#define DEBUG_LIVECOUNT_Node_INC CO_DEBUG_COUNT_INC(DEBUG_LIVECOUNT_Node);
#define DEBUG_LIVECOUNT_Node_DEC CO_DEBUG_COUNT_DEC(DEBUG_LIVECOUNT_Node);
#else
#define DEBUG_LIVECOUNT_Node_INC
#define DEBUG_LIVECOUNT_Node_DEC
#endif

#ifdef DEBUG_Vector_refcount
static size_t live_vector_count = 0;
#define DEBUG_LIVECOUNT_Vector live_vector_count
#define DEBUG_LIVECOUNT_Vector_INC CO_DEBUG_COUNT_INC(DEBUG_LIVECOUNT_Vector);
#define DEBUG_LIVECOUNT_Vector_DEC CO_DEBUG_COUNT_DEC(DEBUG_LIVECOUNT_Vector);
#else
#define DEBUG_LIVECOUNT_Vector_INC
#define DEBUG_LIVECOUNT_Vector_DEC
#endif


// Rudimentary fixed-size copyable array
class Node { CO_OBJECT(Node)
public:
  static Node Empty;
  typedef void* V;
  
  uint8_t length; // <= 32 = 100000 (only 6-bits are used)
  uint8_t unused;
  std::bitset<32> objectBitset; // each bit is a flag which if set means that index is an object
  // Note: Object superclass is 32-bit wide, meaning we align on 64-bit boundaries.
  
  // Note: We could use bit-fields of 6 and 58 bits here, so we align
  // at 64-bit boundaries. But since refcount is operated on as a native
  // integer using atomic operations, it can not be a bit-field unless we
  // operate using regular arithmetic.
  //uint8_t length : 6; // <= 32 = 100000
  //uint64_t refcount_ : 58;
  
  // An empty node is 8 bytes. A node with two elements is 24 bytes, and so on.
  
  // Must be the last member
  V data[0];
  
  
  static Node* create(uint8_t length) {
    Node* node = alloc(length);
    node->length = length;
    node->objectBitset = 0;
    return node;
  }
  
  static Node* create(const V value) {
    Node* node = alloc(1);
    ((V*)&node->data)[0] = value;
    node->length = 1;
    node->objectBitset = 0;
    return node;
  }
  
  static Node* create(const Node& other, V tailValue) {
    Node* node = alloc(other.length+1);
    __copy(node, &other);
    
    // Increase refcount of any shallow-copied objects
    if (node->objectBitset.any()) for (size_t i = 0; i < node->objectBitset.size(); ++i) {
      if (node->objectBitset[i]) node->getNode(i)->retain();
    }
    
    ((V*)&node->data)[other.length] = tailValue;
    ++node->length;
    return node;
  }
  
  static Node* create(const Node& other, uint8_t length) {
    assert(length >= other.length);
    Node* node = alloc(length);
    __copy(node, &other);
    
    // Increase refcount of any shallow-copied objects
    if (node->objectBitset.any()) for (size_t i = 0; i < node->objectBitset.size(); ++i) {
      if (node->objectBitset[i]) node->getNode(i)->retain();
    }
    
    node->length = length;
    return node;
  }
  
  inline void setValue(uint8_t i, V value) {
    assert(objectBitset[i] == false); // Or we need a retain-release dance
    ((V*)&data)[i] = value;
  }
  
  inline void setNode(uint8_t i, Node* node, CORefRule refrule = CORetainReference) {
    Node* oldNode = objectBitset[i] ? getNode(i) : 0;
    if (refrule == CORetainReference) node->retain();
    ((Node**)&data)[i] = node;
    objectBitset[i] = true;
    if (oldNode) oldNode->release();
  }
  
  inline V getValue(uint8_t i) const {
    assert(objectBitset[i] == false);
    return ((V*)&data)[i];
  }
  
  inline Node* getNode(uint8_t i) const {
    assert(objectBitset[i] == true);
    return ((Node**)&data)[i];
  }
  
  std::string repr() const {
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

private:
  Node() : refcount_(COUnretainable), length(0), objectBitset(0) {}
  
  inline static Node* alloc(uint8_t length) {
    DEBUG_LIVECOUNT_Node_INC
    return __alloc(sizeof(Node) + (sizeof(V) * length));
  }
  
  static Node* __copy(Node* dest, Node const* source) {
    // This function performs a memcpy but leaves the reference count unchanged.
    // Requirement: refcount_ (usually from CO_OBJECT) must be the first member of the
    // struct/class.
    //
    // TODO: If we can figure out how to communicate a class's intended size, we could
    // bundle this function into CO_OBJECT.
    //
    return (Node*)memcpy(
      ((uint8_t*)dest) + sizeof(CORef), // start after refcount_ member
      ((uint8_t*)source) + sizeof(CORef),      // start after refcount_ member
      (sizeof(Node)-sizeof(CORef)) + (sizeof(void*) * source->length) // size - refcount_ member
    );
  }
  
  void dealloc() {
    DEBUG_LIVECOUNT_Node_DEC
    // Release any refs we own
    if (objectBitset.any()) for (size_t i = 0; i < objectBitset.size(); ++i) {
      if (objectBitset[i]) getNode(i)->release();
    }
  }
};

Node Node::Empty;


// A persistent immutable vector
class Vector { CO_OBJECT(Vector)
private:
  size_t count_; // number of items in this vector
  uint32_t shift_;
  Node* root_;
  Node* tail_;

public:
  static const Vector Empty;
  
  const size_t count() const { return count_; }
  
  // Returns a vector with val added to the end
  Vector const* append(void* val) const {
    //room in tail?
    if (tailLength() < 32) {
      Node* newTail = (tail_ == 0) ? Node::create(val) : Node::create(*tail_, val);
      Vector* v = Vector::create(count_ + 1, shift_, root_,CORetainReference, newTail,COTransferReference);
      return v;
    }
    
    // Full tail -- push into tree
    Node* newroot;
    uint32_t newshift = shift_;
    
    // Overflow root?
    if ((count_ >> 5) > (1 << shift_)) {
      newroot = Node::create(2);
      newroot->setNode(0, root_);
      newroot->setNode(1, newPath(shift_, tail_), COTransferReference);
      newshift += 5;
    } else {
      newroot = pushTail(shift_, root_, tail_);
    }
    
    Node* newTail = Node::create(val);
    Vector* v = Vector::create(count_ + 1, newshift, newroot,COTransferReference, newTail,COTransferReference);
    return v;
  }
  
  // Retrieve item at index i
  inline void* itemAt(size_t i) const throw(std::out_of_range) {
    return nodeFor(i).getValue(i & 0x1f);
  }
  
protected:

  // Used for the empty vector ::Empty
  Vector() : refcount_(COUnretainable), count_(0), shift_(5), root_(&Node::Empty), tail_(0) {}
  
  static Vector* create(size_t count, uint32_t shift,
                        Node* root, CORefRule root_refrule,
                        Node* tail, CORefRule tail_refrule ) {
    DEBUG_LIVECOUNT_Vector_INC
    Vector* v = __alloc(sizeof(Vector));
    v->count_ = count;
    v->shift_ = shift;
    v->root_ = (root_refrule == COTransferReference) ? root : root->retain();
    v->tail_ = (tail_refrule == COTransferReference) ? tail : tail->retain();
    assert(v->shift_ % 5 == 0);
    return v;
  }
  
  void dealloc() {
    DEBUG_LIVECOUNT_Vector_DEC
    if (root_) root_->release();
    if (tail_) tail_->release();
  }

  inline size_t tailLength() const {
    return count_ - tailoff();
  }

  // Offset of tail (the start of tail relative to count)
  inline size_t tailoff() const {
    if (tail_ == 0 || count_ < 32)
      return 0;
    return ((count_ - 1) >> 5) << 5;
  }
  
  // Finds the node for index i
  const Node& nodeFor(size_t i) const throw(std::out_of_range) {
    if (i >= count_)
      throw std::out_of_range("index out of range");

    // i is in tail?
    if (i >= tailoff())
      return *tail_;

    Node* node = root_;
    for (uint32_t level = shift_; level > 0; level -= 5) {
      node = node->getNode( (i >> level) & 0x1f );
    }
    
    assert(node != 0);
    return *node;
  }
  
  // Create a new tail. Returns a node with a +1 refcount.
  Node* pushTail(size_t level, Node* parent, Node* tailnode) const {
    //if parent is leaf, insert node,
    // else does it map to an existing child? -> nodeToInsert = pushNode one more level
    // else alloc new path
    //return  nodeToInsert placed in copy of parent
    size_t subidx = ((count_ - 1) >> level) & 0x1f;
    Node* nodeToInsert;
    
    if (level == 5) {
      nodeToInsert = tailnode->retain();
      // note: We retain here since we later transfer ownership, gaining perf
      // in the common case (the else branch) where no refs change.
    } else {
      if (parent == 0 || subidx >= parent->length) {
        nodeToInsert = newPath(level-5, tailnode);
      } else {
        Node* child = parent->getNode(subidx);
        nodeToInsert = pushTail(level-5, child, tailnode);
      }
    }
    
    Node* node;
    if (parent != 0 && parent->length) {
      if (parent->length <= subidx) {
        node = Node::create(*parent, subidx+1);
      } else {
        node = parent->retain();
      }
    } else {
      node = Node::create(subidx+1);
    }
    
    assert(node->length > subidx);
    
    node->setNode(subidx, nodeToInsert, COTransferReference);
    assert(node->getNode(subidx) == nodeToInsert);
    
    return node;
  }
  
  // Create a new path. Returns a node with a +1 refcount.
  Node* newPath(uint32_t level, Node* node) const {
    if (level == 0) {
      node->retain();
      return node;
    }
    // Create a new node with a single value, which is the parent node
    Node* newnode = Node::create(1);
    newnode->setNode(0, newPath(level - 5, node), COTransferReference);
    
    return newnode;
  }

};

const Vector Vector::Empty;


int main() {
  Vector const* v = &Vector::Empty;
  
  uint64_t i = 0;
  uint64_t N = 1000000;
  
  for (; i < N; ++i) {
    Vector const* oldV = v;
    v = v->append((void*)(i * 10));
    ((Vector*)oldV)->release();
  }
  
  cerr << "count(): " << v->count() << endl;

  for (i = 0; i < N; ++i) {
    volatile uint64_t _ = (uint64_t)v->itemAt(i);
    if (_ != (uint64_t)(i * 10))
      cerr << "itemAt returned incorrect value -- got " << _ << " but expected " << (uint64_t)(i * 10) << endl;
    assert(_ == (uint64_t)(i * 10));
  }
  
  // Release the vector
  ((Vector*)v)->release();
  v = 0;
  
  // Verify that there are no leaks
  #ifdef DEBUG_LIVECOUNT_Node
  cerr << "livecount of Node: " << DEBUG_LIVECOUNT_Node << endl;
  assert(DEBUG_LIVECOUNT_Node == 0);
  #endif
  
  #ifdef DEBUG_LIVECOUNT_Vector
  cerr << "livecount of Vector: " << DEBUG_LIVECOUNT_Vector << endl;
  assert(DEBUG_LIVECOUNT_Vector == 0);
  #endif
  
  return 0;
}
