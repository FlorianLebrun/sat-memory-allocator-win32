#pragma once
#include <stdint.h>
#include <atomic>

typedef int tSpinWord;

//#define DISABLE_LOCK
namespace sat {

   class SpinLock {
   public:

      SpinLock() : lockword_(kSpinLockFree) {
      }

      inline void lock() {
#ifndef DISABLE_LOCK
         tSpinWord lockValue = kSpinLockFree;
         if (!lockword_.compare_exchange_weak(lockValue, kSpinLockHeld)) {
            slowLock();
         }
#endif
      }

      inline bool tryLock() {
#ifndef DISABLE_LOCK
         tSpinWord lockValue = kSpinLockFree;
         return lockword_.compare_exchange_weak(lockValue, kSpinLockHeld);
#else
         return true;
#endif
      }

      inline void unlock() {
#ifndef DISABLE_LOCK
         uint64_t prev_value = static_cast<uint64_t>(lockword_.exchange(kSpinLockFree));
         if (prev_value != kSpinLockHeld) slowUnlock();
#endif
      }

      inline bool isHeld() const {
#ifndef DISABLE_LOCK
         return lockword_.load() != kSpinLockFree;
#else
         return false;
#endif
      }

      static void SpinlockPause();
      static void SpinlockWait(volatile tSpinWord* at, int32_t value, int loop);
      static void SpinlockWake(volatile tSpinWord* at);

   private:
      enum {
         kSpinLockFree = 0,
         kSpinLockHeld = 1,
         kSpinLockSleeper = 2
      };

      std::atomic<tSpinWord> lockword_;

      void slowLock();
      void slowUnlock();
   };

   class SpinLockHolder {
   public:
      SpinLock& lock;
      SpinLockHolder(SpinLock& lock) : lock(lock) {
         lock.lock();
      }
      ~SpinLockHolder() {
         this->lock.unlock();
      }
   };

}
