#ifndef chrono_h_
#define chrono_h_

#include <stdint.h>
#include <functional>

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
  void PerfTest(const char* title, const std::function<void()>& cb);
};

#endif