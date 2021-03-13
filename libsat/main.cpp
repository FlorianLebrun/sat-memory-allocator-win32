#include <sat-memory-allocator/sat-memory-allocator.h>
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
   switch (fdwReason)
   {
   case DLL_PROCESS_ATTACH:
      sat_init_process(true);
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
