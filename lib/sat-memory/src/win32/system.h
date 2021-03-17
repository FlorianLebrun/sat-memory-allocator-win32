#pragma once
#include <stdint.h>

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

void InitSystemFeatures();
void sat_init_process();
void sat_attach_current_thread();
void sat_dettach_current_thread();
void sat_terminate_process();

