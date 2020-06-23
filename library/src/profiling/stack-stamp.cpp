#include "./stack-stamp.h"

using namespace SAT;


StackStampDatabase* createStackStampDatabase() {
  uintptr_t index = g_SAT.allocSegmentSpan(1);
  StackStampDatabase* tracer = (StackStampDatabase*)(index << SAT::cSegmentSizeL2);
  tracer->StackStampDatabase::StackStampDatabase(uintptr_t(&tracer[1]));
  return tracer;
}
/*
static int printStackHistogramNode(StackNode node, int level, float hitsScale) {
  char buffer[512];
  node->marker->getSymbol(buffer, sizeof(buffer));
  for (int i = 0; i < level; i++) printf("  ");
  printf("%s %3.2f %%\n", buffer, float(node->hits) * hitsScale);

  if (node->subcalls) {
    int nleaf = 0;
    level++;
    for (StackHistogram::tNode* sub = node->subcalls; sub; sub = sub->next) {
      nleaf += printStackHistogramNode(sub, level, hitsScale);
    }
    return nleaf;
  }
  else return 1;
}

void StackHistogram::print() {
  int nleaf = printStackHistogramNode(&root, 0, 100.0f / float(root.hits));
  printf("Num. hits = %d\n", root.hits);
  printf("Num. callgraph path = %d\n", nleaf);
}
void StackHistogram::destroy() {
  delete this;
}
*/
