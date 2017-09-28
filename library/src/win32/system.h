
#include <stdint.h>
#include <sat-memory-allocator/sat-memory-allocator.h>

#ifndef _SAT_system_win32_h_
#define _SAT_system_win32_h_

namespace SystemMemory {
  enum tState {
    FREE,
    RESERVED,
    COMMITTED,
    OUT_OF_MEMORY,
  };
  struct tZoneState {
    tState state;
    uintptr_t address;
    uintptr_t size;
  };

  uintptr_t GetMemorySize();
  uintptr_t GetPageSize();
  tZoneState GetMemoryZoneState(uintptr_t address);

  uintptr_t AllocBuffer(uintptr_t base, uintptr_t limit, uintptr_t size, uintptr_t alignement);

  bool AllocMemory(uintptr_t base, uintptr_t size);
  bool ReserveMemory(uintptr_t base, uintptr_t size);
  bool CommitMemory(uintptr_t base, uintptr_t size);
  bool DecommitMemory(uintptr_t base, uintptr_t size);
  bool ReleaseMemory(uintptr_t base, uintptr_t size);
};

namespace SystemThreading {

  struct tThread {
    uint32_t id;
    void* handle;
    tThread(uint32_t _id = 0, void* _handle = 0)
      : id(_id), handle(_handle) {}
  };

  struct tSymbol {
    uint64_t address; // virtual address including dll base address
    uint32_t offset;  // offset related to resolved address
    uint32_t size;    // estimated size of symbol, can be zero
    uint32_t flags;   // info about the symbols, see the SYMF defines
    uint32_t nameLen;
    char* name;     // symbol name (null terminated string)

    tSymbol(char* name, uint32_t nameLen) {
      this->nameLen = nameLen;
      this->name = name;
    }
    virtual bool resolve(uintptr_t address);
  };

  template <int cNameLen>
  struct tSymbolEx : tSymbol {
    char __nameBytes[cNameLen + 1];
    tSymbolEx() : tSymbol(__nameBytes, cNameLen) { }
  };

  uint64_t GetCurrentThreadId();
  tThread GetCurrentThread();
  tThread GetThreadFromID(uint64_t threadID);
  tThread CreateThread(void(__stdcall* proc)(void*), void* arg);
  
  uint64_t GetTimeStamp();
  uint64_t GetTimeFrequency();

  void SleepThreadCycle(uint64_t ncycle);
  void SleepThread(int time_us);
  void SleepInaccurate(int time_ms);

  void SuspendCurrentThread();
  void SuspendThread(tThread* thread);
  void ResumeThread(tThread* thread);

  bool GetThreadName(tThread* thread, char* buffer, int size);
}

void InitSystemFeatures();
void sat_init_process();
void sat_attach_current_thread();
void sat_dettach_current_thread();
void sat_terminate_process();

#endif
