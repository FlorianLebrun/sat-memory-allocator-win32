#pragma once
#include <atomic>
#include <stdint.h>
#include <assert.h>

namespace sat {
   namespace shared {
      class ISharedObject {
      public:
         virtual void retain() = 0;
         virtual void release() = 0;
         virtual ~ISharedObject() = default;
      };

      template <class Interface = ISharedObject>
      class SharedObject : public Interface {
         std::atomic_uint32_t numRefs;
      public:
         SharedObject() {
            this->numRefs = 1;
         }
         virtual void retain() override final {
            assert(this->numRefs >= 0);
            this->numRefs++;
         }
         virtual void release() override final {
            assert(this->numRefs >= 0);
            if (!(--this->numRefs)) this->dispose();
         }
         virtual void dispose() {
            delete this;
         }
      };
   }
}