#ifndef _sat_memory_allocator_win32_h_
#error "bad include"
#endif

namespace SAT {

  struct IData : IReleasable {
    virtual ~IData() {}
    virtual const char* getTag() = 0;
    virtual void toString(char* buffer, int size) = 0;
  };

  struct IMark {

    // Timing
    virtual double getStartTime() = 0;
    virtual double getElapsedTime() = 0;

    // Data setting
    virtual IData* get() = 0;
    virtual void set(IData* data) = 0;

    // Terminate the mark
    virtual void complete() = 0;
  };
  
  struct IMarkVisitor {
    virtual void visit(double timeStart, double timeEnd, SAT::IData* data, bool pending) = 0;
  };

}

// Marking API
extern"C" void sat_event_message(const char* tag, const char* format, ...);
extern"C" SAT::IMark* sat_mark_message(const char* tag, const char* format, ...);
extern"C" SAT::IMark* sat_mark(SAT::IData* data);
extern"C" void sat_marking_print();

