#include "./index.h"


SATThreadCallStackStream::SATThreadCallStackStream(SAT::IThread* thread) : SATStream(sizeof(Sample)) {
  this->attributId = SATAttributID::THREAD_CALLSTACK;
  this->thread = thread;
}

void SATThreadCallStackStream::emit() {
  auto sample = this->appendSample<Sample>();
  sample->timestamp = g_SAT.getCurrentTimestamp();
  sample->stackstamp = this->thread->getStackStamp();
}

void SATThreadCallStackStream::computeHistogram(StackHistogram* histogram, uint64_t start, uint64_t end) {
  iterator<Sample> it(this);
  Sample* sample = it.begin();
  sample = it.gotoTime(start);
  while (sample && sample->timestamp < end)
  {
    histogram->insertCallsite(sample->stackstamp);
    sample = it.next();
  }
}

SAT::IStackHistogram* SATProfile::computeCallStackHistogram(SAT::IThread* thread, double startTime, double endTime) {
  StackHistogram* hist = new StackHistogram();
  uint64_t start = (startTime < 0) ? g_SAT.getTimestamp(0) : g_SAT.getTimestamp(startTime);
  uint64_t end = (startTime < 0) ? g_SAT.getCurrentTimestamp() : g_SAT.getTimestamp(endTime);
  for(SATStream* stream=this->streams;stream;stream=stream->profileNext) {
    if(stream->attributId == SATAttributID::THREAD_CALLSTACK && (!thread || stream->thread == thread)) {
      ((SATThreadCallStackStream*)stream)->computeHistogram(hist, start, end);
    }
  }
  return hist;
}

void SATProfile::addStream(SATStream* stream) {
  this->lock.lock();
  stream->profileNext = this->streams;
  this->streams = stream;
  this->lock.unlock();
}

void SATProfile::removeStream(SATStream* stream) {
  this->lock.lock();
  for(SATStream** pstream=&this->streams;*pstream;pstream=&(*pstream)->profileNext) {
    if(*pstream == stream) {
      *pstream = stream->profileNext;
      break;
    }
  }
  stream->profileNext = 0;
  stream->profile = 0;
  this->lock.unlock();
}

void SATProfile::startProfiling() {
  g_SAT.sampler.addProfile(this);
}

void SATProfile::stopProfiling() {
  g_SAT.sampler.removeProfile(this);
}

void SATProfile::destroy() {
  g_SAT.sampler.removeProfile(this);
  while(SATStream* stream = this->streams) {
    this->streams = stream->profileNext;
    delete stream;
  }
  g_SAT.freeBuffer(this, sizeof(*this));
}

/*

int SATProfiler::traverseSamples(SAT::ISamplesVisitor* visitor, int length) {
uint64_t start = g_SAT.getTimestamp(0);
uint64_t end = g_SAT.getCurrentTimestamp();
uint64_t step = (end - start) / length;
return sampler.filterSamples(visitor, start, end, step);
}

int SATProfiler::traverseSamples(SAT::ISamplesVisitor* visitor, double startTime, double endTime, double stepTime) {
uint64_t step = uint64_t(double(g_SAT.timeFrequency)*stepTime);
uint64_t start = (startTime < 0) ? g_SAT.getTimestamp(0) : g_SAT.getTimestamp(startTime);
uint64_t end = (startTime < 0) ? g_SAT.getCurrentTimestamp() : g_SAT.getTimestamp(endTime);
return sampler.filterSamples(visitor, start, end, step);
}

struct tSampleView : SAT::ISampleView {
SAT::ISamplesVisitor* visitor;

tHeapInfos heapsBuffer[8];
tThreadInfos threadsBuffer[8];

tpTickSample sbegin;

tSampleView(SAT::ISamplesVisitor* visitor) {
this->index = -1;
this->visitor = visitor;
this->sbegin = 0;

this->heaps = this->heapsBuffer;
this->threads = this->threadsBuffer;
}
void begin(tpTickSample sample) {
this->sbegin = sample;

// Reset accumulator
this->numThreads = 0;
this->numHeaps = 0;

// Set immediate property
this->index++;
this->time = g_SAT.getTime(sample->timestamp);
this->memoryUse = sample->memoryUse;
}
void end(tpTickSample send) {
tpTickSample sbegin = this->sbegin;

// Set differentiel property
double deltaTime = double(send->timestamp - sbegin->timestamp);
double cpuDelta = double(send->threads[0].cpuTime - sbegin->threads[0].cpuTime);
this->cpuUse = cpuDelta / deltaTime;
if (_isnan(this->cpuUse))this->cpuUse = -1;
else if (this->cpuUse > 1000) this->cpuUse = 1000;

this->visitor->visit(*this);
}
};

int SATSampler::filterSamples(SAT::ISamplesVisitor* visitor, uint64_t start, uint64_t end, uint64_t step) {
ProfileStream* stream = &this->ticksStream;
ProfileStream::iterator it(stream);
tpRecord sample = it.begin();
sample = it.gotoTime(start);

// Traverse samples  
if (sample) {
tSampleView view(visitor);
SAT::ISamplesVisitor::tStatistics stats;
uint64_t samplingDuration = 0;
stats.samplingCycles = 0;
stats.numSamples = 0;

visitor->begin(g_SAT.getTime(sample->timestamp));
while (sample->timestamp < end)
{
// Begin the sample
view.begin(tpTickSample(sample));
stats.samplingCycles++;
samplingDuration += tpTickSample(sample)->duration;

// Merge samples in the same step
uint64_t nextTimestamp = sample->timestamp + step;
while (sample = it.next()) {
if (sample->timestamp < nextTimestamp) {
stats.samplingCycles++;
samplingDuration += tpTickSample(sample)->duration;
}
else break;
}

// End the sample
if (sample) {
view.end(tpTickSample(sample));
stats.numSamples++;
}
else break; // (partial sample)
}

stats.samplingDuration = g_SAT.getTime(samplingDuration);
if(stats.samplingDuration > 0) {
stats.samplingRateLimit = double(stats.samplingCycles) / stats.samplingDuration;
}
else {
stats.samplingRateLimit = 100000;
}
visitor->end(stats);

return stats.numSamples;
}
return 0;
}

*/
