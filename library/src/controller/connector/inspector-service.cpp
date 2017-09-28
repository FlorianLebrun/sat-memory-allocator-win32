#ifdef ENABLE_CONNECTOR
#include "./connector.h"
#include "../../base.h"
#include <stdio.h>

using namespace SAT;

void SerializeByType(void* buffer, int size, TypeDef def, JSONValue& value);

uintptr_t DeserializeAddress(JSONValue* value) {
  if (value->typeID == JSONTypeID::Integer) {
    return value->_integer;
  }
  else if (value->typeID == JSONTypeID::String) {
    const char* ref = value->_string->buffer;
    if(ref[0] == '@') {
      return (uintptr_t)_strtoi64(&ref[1], 0, 16);
    }
  }
  return 0;
}

void SerializeAddress(void* ptr, JSONValue& value) {
  char tmp[24];
  if (sizeof(ptr) == 8) sprintf_s(tmp, 24, "@%lX", ptr);
  else if (sizeof(ptr) == 4) sprintf_s(tmp, 24, "@%.8X", ptr);
  else throw "sizeof(void*) invalid";
  value = tmp;
}


static char hexDigits[17] = "0123456789abcdef";

void SerializeBytes(void* buffer, int size, JSONValue& value) {
  char tmp[8];
  if(buffer) {
    std::stringstream out;
    for(int i=0;i<size;i++) {
      int value = ((uint8_t*)buffer)[i];
      out << hexDigits[value&0xf] << hexDigits[value>>4];
    }
    value["$bytes"].set(out.str());
  }
  else value.set(JSONLib::TypeID::Null);
}

void SerializePointer(void* buffer, int size, int flags, JSONValue& value) {
  void *ptr = *(void**)buffer;
  if(ptr) {
    SerializeAddress(*(void**)buffer, value["$ref"]);
  }
  else value.set(JSONLib::TypeID::Null);
}

void SerializePointerPointer(void* buffer, int size, int flags, JSONValue& value) {
  void **ptr = *(void***)buffer;
  if(ptr) {
    if(sat_get_address_infos(ptr)) {
      SerializePointer(ptr, sizeof(void*), 0, value);
    }
    else {
      SerializeAddress(ptr, value["$ref"]);
      value["$error"] = "Invalid pointer";
    }
  }
  else value.set(JSONLib::TypeID::Null);
}

void SerializeByType(void* buffer, int size, TypeDef def, JSONValue& value) {
  if(size > def->size) size = def->size;
  SerializeByValue(buffer, size, def->encoding, def->flags, def->getInfos(), value);
}

void SerializeByValue(void* buffer, int size, TypeEncoding encoding, TypeFlags flags, JSONValue& value) {
  switch(encoding) {
  case ENC_POINTER:
    SerializePointer(buffer, size, flags, value);
    break;  
  case ENC_POINTER_POINTER:
    SerializePointerPointer(buffer, size, flags, value);
    break;
  case ENC_BOOLEAN:
    value = *(bool*)buffer; 
    break;
  case ENC_INT:
    switch(size) {
    case 1: value = *(int8_t*)buffer; break;
    case 2: value = *(int16_t*)buffer; break;
    case 4: value = *(int32_t*)buffer; break;
    case 8: value = *(int64_t*)buffer; break;
    }
    break;
  case ENC_UINT:
    switch(size) {
    case 1: value = *(uint8_t*)buffer; break;
    case 2: value = *(uint16_t*)buffer; break;
    case 4: value = *(uint32_t*)buffer; break;
    case 8: value = *(uint64_t*)buffer; break;
    }
    break;
  case ENC_FLOAT:
    switch(size) {
    case 4: value = *(float*)buffer; break;
    case 8: value = *(double*)buffer; break;
    }
    break;
  case ENC_ASCII:
    if(flags & ENCFLAG_STRING_PASCAL) value.set(&((char*)buffer)[1], (*(uint8_t*)buffer) < size ? (*(uint8_t*)buffer) : size);
    else value.set((char*)buffer, strnlen((char*)buffer, size));
    break;
  case ENC_BUFFER:
  default:
    if(infos) SerializeBuffer(buffer, size, infos, value);
    else SerializeBytes(buffer, size, value);
    break;
  }

}

