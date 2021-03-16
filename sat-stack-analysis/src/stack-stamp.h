#pragma once
#include <sat-memory-allocator/stack_analysis.h>
#include <sat-threads/spinlock.hpp>
#include <sat-memory/memory.hpp>
#include "./utils/alignment.h"

namespace sat {

   struct tTechEntryID {
      enum _ {
         PROFILING_SEGMENT = memory::tEntryID::__FIRST_TECHNICAL_SEGMENT + 1,
      };
   };

   typedef struct tProfilingSegmentEntry {
      uint8_t id; // = PROFILING_SEGMENT
      uint64_t profilingID; // Number representing the profiling
      void set(uint64_t profilingID = 0) {
         this->id = tTechEntryID::PROFILING_SEGMENT;
         this->profilingID = profilingID;
      }
   };

   template <class tData>
   class StackTree {
   public:

      struct tRootMarker : sat::IStackMarker {
         virtual void getSymbol(char* buffer, int size) override {
            sprintf_s(buffer, size, "<root>");
         }
      };

      typedef struct tNode : sat::StackNode<tData> {
         SpinLock lock;
      } *Node;

      tNode root;

      StackTree(uintptr_t firstSegmentBuffer = 0) {

         // Init allocator
         if (firstSegmentBuffer) {
            firstSegmentBuffer = alignX(8, firstSegmentBuffer);
            this->buffer_base = (char*)firstSegmentBuffer;
            this->buffer_size = sat::memory::cSegmentSize - (firstSegmentBuffer & sat::memory::cSegmentOffsetMask);
            memory::table->get<tProfilingSegmentEntry>(firstSegmentBuffer >> sat::memory::cSegmentSizeL2)->set(uintptr_t(this));
         }
         else {
            this->buffer_base = 0;
            this->buffer_size = 0;
         }

         // Init root
         memset(&this->root, 0, sizeof(this->root));
         this->root.marker.as<tRootMarker>();
         this->root.lock.SpinLock::SpinLock();
      }

      Node addMarkerInto(sat::StackMarker& marker, sat::StackNode<tData>* parent) {
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
               Node(parent)->lock.lock();
               if (!parent->children) {
                  Node node = (Node)this->allocBuffer(sizeof(tNode));
                  node->tNode::tNode();
                  node->next = 0;
                  node->children = 0;
                  node->parent = parent;
                  node->marker.copy(marker);
                  parent->children = node;
                  Node(parent)->lock.unlock();
                  return node;
               }
               else Node(parent)->lock.unlock();
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
            uintptr_t index = memory::table->allocSegmentSpan(1);
            sat::memory::table->get<tProfilingSegmentEntry>(index)->set(uintptr_t(this));
            ptr = (char*)(index << sat::memory::cSegmentSizeL2);
            this->buffer_base = ptr + size;
            this->buffer_size = sat::memory::cSegmentSize - size;
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
      struct tData { };
      StackTree<tData> stacktree;
      static StackStampDatabase* create();
      void traverseStack(uint64_t stackstamp, IStackVisitor* visitor);
   private:
      StackStampDatabase(uintptr_t firstSegmentBuffer = 0);
   };

   StackStampDatabase* getStackStampDatabase();
}
