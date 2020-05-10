#include <windows.h>
#include <psapi.h>
#include <malloc.h>
#include <stdio.h>
#include <winternl.h>
#include <Dbghelp.h>
#include "../base.h"

#pragma comment(lib,"Dbghelp.lib")
#pragma comment(lib,"Ntdll.lib")

void InitSystemFeatures() {
  // Nothing to do for now
}

uintptr_t SystemMemory::GetMemorySize() {
  SYSTEM_INFO infos;
  GetSystemInfo(&infos);
  return (uintptr_t)infos.lpMaximumApplicationAddress;
}

uintptr_t SystemMemory::GetPageSize() {
  SYSTEM_INFO infos;
  GetSystemInfo(&infos);
  if (infos.dwPageSize > infos.dwAllocationGranularity) return infos.dwPageSize;
  else return infos.dwAllocationGranularity;
}

SystemMemory::tZoneState SystemMemory::GetMemoryZoneState(uintptr_t address) {
  MEMORY_BASIC_INFORMATION infos;
  SystemMemory::tZoneState region;
  if (!VirtualQuery((void*)address, &infos, sizeof(MEMORY_BASIC_INFORMATION))) {
    region.address = address;
    region.size = 0;
    region.state = SystemMemory::tState::OUT_OF_MEMORY;
  }
  else {
    region.address = (uintptr_t)infos.BaseAddress;
    region.size = infos.RegionSize;
    switch (infos.State) {
    case MEM_COMMIT:region.state = SystemMemory::tState::COMMITTED; break;
    case MEM_RESERVE:region.state = SystemMemory::tState::RESERVED; break;
    case MEM_FREE:region.state = SystemMemory::tState::FREE; break;
    }
  }
  return region;
}

