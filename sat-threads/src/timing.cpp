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

   }
}
