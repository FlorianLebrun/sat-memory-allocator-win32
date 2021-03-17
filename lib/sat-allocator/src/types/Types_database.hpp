
#include <sat/thread.hpp>

namespace sat {
  void TypesDataBase_init();
  extern __declspec(thread) Thread* current_thread;
}