uintptr_t SystemMemory::AllocBuffer(uintptr_t base, uintptr_t limit, uintptr_t size, uintptr_t alignement) {
  while (base < (uintptr_t)limit) {
    uintptr_t address = (uintptr_t)VirtualAlloc((void*)base, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (address == base) return base;
    base += alignement;
  }
  return 0;
}

bool SystemMemory::AllocMemory(uintptr_t base, uintptr_t size) {
  return base == (uintptr_t)VirtualAlloc((void*)base, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

bool SystemMemory::ReserveMemory(uintptr_t base, uintptr_t size) {
  return base == (uintptr_t)VirtualAlloc((void*)base, size, MEM_RESERVE, PAGE_NOACCESS);
}

bool SystemMemory::CommitMemory(uintptr_t base, uintptr_t size) {
  uintptr_t ptr = (uintptr_t)VirtualAlloc((void*)base, size, MEM_COMMIT, PAGE_READWRITE);
  return base == ptr;
}

bool SystemMemory::DecommitMemory(uintptr_t base, uintptr_t size) {
  return VirtualFree((void*)base, size, MEM_DECOMMIT);
}

bool SystemMemory::ReleaseMemory(uintptr_t base, uintptr_t size) {
  return VirtualFree((void*)base, size, MEM_RELEASE);
}

SystemThreading::tThread SystemThreading::CreateThread(void(__stdcall* proc)(void*), void* arg) {
  tThread thread;
  DWORD threadID;
  thread.handle = ::CreateThread(NULL, 0, LPTHREAD_START_ROUTINE(proc), arg, 0, &threadID);
  thread.id = threadID;
  return thread;
}

uint64_t SystemThreading::GetTimeStamp() {
  uint64_t timestamp;
  QueryPerformanceCounter((LARGE_INTEGER*)&timestamp);
  return timestamp;
}

uint64_t SystemThreading::GetTimeFrequency() {
  uint64_t frequency;
  QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
  return frequency;
}

uint64_t SAT::Thread::CaptureThreadCpuTime() {
  uint64_t timestamp;
  QueryThreadCycleTime(this->handle.handle, &timestamp);
  return timestamp;
}

void SystemThreading::SleepThreadCycle(uint64_t ncycle) {
  uint64_t endTime = SystemThreading::GetTimeStamp() + ncycle;
  while (SystemThreading::GetTimeStamp() < endTime) {
    ::SwitchToThread();
  }
}

void SystemThreading::SleepThread(int time_us) {
  uint64_t freq = SystemThreading::GetTimeFrequency();
  uint64_t ncycle = double(time_us*1000000.0) / double(freq);
  uint64_t endTime = SystemThreading::GetTimeStamp() + ncycle;
  if (time_us > 2000) {
    ::Sleep(time_us / 1000);
  }
  while (SystemThreading::GetTimeStamp() < endTime) {
    ::SwitchToThread();
  }
}

void SystemThreading::SleepInaccurate(int time_ms) {
  ::Sleep(time_ms);
}

void SystemThreading::SuspendCurrentThread() {
  ::SuspendThread(::GetCurrentThread());
}

void SystemThreading::SuspendThread(SystemThreading::tThread* thread) {
  ::SuspendThread(thread->handle);
}

void SystemThreading::ResumeThread(SystemThreading::tThread* thread) {
  ::ResumeThread(thread->handle);
}

SystemThreading::tThread SystemThreading::GetThreadFromID(uint64_t threadID) {
  return SystemThreading::tThread(threadID, OpenThread(THREAD_ALL_ACCESS, FALSE, threadID));
}

SystemThreading::tThread SystemThreading::GetCurrentThread() {
  return SystemThreading::GetThreadFromID(GetCurrentThreadId());
}

uint64_t SystemThreading::GetCurrentThreadId() {
  return ::GetCurrentThreadId();
}

bool SystemThreading::GetThreadName(SystemThreading::tThread* thread, char* buffer, int size) {
  uintptr_t startAddress = 0;
  NtQueryInformationThread(thread->handle, THREADINFOCLASS(9)/*ThreadQuerySetWin32StartAddress*/, &startAddress, sizeof(startAddress), 0);
  tSymbol symbol(buffer, size);
  if(!symbol.resolve(startAddress)) {
    strcpy_s(buffer, size, "(unamed)");
  }
  return true;
}

bool SystemThreading::tSymbol::resolve(uintptr_t address)
{
  static bool isSymInitiate = false;
  if (!isSymInitiate) {
    if (!SymInitialize(GetCurrentProcess(), NULL, TRUE)) {
      printf("Issue with SymInitialize ! %s\n", ::GetLastError());
    }
    isSymInitiate = true;
  }

  IMAGEHLP_SYMBOL64* Sym = (IMAGEHLP_SYMBOL64*)alloca(sizeof(IMAGEHLP_SYMBOL64) + this->nameLen);
  uint64_t offset;
  Sym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
  Sym->MaxNameLength = this->nameLen;
  if (SymGetSymFromAddr64(GetCurrentProcess(), address, &offset, Sym)) {
    Sym->Name[this->nameLen - 1] = 0;
    this->address = Sym->Address;
    this->size = Sym->Size;
    this->offset = offset;
    this->flags = Sym->Flags;
    strcpy_s(this->name, this->nameLen, Sym->Name);
    return true;
  }
  else if (address) {
    sprintf_s(this->name, this->nameLen, "(unresolved 0x%.16X : code %d)", address, GetLastError());
    return false;
  }
  else {
    this->name[0] = 0;
    return false;
  }
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL,  DWORD fdwReason, LPVOID lpvReserved)
{
  extern void PatchWindowsFunctions();

  switch (fdwReason)
  {
  case DLL_PROCESS_ATTACH:
    sat_init_process();
    PatchWindowsFunctions();
    break;
  case DLL_PROCESS_DETACH:
    sat_terminate_process();
    break;
  case DLL_THREAD_ATTACH:
    sat_attach_current_thread();
    break;
  case DLL_THREAD_DETACH:
    sat_dettach_current_thread();
    break;
  default:
    break;
  }
  return TRUE;
}
