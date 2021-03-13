#pragma once
#include <sat-threads/spinlock.hpp>
#include "./win32/system.h"

namespace sat {

   class TickWorker {
   public:
      TickWorker* scheduledNext;
      TickWorker();
      virtual void execute() = 0;
   };

   static void addTickWorker(TickWorker* worker);
   static void removeTickWorker(TickWorker* worker);
}
