
#include <json-soft-document>
#include <node-webengine-hosting>
#include "../../index.h"

typedef SoftDoc<> JSONLib;
typedef SoftDoc<>::Document JSONDoc;
typedef SoftDoc<>::Value JSONValue;
typedef SoftDoc<>::TypeID JSONTypeID;
typedef SoftDoc<>::JSON JSON;

typedef std::function<JSONValue(JSONDoc&, JSONValue, JSONValue)> tHttpFunc;
typedef std::map<std::string, tHttpFunc> tHttpMapping;

namespace webx {
  namespace events {

    class Event : public BuiltinEvent<IEvent>
    {
      virtual void free() override {
        delete this;
      }
    };

    class SessionStartup : public Event
    {
      virtual const char *eventName() override { return "Session.startup"; }
    };

    class SessionExit : public Event
    {
      virtual const char *eventName() override { return "Session.exit"; }
    };

    class RuntimeStartup : public Event
    {
      virtual const char *eventName() override { return "Runtime.startup"; }
    };

    class RuntimeExit : public Event
    {
      virtual const char *eventName() override { return "Runtime.exit"; }
    };
  }
}

void map_inspector_service(tHttpMapping& mapping);
