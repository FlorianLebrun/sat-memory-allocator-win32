#ifndef _sat_memory_allocator_win32_h_
#error "bad include"
#endif

#include <stdint.h>

namespace SAT {

   typedef struct tEntry* tpEntry;

   const uintptr_t cSegmentSizeL2 = 16;
   const uintptr_t cSegmentSize = 1 << cSegmentSizeL2;
   const uintptr_t cSegmentOffsetMask = (1 << cSegmentSizeL2) - 1;
   const uintptr_t cSegmentPtrMask = ~cSegmentOffsetMask;
   const uintptr_t cSegmentMinIndex = 32;
   const uintptr_t cSegmentMinAddress = cSegmentMinIndex << cSegmentSizeL2;

   const uintptr_t cEntrySizeL2 = 5;
   const uintptr_t cEntrySize = 1 << cEntrySizeL2;

   typedef void* (*tp_malloc)(size_t size);
   typedef void* (*tp_realloc)(void* ptr, size_t size);
   typedef size_t(*tp_msize)(void* ptr);
   typedef void (*tp_free)(void*);

   struct IReleasable {
      virtual void retain() = 0;
      virtual void release() = 0;
   };

}
