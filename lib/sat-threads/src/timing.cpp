#include "./sat-threads/timing.hpp"
#include <chrono>

static uint64_t timeOrigin = _Query_perf_counter();
static uint64_t timeFrequency = _Query_perf_frequency();

namespace sat {
   namespace timing {

      uint64_t getCurrentTimestamp() {
         return _Query_perf_counter() - timeOrigin;
      }
      uint64_t getTimestamp(double time) {
         return uint64_t(time * double(timeFrequency));
      }

      double getCurrentTime() {
         uint64_t timestamp = getCurrentTimestamp();
         return double(timestamp) / double(timeFrequency);
      }
      double getTime(uint64_t timestamp) {
         return double(timestamp) / double(timeFrequency);
      }

      Chrono::Chrono()
         : t0(_Query_perf_counter()) {
      }

      void Chrono::Start() {
         this->t0 = _Query_perf_counter();
      }

      double Chrono::GetDiffDouble(PRECISION unit) {
         __int64 dt = _Query_perf_counter() - this->t0;
         return (double)(dt * unit) / (double)timeFrequency;
      }

      float Chrono::GetDiffFloat(PRECISION unit) {
         __int64 dt = _Query_perf_counter() - this->t0;
         return (float)((double)(dt * unit) / (double)timeFrequency);
      }

      float Chrono::GetOpsFloat(uint64_t ncycle, OPS unit) {
         return float(double(ncycle) / this->GetDiffDouble(S)) / float(unit);
      }

      uint64_t Chrono::GetNumCycleClock() {
         return _Query_perf_counter() - this->t0;
      }

      uint64_t Chrono::GetFreq() {
         return timeFrequency;
      }

   }
}