void SnapshotData(tpObjectInfos infos, JSONValue& value) {
  struct ObjectStackPrinter : IStackVisitor {
    JSONValue* stack;
    ObjectStackPrinter(JSONValue* _stack) : stack(_stack) {
    }
    virtual void visit(const char* name) {
      this->stack->push_back().set(name);
    }
  };

  IController* sat = sat_get_contoller();
  void* buffer = (void*) infos->base;

  // Serialize meta information
  value["heap"] = int64_t(infos->heapID);
  if(infos->stamp.timestamp) {
    value["time"] = sat->getTime(infos->stamp.timestamp);
  }
  if (infos->stamp.stackstamp) {
    sat->traverseStack(infos->stamp.stackstamp, &ObjectStackPrinter(&value["stack"]));
  }

  // Serialize data
  JSONValue& _data = value["data"];
  SerializeAddress(buffer, value["$ref"]);
  value["size"] = int64_t(infos->size);
  if(TypeDef def = sat_get_types_controller()->getType(infos->meta.typeID)) {
    value["type"] = "object";
    value["extends"] = def->getName();

    JSONValue& _refs = value["refs"];
    for(int i=0;i<def->nrefs;i++) {
      void* ref = *(void**)((char*)buffer + def->refs[i]);
      if(ref) SerializeAddress(ref, _refs.push_back());
    }

    SerializeByType(buffer, infos->size, def, _data);
  }
  else {
    SerializeBytes(buffer, infos->size, _data);
  }
}

static JSONValue getMemoryData(JSONDoc&doc, JSONValue params, JSONValue req) {
  IController* sat = sat_get_contoller();
  JSONValue res(&doc);

  uintptr_t buffer = DeserializeAddress(&req["$ref"]);
  if(!buffer) throw "bad reference";

  tObjectInfos infos;
  if(sat_get_address_infos((void*) buffer, &infos)) {
    SnapshotData(&infos, res);
  }
  else {
    res["$ref"] = req["$ref"];
    res["data"]["$error"] = "Invalid address";
  }
  return res;
}

static JSONValue getMemoryQuery(JSONDoc&doc, JSONValue params, JSONValue req) {
  IController* sat = sat_get_contoller();
  JSONValue res(&doc);

  struct tFilterTime {
    uint64_t start;
    uint64_t end;
  };

  struct tFilterObject {
    virtual bool filter(void* buffer, int size, TypeDef type) = 0;
  };

  struct ObjectsPrinter : IObjectVisitor {
  private:
    IController* sat;
    ITypesController* types;

    JSONValue* root;
    JSONValue* objects;
    uint32_t startIndex, endIndex;
    uint32_t currentIndex;
    uint32_t usedSize;

    uint32_t heapObjectCount;
    uint32_t heapUsedSize;

    tFilterObject* filterObject;
    tFilterTime* filterTime;

  public:
    ObjectsPrinter(IController* sat, JSONValue* root, tFilterTime* filterTime, tFilterObject* filterObject, uint32_t startIndex, uint32_t endIndex, TypeDefID filterType)
    {
      this->sat = sat;
      this->types = sat_get_types_controller();
      this->root = root;
      this->objects = &root->map("objects");

      this->startIndex = startIndex;
      this->endIndex = endIndex;
      this->currentIndex = 0;
      this->usedSize = 0;

      this->heapObjectCount = 0;
      this->heapUsedSize = 0;

      this->filterObject = filterObject;
      this->filterTime = filterTime;
    }
    uint32_t getUsedSize() {
      return this->usedSize;
    }
    uint32_t getObjectCount() {
      return this->currentIndex;
    }
    uint32_t getHeapObjectCount() {
      return this->heapObjectCount;
    }
    uint32_t getHeapUsedSize() {
      return this->heapUsedSize;
    }
    virtual bool visit(tpObjectInfos infos) override {

      // Heap stats
      this->heapObjectCount++;
      this->heapUsedSize += infos->size;

      // Filter on time
      if (infos->meta.isStamped)
      {
        uint64_t timestamp = infos->stamp.timestamp;
        if (this->filterTime && this->filterTime->start > timestamp && this->filterTime->end <= timestamp) {
          return true;
        }
      }

      // Filter on object data & type
      //TypeDef type = this->types->getType(infos->meta.typeID);
      //if(this->filterObject && !this->filterObject->filter((void*)infos->base, infos->size, type)) return true;
      if(!infos->meta.typeID) return true;

      // Append the object
      if(this->currentIndex >= this->startIndex && this->currentIndex < this->endIndex) {
        SnapshotData(infos, objects->push_back());
      }
      this->currentIndex++;
      this->usedSize += infos->size;
      return true;
    }
  };

  struct tFilterObjectType: tFilterObject {
    virtual bool filter(void* buffer, int size, TypeDef type) {
      return type != 0;
    }
  };

  uintptr_t address = 0;
  if (JSONValue* _address = req.find("address")) address = DeserializeAddress(_address);

  uint32_t startIndex = 0;
  if (req.find("startIndex")) startIndex = req["startIndex"];

  uint32_t endIndex = 0;
  if (req.find("endIndex")) {
    endIndex = req["endIndex"];
  }
  else {
    uint32_t maxObjects = 128;
    if (req.find("maxObjects")) {
      maxObjects = req["maxObjects"];
    }
    endIndex = startIndex + maxObjects;
  }

  tFilterTime buffer_filterTime;
  tFilterTime* filterTime = 0;
  if (req.find("start") || req.find("end")) {
    double timeStart = 0;
    double timeEnd = sat->getCurrentTime();
    if (req.find("start")) timeStart = req["start"];
    if (req.find("end")) timeEnd = req["end"];
    buffer_filterTime.start = sat->getTimestamp(timeStart);
    buffer_filterTime.end = sat->getTimestamp(timeEnd);
    filterTime = &buffer_filterTime;
  }

  tFilterObjectType buffer_tFilterObjectType;
  tFilterObject* filterObject = &buffer_tFilterObjectType;

  ObjectsPrinter analyzer(sat, &res, filterTime, filterObject, startIndex, endIndex, 0);
  res.map("objects").set(JSONTypeID::Array);
  sat->traverseObjects(&analyzer, address);
  res["objectCount"] = analyzer.getObjectCount();
  res["usedSize"] = analyzer.getUsedSize();
  res["heapObjectCount"] = analyzer.getHeapObjectCount();
  res["heapUsedSize"] = analyzer.getHeapUsedSize();
  return res;
}

