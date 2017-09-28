
#include "spinlock.h"

#include <intrin.h>

// NOTE on the Lock-state values:
//
// kSpinLockFree represents the unlocked state
// kSpinLockHeld represents the locked state with no waiters
// kSpinLockSleeper represents the locked state with waiters

static int adaptive_spin_count = 0;

static int SuggestedDelayNS(int loop);

/*******************************************************
**** FOR _WIN32
*******************************************************/
#if defined(_WIN32)
#include <windows.h>
void SpinLock::SpinlockWait(volatile tSpinWord *w, int32_t value, int loop) {
   if (loop == 0) {
   } else if (loop == 1) {
      Sleep(0);
   } else {
      Sleep(SuggestedDelayNS(loop) / 1000000);
   }
}
void SpinLock::SpinlockWake(volatile tSpinWord *w) {
}

/*******************************************************
**** FOR __linux__
*******************************************************/
#elif defined(__linux__)
#include <errno.h>
#include <sched.h>
#include <time.h>
#include <limits.h>
#include "base/linux_syscall_support.h"
#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_PRIVATE_FLAG 128
static bool have_futex;
static int futex_private_flag = FUTEX_PRIVATE_FLAG;
static struct InitModule {
   InitModule() {
      int x = 0;
      // futexes are ints, so we can use them only when
      // that's the same size as the lockword_ in spinlock.
      have_futex = (sizeof (tSpinWord) == sizeof (int) &&
         sys_futex(&x, FUTEX_WAKE, 1, NULL, NULL, 0) >= 0);
      if (have_futex &&
         sys_futex(&x, FUTEX_WAKE | futex_private_flag, 1, NULL, NULL, 0) < 0) {
            futex_private_flag = 0;
      }
   }
} init_module;
void SpinLock::SpinlockWait(volatile tSpinWord *w, int32_t value, int loop) {
   if (loop != 0) {
      int save_errno = errno;
      struct timespec tm;
      tm.tv_sec = 0;
      if (have_futex) {
         tm.tv_nsec = SuggestedDelayNS(loop);
      } else {
         tm.tv_nsec = 2000001;   // above 2ms so linux 2.4 doesn't spin
      }
      if (have_futex) {
         tm.tv_nsec *= 16;  // increase the delay; we expect explicit wakeups
         sys_futex(reinterpret_cast<int *>(const_cast<tSpinWord *>(w)),
            FUTEX_WAIT | futex_private_flag,
            value, reinterpret_cast<struct kernel_timespec *>(&tm),
            NULL, 0);
      } else {
         nanosleep(&tm, NULL);
      }
      errno = save_errno;
   }
}
void SpinLock::SpinlockWake(volatile tSpinWord *w) {
   if (have_futex) {
      sys_futex(reinterpret_cast<int *>(const_cast<tSpinWord *>(w)),
         FUTEX_WAKE | futex_private_flag, all? INT_MAX : 1,
         NULL, NULL, 0);
   }
}

/*******************************************************
**** FOR posix
*******************************************************/
#else
#include <config.h>
#include <errno.h>
#ifdef HAVE_SCHED_H
#include <sched.h>      /* For sched_yield() */
#endif
#include <time.h>       /* For nanosleep() */
void SpinLock::SpinlockWait(volatile tSpinWord *w, int32_t value, int loop) {
   int save_errno = errno;
   if (loop == 0) {
   } else if (loop == 1) {
      sched_yield();
   } else {
      struct timespec tm;
      tm.tv_sec = 0;
      tm.tv_nsec = SuggestedDelayNS(loop);
      nanosleep(&tm, NULL);
   }
   errno = save_errno;
}
void SpinLock::SpinlockWake(volatile tSpinWord *w) {
}
#endif

inline static void SpinlockPause() {
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
  __asm__ __volatile__("rep; nop" : : );
#else
  __nop();
#endif
}

void SpinLock::slowLock() {
   for(int lock_wait_call_count=1;;lock_wait_call_count++) {

      // Spin to give this thread some chance of obtaining the lock without wait.
      //   Monitor the lock to see if its value changes within some time
      //   period (adaptive_spin_count loop iterations). The last value read
      //   from the lock is returned from the method.
      int c = adaptive_spin_count;
      while (lockword_.load() != kSpinLockFree && --c > 0) {
         SpinlockPause();
      }

      // If the lock is currently held, but not marked as having a sleeper, mark
      // it as having a sleeper.
      tSpinWord lock_value = kSpinLockFree;
      if(lockword_.compare_exchange_weak(lock_value, kSpinLockSleeper)) {
         return; // Lock acquire
      }
      else if(lock_value == kSpinLockHeld) {
         // Here, just "mark" that the thread is going to sleep.  Don't store the
         // lock wait time in the lock as that will cause the current lock
         // owner to think it experienced contention.
         if (lockword_.compare_exchange_weak(lock_value, kSpinLockSleeper)) {
            // Successfully transitioned to kSpinLockSleeper.  Pass
            // kSpinLockSleeper to the SpinlockWait routine to properly indicate
            // the last lock_value observed.
            lock_value = kSpinLockSleeper;
         } else if (lock_value == kSpinLockFree) {
            // Lock is free again, so try and acquire it before sleeping.  The
            // new lock state will be the number of cycles this thread waited if
            // this thread obtains the lock.
            if(lockword_.compare_exchange_weak(lock_value, kSpinLockSleeper)) {
               return; // Lock acquire
            }
         }
      }

      // Wait for an OS specific delay.
      SpinlockWait((int*)&lockword_, lock_value, lock_wait_call_count);
   }
}

void SpinLock::slowUnlock() {
   // wake waiter if necessary
   SpinlockWake((int*)&lockword_);
}

// Return a suggested delay in nanoseconds for iteration number "loop"
static int SuggestedDelayNS(int loop) {
   // Weak pseudo-random number generator to get some spread between threads
   // when many are spinning.
#ifdef BASE_HAS_ATOMIC64
   static Atomic64 rand;
   uint64 r = rand.load(&rand);
   r = 0x5deece66dLL * r + 0xb;   // numbers from nrand48()
   rand.store(&rand, r);

   r <<= 16;   // 48-bit random number now in top 48-bits.
   if (loop < 0 || loop > 32) {   // limit loop to 0..32
      loop = 32;
   }
   // loop>>3 cannot exceed 4 because loop cannot exceed 32.
   // Select top 20..24 bits of lower 48 bits,
   // giving approximately 0ms to 16ms.
   // Mean is exponential in loop for first 32 iterations, then 8ms.
   // The futex path multiplies this by 16, since we expect explicit wakeups
   // almost always on that path.
   return r >> (44 - (loop >> 3));
#else
   static std::atomic<tSpinWord> rand;
   uint32_t r = rand.load();
   r = 0x343fd * r + 0x269ec3;   // numbers from MSVC++
   rand.store(r);

   r <<= 1;   // 31-bit random number now in top 31-bits.
   if (loop < 0 || loop > 32) {   // limit loop to 0..32
      loop = 32;
   }
   // loop>>3 cannot exceed 4 because loop cannot exceed 32.
   // Select top 20..24 bits of lower 31 bits,
   // giving approximately 0ms to 16ms.
   // Mean is exponential in loop for first 32 iterations, then 8ms.
   // The futex path multiplies this by 16, since we expect explicit wakeups
   // almost always on that path.
   return r >> (12 - (loop >> 3));
#endif
}
