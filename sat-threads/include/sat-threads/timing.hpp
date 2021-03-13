#pragma once
#include <stdint.h>

namespace sat {
   namespace timing {

      uint64_t getCurrentTimestamp();
      uint64_t getTimestamp(double time);

      double getCurrentTime();
      double getTime(uint64_t timestamp);

   }
}