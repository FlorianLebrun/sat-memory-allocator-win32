#pragma once

#include "../base.h"

namespace SAT {

   struct Thread;

   struct ThreadStackContext;

   struct NativeStackMarker : IStackMarker {
      uintptr_t base;
      uintptr_t callsite;
      virtual void getSymbol(char* buffer, int size) override {
         SystemThreading::tSymbol symbol(buffer, size);
         if (!symbol.resolve(this->base)) {
            sprintf_s(buffer, size, "0x%.8lX", uint64_t(this->base));
         }
      }
   };

   struct StackFrame {

      enum tKind {
         NoFrame = 0,
         RootFrame,
         FunctionFrame,
         BeaconSpanFrame,
         BeaconTagFrame,
      };

      // Basic
      StackFrame* next;
      uintptr_t InsAddress;
      uintptr_t StackAddress;
      uint64_t BeaconId;

      // Specific
      tKind Kind;
      union {
         uintptr_t FunctionAddress;
         SAT::StackBeacon* BeaconAddress;
      } Data;

      // For call tree matching
      void* node;

      StackFrame* setRoot();
      std::string toString();
      void print();
   };

   struct ThreadStackTracker {
      static const int reserve_batchSize = 512;

      SAT::Thread* thread;
      StackFrame* lastStack;

      ThreadStackTracker(SAT::Thread* thread);
      bool updateFrames(ThreadStackContext& context, SAT::StackBeacon** beacons);
      bool captureFrames(std::function<void(StackFrame* topOfStack)> processing);
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

};
