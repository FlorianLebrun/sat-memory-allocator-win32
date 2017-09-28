#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

class Chrono {
   int64_t freq, t0;
public:
   
   enum PRECISION {
      S=1,
      MS=1000,
      US=1000000,
      NS=1000000000,
   };

   enum OPS {
      ops=1,
      Kops=1000,
      Mops=1000000,
   };

   Chrono();
   void Start();
   double GetDiffDouble(PRECISION unit = S);
   float GetDiffFloat(PRECISION unit = S);
   float GetOpsFloat(uint64_t ncycle, OPS unit = ops);
   uint64_t GetNumCycleClock();
   uint64_t GetFreq();
};

inline int fastrand() {
  static int g_seed = 0;
  g_seed = (214013 * g_seed + 2531011);
  return (g_seed >> 16) & 0x7FFF;
}
