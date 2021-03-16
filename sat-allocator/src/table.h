#pragma once
#include <stdint.h>

namespace sat {

   struct tHeapEntryID {
      enum _ {
         PAGE_ZONED_BUDDY = memory::tEntryID::__FIRST_HEAP_SEGMENT,
         PAGE_SCALED_BUDDY_3,
         PAGE_SCALED_BUDDY_5,
         PAGE_SCALED_BUDDY_7,
         PAGE_SCALED_BUDDY_9,
         PAGE_SCALED_BUDDY_11,
         PAGE_SCALED_BUDDY_13,
         PAGE_SCALED_BUDDY_15,
         LARGE_OBJECT,
      };
   };

   struct tTechEntryID {
      enum _ {
         TYPES_DATABASE = memory::tEntryID::__FIRST_TECHNICAL_SEGMENT + 0,
      };
   };

}
