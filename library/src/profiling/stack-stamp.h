
#include "../base.h"

#ifndef _SAT_StackStamp_h_
#define _SAT_StackStamp_h_

struct RootStackMarker : SAT::IStackMarker {
  virtual void getSymbol(char* buffer, int size) override {
    sprintf_s(buffer, size, "<root>");
  }
};

template <class tData>
class SATStackNodeManifold {
public:

  typedef struct tNode : SAT::StackNode<tData> {
    SpinLock lock;
  } *Node;

  tNode root;

  SATStackNodeManifold(uintptr_t firstSegmentBuffer = 0) {

    // Init allocator
    if (firstSegmentBuffer) {
      firstSegmentBuffer = alignX(8, firstSegmentBuffer);
      this->buffer_base = (char*)firstSegmentBuffer;
      this->buffer_size = SAT::cSegmentSize - (firstSegmentBuffer & SAT::cSegmentOffsetMask);
      SAT::tpProfilingSegmentEntry(&g_SATable[firstSegmentBuffer >> SAT::cSegmentSizeL2])->set(uintptr_t(this));
    }
    else {
      this->buffer_base = 0;
      this->buffer_size = 0;
    }

    // Init root
    memset(&this->root, 0, sizeof(this->root));
    this->root.marker.as<RootStackMarker>();
    this->root.lock.SpinLock::SpinLock();
  }

  Node getStackNode(SAT::Thread* thread) {
    struct StackStampBuilder : SAT::IStackStampBuilder {
      SATStackNodeManifold<tData>* manifold;
      Node tos;
      StackStampBuilder(SATStackNodeManifold<tData>* manifold) {
        this->manifold = manifold;
        this->tos = &manifold->root;
      }
      virtual void push(SAT::StackMarker& marker) override {
        this->tos = this->manifold->addMarkerInto(marker, this->tos);
      }
      Node flush() {
        return this->tos;
      }
    };
    StackStampBuilder stack_builder(this);
    if (thread->CaptureThreadStackStamp(stack_builder)) {
      return stack_builder.flush();
    }
    else {
      return 0;
    }
  }

  Node addMarkerInto(SAT::StackMarker& marker, Node parent) {
     for (;;) {
        Node lastNode = Node(parent->children);
        for (Node node = lastNode; node; node = Node(lastNode->next)) {
           lastNode = node;
           if (node->marker.compare(marker) == 0) {
              return node;
           }
        }

        if (lastNode) {
           lastNode->lock.lock();
           if (!lastNode->next) {
              Node node = (Node)this->allocBuffer(sizeof(tNode));
              node->tNode::tNode();
              node->next = 0;
              node->children = 0;
              node->parent = parent;
              node->marker.copy(marker);
              lastNode->next = node;
              lastNode->lock.unlock();
              return node;
           }
           else lastNode->lock.unlock();
        }
        else {
           parent->lock.lock();
           if (!parent->children) {
              Node node = (Node)this->allocBuffer(sizeof(tNode));
              node->tNode::tNode();
              node->next = 0;
              node->children = 0;
              node->parent = parent;
              node->marker.copy(marker);
              parent->children = node;
              parent->lock.unlock();
              return node;
           }
           else parent->lock.unlock();
        }
     }
  }

private:

  char* buffer_base;
  int buffer_size;
  SpinLock buffer_lock;

  void* allocBuffer(int size) {
    char* ptr;
    this->buffer_lock.lock();
    if (this->buffer_size < size) {
      uintptr_t index = g_SAT.allocSegmentSpan(1);
      SAT::tpProfilingSegmentEntry(&g_SATable[index])->set(uintptr_t(this));
      ptr = (char*)(index << SAT::cSegmentSizeL2);
      this->buffer_base = ptr + size;
      this->buffer_size = SAT::cSegmentSize - size;
    }
    else {
      ptr = this->buffer_base;
      this->buffer_base = ptr + size;
      this->buffer_size -= size;
    }
    this->buffer_lock.unlock();
    return ptr;
  }

};

struct StackStampDatabase {

  struct tStackStampData { };
  typedef SATStackNodeManifold<tStackStampData> StackStampManifold;
  typedef SAT::StackNode<tStackStampData>* StackStampNode;

  StackStampManifold manifold;

  StackStampDatabase(uintptr_t firstSegmentBuffer = 0)
    : manifold(firstSegmentBuffer)
  {
  }
  void traverseStack(uint64_t stackstamp, SAT::IStackVisitor* visitor) {
    for (auto node = StackStampNode(stackstamp); node; node = node->parent) {
      visitor->visit(node->marker);
    }
  }
};
StackStampDatabase* createStackStampDatabase();

#endif