static JSONValue getTickHistogram(JSONDoc&doc, JSONValue params, JSONValue req) {
  IController* sat = sat_get_contoller();
  JSONValue res(&doc);

  struct HistogramPrinter {
    IController* sat;
    HistogramPrinter(IController* sat) {
      this->sat = sat;
    }
    int print(JSONValue* subs, IStackHistogram::tNode* subcalls, float hitsScale) {
      if (subcalls) {
        int nleaf = 0;
        for (IStackHistogram::tNode* sub = subcalls; sub; sub = sub->next) {
          JSONValue* value = &subs->push_back();
          value->map("time").set(float(sub->hits) *hitsScale);
          value->map("name").set(sym);
          nleaf += print(&value->map("subs"), sub->subcalls, hitsScale);
        }
        return nleaf;
      }
      else return 1;
    }
  };

  double startTime = -1.0, endTime = -1.0;
  if (req.find("start")) startTime = req["start"];
  if (req.find("end")) endTime = req["end"];

  IStackHistogram* hist = sat->getProfiler()->computeCallStackHistogram(startTime, endTime);
  HistogramPrinter printer(sat);
  res.map("ncallpath").set((int64_t)printer.print(&res.map("subs"), hist->root.subcalls, 1.0f / float(hist->root.hits)));
  res.map("hits").set((int64_t)hist->root.hits);
  hist->release();

  return res;
}

static JSONValue getTickSampling(JSONDoc&doc, JSONValue params, JSONValue req) {
  IController* sat = sat_get_contoller();
  JSONValue res(&doc);

  struct StackPrinter : IStackVisitor {
    JSONValue* value;
    StackPrinter(JSONValue* _value) :value(_value) {
    }
    virtual void visit(const char* name) override {
      value->push_back().set(name);
    }
  };

  struct SamplesPrinter : ISamplesVisitor {
    IProfiler* profiler;
    JSONValue* root;

    JSONValue* time;
    JSONValue* cpuUse;
    JSONValue* memoryUse;
    JSONValue* events;

    SamplesPrinter(JSONValue* root, IProfiler* profiler) {
      this->root = root;
      this->profiler = profiler;

      JSONValue* samples = &root->map("samples");
      this->time = &samples->map("time");
      this->cpuUse = &samples->map("cpu-use");
      this->memoryUse = &samples->map("memory-use");
      this->events = &samples->map("events");
    }
    virtual void begin(double time) override {
    }
    virtual void end(tStatistics stats) override {
      JSONValue* _stats = &root->map("stats");
      _stats->map("samplingCycles").set(int64_t(stats.samplingCycles));
      _stats->map("samplingRateLimit").set(stats.samplingRateLimit);
      _stats->map("samplingDuration").set(stats.samplingDuration);
    }
    virtual void visit(ISampleView& sample) override {
      this->time->push_back().set(sample.time);
      this->cpuUse->push_back().set(sample.cpuUse);
      this->memoryUse->push_back().set(sample.memoryUse);
      JSONValue* ev = &this->events->push_back();
      ev->set(JSONTypeID::Null);
    }
  };

  IHeap* heap = sat->getHeap(0);
  if (IProfiler* profiler = sat_get_contoller()->getProfiler()) {
    double timeMax = sat->getCurrentTime();

    if (req.find("blocks")) {
      double length = req.find("length") ? req["length"] : 128.0;
      JSONLib::array_iterator<> it(&req["blocks"]);
      for (JSONValue* vitem = it.begin(); vitem; vitem = it.next()) {
        int index = (*vitem)["index"], level = (*vitem)["level"];
        double interval = (level < 0) ? double(1 << -level) : 1.0 / double(1 << level);
        double startTime = double(index) * interval;
        double endTime = startTime + interval;
        double stepTime = interval / length;
        profiler->traverseSamples(&SamplesPrinter(&res.push_back(), profiler), startTime, endTime, stepTime);
      }
    }
    else {
      double startTime = 0.0;
      double endTime = sat->getCurrentTime();
      double stepTime = 0.001;
      if (req.find("start")) startTime = req["start"];
      if (req.find("end")) endTime = req["end"];
      if (req.find("step")) stepTime = req["step"];
      if (req.find("length")) stepTime = (endTime - startTime) / double(req["length"]);

      res.set(JSONTypeID::Map);
      res.map("timeStart").set(startTime);
      res.map("timeEnd").set(endTime);

      tProfileStatistics gstats = profiler->getStatistics();
      res.map("stats").map("memoryUse").set(gstats.samplingMemoryUse);

      profiler->traverseSamples(&SamplesPrinter(&res, profiler), startTime, endTime, stepTime);
    }

    profiler->release();
  }
  return res;
}

