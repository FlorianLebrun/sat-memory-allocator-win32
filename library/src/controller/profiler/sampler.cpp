#include "./index.h"
#include "../../win32/system.h"

#include <algorithm>

SATSampler::SATSampler()
{
  this->enabled = false;
  this->profiles = 0;
}

void SATSampler::addProfile(SATProfile* profile) {
  if(profile && profile->sampler == 0) {
    this->lock.lock();
    profile->sampler = this;
    profile->samplerNext = this->profiles;
    this->profiles = profile;
    this->resume();
    this->lock.unlock();
  }
}

void SATSampler::removeProfile(SATProfile* profile) {
  if(profile && profile->sampler == this) {
    this->lock.lock();
    for(SATProfile** pprofile=&this->profiles;*pprofile;pprofile=&(*pprofile)->samplerNext) {
      if(*pprofile == profile) {
        *pprofile = profile->samplerNext;
        break;
      }
    }
    profile->samplerNext = 0;
    profile->sampler = 0;
    if(!this->profiles) this->pause();
    this->lock.unlock();
  }
}

void SATSampler::resume() {
  if (!this->enabled) {
    this->enabled = true;
    if (!this->thread.id) {
      this->thread = SystemThreading::CreateThread(&SATSampler::_SamplerThreadEntry, this);
    }
    else {
      SystemThreading::ResumeThread(&this->thread);
    }
  }
}

void SATSampler::pause() {
  if (this->enabled) {
    this->enabled = false;
    SystemThreading::SuspendThread(&this->thread);
  }
}

void __stdcall SATSampler::_SamplerThreadEntry(void* arg) {
  SATSampler* sampler = (SATSampler*)arg;
  printf("[SAT sampler] start\n");
  uint64_t period = g_SAT.timeFrequency / 10000;
  while (1) {
    SystemThreading::SleepThreadCycle(period);
    sampler->lock.lock();
    for(SATProfile* profile=sampler->profiles;profile;profile=profile->samplerNext) {
      for(SATStream* stream=profile->streams;stream;stream=stream->profileNext) {
        stream->emit();
      }
    }
    sampler->lock.unlock();
  }
  sampler->thread.tThread::tThread();
  printf("[SAT sampler] stop\n");
}
