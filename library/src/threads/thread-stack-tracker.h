#pragma once

#include "../base.h"

struct StackFrame {
   StackFrame* next;
   uintptr_t BaseAddress;
   uintptr_t InsAddress;
   uintptr_t StackAddress;

   StackFrame* enclose() {
      StackFrame* overflow = this->next;
      this->StackAddress = 0;
      this->BaseAddress = 0;
      this->InsAddress = 0;
      this->next = 0;
      return overflow;
   }
   void print();
};

struct ThreadStackTracker {
   static const int reserve_batchSize = 512;

   StackFrame* lastStack;

   ThreadStackTracker() {
      this->_reserve = 0;
      this->lastStack = this->allocFrame();
      this->lastStack->enclose();
   }
   StackFrame* updateFrames(void* pcontext);
private:
   StackFrame* _reserve;
   StackFrame* allocFrame() {
      StackFrame* frame = this->allocFrameBatch();
      this->_reserve = frame->next;
      return frame;
   }
   StackFrame* allocFrameBatch() {
      StackFrame* frames = this->_reserve;
      if (!frames) {
         frames = new StackFrame[reserve_batchSize];
         for (int i = 0; i < reserve_batchSize; i++) {
            frames[i].next = &frames[i + 1];
         }
         frames[reserve_batchSize - 1].next = this->_reserve;
      }
      this->_reserve = 0;
      return frames;
   }
};