static JSONValue getThread(JSONDoc&doc, JSONValue params, JSONValue req) {
  IController* sat = sat_get_contoller();
  JSONValue res(&doc);
  int id = req["id"].toNumber();
  if(SAT::IThread* thread = sat->getThreadById(id)) {
    if(req.find("watch")) {
      if(req["watch"].toBoolean()) thread->watch();
      else thread->unwatch();
    }
  }
  else {
    res = false;
  }
  return res;
}

static JSONValue getStats(JSONDoc&doc, JSONValue params, JSONValue req) {
  IController* sat = sat_get_contoller();
  JSONValue res(&doc);
  JSONValue& v_heaps = res["heaps"];
  JSONValue& v_threads = res["threads"];

  SAT::tEntry* table = sat_get_table();
  for (int id = 0; id < 256; id++) {
    if (IHeap* heap = sat->getHeap(id)) {

      uintptr_t nsegments = 0;
      for (int i = 0; i < table->SATDescriptor.length; i++) {
        if (table[i].getHeapID() == id) {
          nsegments++;
        }
      }
      double size = double(nsegments)*double(1 << table->SATDescriptor.bytesPerSegmentL2) / 1000000.0;

      JSONValue& v_heap = v_heaps.push_back();
      v_heap["id"] = id;
      v_heap["name"] = heap->getName();
      v_heap["size"] = size;
    }
  }

  g_SAT.threads_lock.lock();
  for (Thread* thread=g_SAT.threads_list;thread;thread=thread->nextThread) {
    JSONValue& v_thread = v_threads.push_back();
    v_thread["id"] = thread->id;
    v_thread["name"] = thread->getName();
    if(thread->isSampled) v_thread["sampled"] = true;
  }
  g_SAT.threads_lock.unlock();

  return res;
}

#include "../marking-allocator.h"

static JSONValue getMarkAll(JSONDoc&doc, JSONValue params, JSONValue req) {
  IController* sat = sat_get_contoller();
  JSONValue res(&doc);

  struct MarkVisitor: SAT::IMarkVisitor {
    JSONValue& marks;
    MarkVisitor(JSONValue& _marks) : marks(_marks) {
    }
    virtual void visit(double timeStart, double timeEnd, SAT::IData* data, bool pending) override {
      JSONValue& mark = this->marks.push_back();
      mark["start"] = timeStart;
      mark["end"] = timeEnd;
      if(data) {
        char buffer[1024];
        data->toString(buffer, sizeof(buffer));
        mark["tag"] = data->getTag();
        mark["title"] = buffer;
      }
    }
  };

  SAT::marking_database.traverseAllMarks(&MarkVisitor(res["marks"]));
  return res;
}

void map_inspector_service(tHttpMapping& mapping) {
  mapping["/sat/memory/data"] = std::bind(&::getMemoryData, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  mapping["/sat/memory/query"] = std::bind(&::getMemoryQuery, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  mapping["/sat/stats"] = std::bind(&::getStats, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  mapping["/sat/tick-sampling"] = std::bind(&::getTickSampling, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  mapping["/sat/tick-histogram"] = std::bind(&::getTickHistogram, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  mapping["/sat/mark/all"] = std::bind(&::getMarkAll, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  mapping["/sat/thread"] = std::bind(&::getThread, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

#endif