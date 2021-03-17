#pragma once
#include <stdint.h>
#include <sat/shared.hpp>
#include <sat/thread.hpp>

namespace sat {

   //-----------------------------------------------------------------------------
   //-----------------------------------------------------------------------------
   //
   // Stack Marker: define the sealed shape of a stack beacon
   // 
   //-----------------------------------------------------------------------------
   //-----------------------------------------------------------------------------

   struct IStackMarker {
      virtual void getSymbol(char* buffer, int size) = 0;
   };

   struct StackMarker : IStackMarker {
      uint64_t word64[3];
      template<typename MarkerT>
      MarkerT* as() {
         _ASSERT(sizeof(typename MarkerT) <= sizeof(StackMarker));
         this->word64[0] = 0;
         this->word64[1] = 0;
         this->word64[2] = 0;
         return new(this) typename MarkerT();
      }
      intptr_t compare(StackMarker& other) {
         intptr_t c;
         if (c = other.word64[0] - this->word64[0]) return c;
         if (c = other.word64[-1] - this->word64[-1]) return c;
         if (c = other.word64[1] - this->word64[1]) return c;
         if (c = other.word64[2] - this->word64[2]) return c;
         return 0;
      }
      void copy(StackMarker& other) {
         this->word64[-1] = other.word64[-1];
         this->word64[0] = other.word64[0];
         this->word64[1] = other.word64[1];
         this->word64[2] = other.word64[2];
      }
      virtual void getSymbol(char* buffer, int size) override {
         buffer[0] = 0;
      }
   };
   static_assert(sizeof(StackMarker) == 4 * sizeof(uint64_t), "StackMarker size invalid");

   //-----------------------------------------------------------------------------
   //-----------------------------------------------------------------------------
   //
   // Stack Beacon: define a frame of a metadata stack, allocated on the call stack
   // 
   //-----------------------------------------------------------------------------
   //-----------------------------------------------------------------------------

   struct StackBeacon {
      uint64_t beaconId;
      virtual bool isSpan() { return false; }
      virtual void createrMarker(StackMarker& buffer) = 0;
   };

   template<class StackBeaconT>
   struct StackBeaconHolder {
      bool isDeployed;
      StackBeaconT beacon;
      StackBeaconHolder() {
         this->isDeployed = false;
      }
      ~StackBeaconHolder() {
         this->remove();
      }
      void deploy() {
         if (!this->isDeployed) {
            sat_begin_stack_beacon(&this->beacon);
            this->isDeployed = true;
         }
      }
      void remove() {
         if (this->isDeployed) {
            sat_end_stack_beacon(&this->beacon);
            this->isDeployed = false;
         }
      }
   };

   //-----------------------------------------------------------------------------
   //-----------------------------------------------------------------------------
   //
   // Stack profiling
   // 
   //-----------------------------------------------------------------------------
   //-----------------------------------------------------------------------------

   template <class tData>
   struct StackNode {
      StackNode* next;
      StackNode* parent;
      StackNode* children;
      sat::StackMarker marker;
      tData data;
   };

   struct IStackStampBuilder {
      virtual void push(sat::StackMarker& marker) = 0;
   };

   struct IStackVisitor {
      virtual void visit(StackMarker& marker) = 0;
   };

   struct IStackProfiling : public shared::ISharedObject {

      struct tData {
         int hits;
         tData() : hits(0) {}
      };

      typedef sat::StackNode<tData>* Node;

      virtual Node getRoot() = 0;
      virtual void print() = 0;
   };

   struct IStackProfiler : public shared::ISharedObject {
      virtual IStackProfiling* flushProfiling() = 0;
      virtual void startProfiling() = 0;
      virtual void stopProfiling() = 0;
   };
/*
   struct Controller {
      StackStampDatabase* stackStampDatabase = StackStampDatabase::create();

      IStackProfiler* createStackProfiler(Thread* thread) {
         return new(this->allocBuffer(sizeof(ThreadStackProfiler))) ThreadStackProfiler((Thread*)thread);
      }
      void traverseStack(uint64_t stackstamp, IStackVisitor* visitor) {
         if (this->stackStampDatabase) {
            this->stackStampDatabase->traverseStack(stackstamp, visitor);
         }
      }

   }
   this->setObject<sat::ThreadStackTracker>(new ThreadStackTracker(this));*/
   namespace analysis {
      SAT_API IStackProfiler* createStackProfiler(Thread* thread);
      SAT_API void traverseStack(uint64_t stackstamp, IStackVisitor* visitor);
      SAT_API uint64_t getCurrentStackStamp();
   }
}

// Stack marking API
extern"C" SAT_API void sat_begin_stack_beacon(sat::StackBeacon * beacon);
extern"C" SAT_API void sat_end_stack_beacon(sat::StackBeacon * beacon);
extern"C" SAT_API void sat_print_stackstamp(uint64_t stackstamp);
