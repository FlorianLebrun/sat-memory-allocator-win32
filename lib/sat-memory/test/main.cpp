#include <sat/memory.hpp>

int main() {
   sat::memory::MemoryTable::initialize();

   auto table = sat::memory::table;
   table->allocBuffer(200);
   table->allocBuffer(200);
   table->allocBuffer(200);
   table->allocBuffer(200);
   table->allocBuffer(200);
   table->allocBuffer(200);
   table->printSegments();
   return 0;
}
