#pragma once
#include <atomic>

namespace sat {

   typedef struct tObjectMetaData {
      static const uint64_t cIsReferer_Bit = uint64_t(1) << 32;
      static const uint64_t cIsStamped_Bit = uint64_t(1) << 33;
      static const uint64_t cIsOverflowProtected_Bit = uint64_t(1) << 34;
      static const uint64_t cTypeID_Mask = 0xffffffff;
      union {
         struct {
            uint32_t typeID;
            unsigned short isReferer : 1; // true, when the object has reference to other data
            unsigned short isStamped : 1; // true, when the object has been stamped by a tracker
            unsigned short isOverflowProtected : 1; // true, when the object has canary words at buffer end
            unsigned short __bit_reserve_0 : 13;
            uint16_t numRef; // ref counter for cached object (avoid releasing of cached object)
         };
         uint64_t bits;
      };
      tObjectMetaData(uint64_t bits = 0) {
         this->bits = bits;
      }
      void lock() {
         ((std::atomic_int16_t*) & this->numRef)->fetch_add(1);
      }
      void unlock() {
         ((std::atomic_int16_t*) & this->numRef)->fetch_sub(1);
      }
   } *tpObjectMetaData;
   static_assert(sizeof(tObjectMetaData) == sizeof(uint64_t), "tObjectMetaData bad size");

   typedef struct tObjectStamp {
      uint64_t stackstamp;
      uint64_t timestamp;
      tObjectStamp(uint64_t timestamp = 0, uint64_t stackstamp = 0) {
         this->stackstamp = stackstamp;
         this->timestamp = timestamp;
      }
   } *tpObjectStamp;
   static_assert(sizeof(tObjectStamp) == sizeof(uint64_t) * 2, "tObjectStamp bad size");

   typedef struct tObjectInfos {
      uint8_t heapID;
      size_t size;
      uintptr_t base;
      tpObjectMetaData meta;

      tObjectInfos& set(int heapID, uintptr_t base, size_t size, uint64_t* meta = 0) {
         this->heapID = heapID;
         this->meta = tpObjectMetaData(meta);
         this->base = base;
         this->size = size;
         return *this;
      }
      tpObjectStamp getStamp() {
         if (!this->meta->isStamped) return 0;
         else return tpObjectStamp(this->base + this->size - sizeof(tObjectStamp));
      }
      void* detectOverflow() {
         if (this->meta && this->meta->isOverflowProtected) {
            uint32_t paddingEnd = this->size - sizeof(uint32_t);
            if (this->meta->isStamped) paddingEnd -= sizeof(tObjectStamp);

            // Read and check padding size
            uint32_t* pBufferSize = (uint32_t*)(this->base + paddingEnd);
            uint32_t bufferSize = (*pBufferSize) ^ 0xabababab;
            if (bufferSize > this->size) {
               return pBufferSize;
            }

            // Read and check padding bytes
            uint8_t* bytes = (uint8_t*)this->base;
            for (uint32_t i = bufferSize; i < paddingEnd; i++) {
               if (bytes[i] != 0xab) {
                  return &bytes[i];
               }
            }
         }
         return 0;
      }
   } *tpObjectInfos;

}
