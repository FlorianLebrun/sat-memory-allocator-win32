#pragma once
#include <stdint.h>

namespace sat {
   namespace timing {

      class SAT_API Chrono {
         int64_t t0;
      public:

         enum PRECISION {
            S = 1,
            MS = 1000,
            US = 1000000,
            NS = 1000000000,
         };

         enum OPS {
            ops = 1,
            Kops = 1000,
            Mops = 1000000,
         };

         Chrono();
         void Start();
         double GetDiffDouble(PRECISION unit = S);
         float GetDiffFloat(PRECISION unit = S);
         float GetOpsFloat(uint64_t ncycle, OPS unit = ops);
         uint64_t GetNumCycleClock();
         uint64_t GetFreq();
      };

      SAT_API uint64_t getCurrentTimestamp();
      SAT_API uint64_t getTimestamp(double time);

      SAT_API double getCurrentTime();
      SAT_API double getTime(uint64_t timestamp);

   }
}