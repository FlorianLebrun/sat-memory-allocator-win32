#ifdef ENABLE_CONNECTOR
#include "./connector.h"

struct HttpTransaction : public webx::NoAttributs<webx::Releasable<webx::IStream>> {
  tHttpFunc exec;
  webx::IStream *req;
  std::stringstream out;

  HttpTransaction(webx::IStream *req, tHttpFunc exec) {
    this->exec = exec;
    this->req = req;
  }
  virtual bool connect(IStream *stream) override {
    return (this->req == stream);
  }
  virtual void close() override {
    JSONDoc doc;
    JSONValue params(&doc);

    JSONValue body(&doc);
    std::string out = this->out.str();
    if (int size = out.size()) {
      JSON::parse(body, out.c_str(), size);
    }

    webx::IData* data;
    try {
      JSONValue result = this->exec(doc, params, body);
      if (result.typeID != JSONTypeID::Undefined) {
        std::string bytes = JSON::stringify(result);
        int size = bytes.size();
        data = webx::IData::New(bytes.c_str(), size);
        data->setAttributString("Content-Type", "application/json");
        data->setAttributInt("Content-Length", size);
        data->setAttributInt(":status", 200);
      }
      else {
        data = webx::IData::New("", 0);
        data->setAttributInt(":status", 204);
      }
    }
    catch(...) {
      const char* message = "Bad request";
      data = webx::IData::New(message, strlen(message));
      data->setAttributInt(":status", 400);
    }
    req->write(data);
    data->release();

    // Close transaction
    req->close();
  }
  virtual bool write(webx::IData* data) override {
    char* buffer;
    uint32_t size;
    data->getData(buffer, size);
    out.write(buffer, size);
    return true;
  }
  virtual void free() override {
    delete this;
  }
};

struct SATEngineContext : public webx::NoAttributs<webx::Releasable<webx::IEngineContext>> {
  webx::IEngineHost* host;
  tHttpMapping mapping;

  SATEngineContext(webx::IEngineHost* host) {
    this->host = host;
    map_inspector_service(this->mapping);
    if(host) host->notify(webx::New(new webx::events::RuntimeStartup()));
  }
  virtual const char *getName() override {
    return "sat-memory-allocator";
  }
  virtual webx::ISessionContext *createSession(webx::ISessionHost *host, const char* name, const char *config) override {
    return 0;
  }
  virtual void notify(webx::IEvent *event) {
    event->release();
  }
  virtual void dispatchWebSocket(webx::IStream *stream) override {
  }
  virtual void dispatchTransaction(webx::IStream *req) override {
    std::string method = req->getAttributString(":method");
    std::string path = req->getAttributString(":path");

    tHttpMapping::iterator it = this->mapping.find(path);
    if (it == this->mapping.end()) {
      webx::IData* data = webx::IData::New("", 0);
      data->setAttributInt(":status", 404);
      req->write(data);
      req->close();
    }
    else {
      if (method == "OPTIONS") {
        webx::IData* data = webx::IData::New("", 0);
        data->setAttributString("Access-Control-Allow-Headers", "Content-Type");
        data->setAttributString("Access-Control-Allow-Methods", "GET,PUT,POST,DELETE");
        data->setAttributString("Access-Control-Allow-Origin", "*");
        data->setAttributInt(":status", 204);
        req->write(data);
        req->close();
      }
      else if (method == "GET" || method == "POST" || method == "PUT") {
        req->connect(new HttpTransaction(req, it->second));
      }
      else {
        webx::IData* data = webx::IData::New("", 0);
        data->setAttributInt(":status", 400);
        req->write(data);
        req->close();     
      }
    }
  }
  virtual bool close() override {
    return false;
  }
  virtual void free() override {
    delete this;
  }
};


extern"C" webx::IEngineContext* sat_connect(webx::IEngineHost* host, const char* config) {
  SATEngineContext* connector = new SATEngineContext(host);
  printf("sat connect...\n");
  return connector;
}

#else

#include <stdio.h>

extern"C" void* sat_connect(void* host, const char* config) {
  printf("sat connect...disabled\n");
  return 0;
}

#endif
