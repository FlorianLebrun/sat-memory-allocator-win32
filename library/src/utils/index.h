#ifndef normalized_
#define normalized_

#include <string>
#include <functional>
#include <iostream>
#include <atomic>
#include <stdint.h>

#include "./binary-arithmetic.h"
#include "./spinlock.h"

#define export

#ifndef null
#define null 0
#endif
#ifndef nil
#define nil 0
#endif

typedef std::string string;

typedef int64_t ThreadId;
typedef int64_t ProcessId;

typedef unsigned char tByte;
typedef unsigned char* pBytes;
typedef unsigned char* tpBytes;
//template<typename T> inline T min(T x, T y) {return (x<y)?x:y;}
//template<typename T> inline T max(T x, T y) {return (x>y)?x:y;}

struct IReleasable {
public:
   virtual void release() = 0;
   virtual void retain() = 0;

   template <class TObject>
   inline TObject* retain(TObject* obj) {obj->retain();return obj;}
};

template <class TReleasable>
struct IReleasableByNumRef : public TReleasable {
public:
   std::atomic<int> numRef;

   inline IReleasableByNumRef() {this->numRef = 1;}
   virtual void release() override {if((this->numRef--) <= 0) this->free();}
   virtual void retain() override {this->numRef++;}
   virtual void free() {delete this;}
};

#define SINGLE_ARG(...) __VA_ARGS__

#endif