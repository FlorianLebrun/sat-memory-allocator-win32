#pragma once
#include <sat/spinlock.hpp>
#include "./win32/system.h"

namespace sat {

   class TickWorker {
   public:
      TickWorker* scheduledNext;
      TickWorker();
      virtual void execute() = 0;
   };

   void addTickWorker(TickWorker* worker);
   void removeTickWorker(TickWorker* worker);
}
