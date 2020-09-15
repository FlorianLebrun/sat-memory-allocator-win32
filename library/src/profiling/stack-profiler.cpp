#include "./stack-profiler.h"

SATStackProfiling* createStackProfiling() {
  uintptr_t index = g_SAT.allocSegmentSpan(1);
  SATStackProfiling* profiling = (SATStackProfiling*)(index << SAT::cSegmentSizeL2);
  profiling->SATStackProfiling::SATStackProfiling();
  return profiling;
}
SATStackProfiling::SATStackProfiling()
  : manifold(uintptr_t(&this[1]))
{
}
SAT::IStackProfiling::Node SATStackProfiling::getRoot() {
  return &this->manifold.root;
}
void SATStackProfiling::print() {

}
void SATStackProfiling::destroy() {
}

SATProfile::SATProfile() {
  this->target = 0;
  this->profiling = 0;
  this->lastSampleTime = 0;
}

void SATProfile::execute() {
  if (auto node = this->profiling->manifold.getStackNode(this->target)) {
    node->data.hits++;
    this->lastSampleTime = g_SAT.getCurrentTimestamp();
  }
}

SAT::IStackProfiling* SATProfile::flushProfiling() {
  SAT::IStackProfiling* profiling = this->profiling;
  this->profiling = 0;
  return profiling;
}

void SATProfile::startProfiling(SAT::IThread* thread) {
  if (!this->profiling) {
    this->target = (SAT::Thread*)thread;
    this->profiling = createStackProfiling();
    g_SAT.scheduler.addTickWorker(this);
  }
  else {
    printf("Cannot start profiling.\n");
  }
}

void SATProfile::stopProfiling() {
  g_SAT.scheduler.removeTickWorker(this);
}

void SATProfile::destroy() {
  g_SAT.scheduler.removeTickWorker(this);
  g_SAT.freeBuffer(this, sizeof(*this));
}
