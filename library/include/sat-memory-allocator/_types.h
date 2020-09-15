#ifndef _sat_memory_allocator_win32_h_
#error "bad include"
#endif

#include <vector>

namespace SAT {
  typedef uint32_t TypeDefID;
  typedef struct tTypeDef *TypeDef;
  typedef struct ITypeHandle *TypeHandle;
  struct ITypesController;
}

extern"C" uintptr_t sat_types_base;

extern"C" SAT::ITypesController* sat_get_types_controller();

namespace SAT {

  enum TypeEncoding : uint8_t {
    ENC_BUFFER = 0,
    ENC_POINTER, // Pointer to a buffer
    ENC_POINTER_POINTER, // Pointer of pointer to a buffer
    ENC_ASCII,
    ENC_UTF8,
    ENC_UTF16,
    ENC_INT,
    ENC_UINT,
    ENC_FLOAT,
    ENC_BOOLEAN,
  };

  enum TypeFlags : uint8_t {
    ENCFLAG_NONE = 0,
    ENCFLAG_DEFINE_TYPE = 0x01,
    ENCFLAG_DEFINE_SIZE = 0x00,
    ENCFLAG_STRING_PASCAL = 0x02,
    ENCFLAG_STRING_C = 0x00,
  };

  __forceinline TypeFlags operator | (TypeFlags x, TypeFlags y) {
    return TypeFlags(uint8_t(x)|uint8_t(y));
  }

  struct ITypeHandle {
    virtual void getTitle(char* buffer, int size) = 0;
  };

  struct ITypesController {
    virtual void* alloc(int size) = 0;
    virtual void free(void* ptr) = 0;

    virtual TypeDef getType(TypeDefID typeID) = 0;
    virtual TypeDefID getTypeID(TypeDef typd) = 0;

    virtual TypeDef allocTypeDef(uint32_t length) = 0;
  };

  struct tTypeBuffer {
    uint32_t length; // Size of this structure in memory
  };

  template <typename T>
  struct tTypeVector {
    uint32_t count;
    T values[0]; // list of refs offset
  };

  struct tTypeDef : tTypeBuffer {
    TypeEncoding encoding;
    TypeFlags flags;
    TypeHandle handle;
    uint32_t size; // Size of the type in bytes, 0: when type shall be lazy build
    uint32_t nrefs;
    uint32_t refs[0]; // list of refs offset

    const char* getName() {
      return (const char*)&this->refs[this->nrefs];
    }
    template <class T>
    static T& zero() { return (*(T*)0); }
  };

  class TypeDefBuilder {
    struct tRef {
      int offset;
      tRef(int offset) {
        this->offset = offset;
      }
    };

    const char* name;
    TypeEncoding encoding;
    int size;
    std::vector<tRef> refs;
    TypeHandle handle;

  public:
    TypeDefBuilder(SAT::TypeEncoding encoding, int size, TypeHandle handle = 0) {
      this->encoding = encoding;
      this->size = size;
      this->handle = handle;
    }
    TypeDefBuilder(int size, TypeHandle handle = 0) {
      this->encoding = ENC_BUFFER;
      this->size = size;
      this->handle = handle;
    }
    void setName(const char* name) {
      this->name = name;
    }
    void addExtend(TypeDef def) {
      for(int i=0;def->nrefs;i++) {
        this->refs.push_back(def->refs[i]);
      }
    }
    void addReference(int offset) {
      this->refs.push_back(tRef(offset));
    }
    TypeDef build(int handleSize = 0) {
      ITypesController* types = sat_get_types_controller();

      // Compute allocated size
      int szName = strlen(this->name)+1;
      int length = sizeof(tTypeDef);
      length += this->refs.size()*sizeof(uint32_t);
      int nameOff7 = length;
      length += szName;
      int handleOff7 = length;
      length += handleSize;

      // Create new type
      TypeDef def = types->allocTypeDef(length);
      char* buffer = (char*) def;
      memcpy(&buffer[nameOff7], this->name, szName);
      int nrefs = this->refs.size();
      for(int i=0;i<nrefs;i++) {
        def->refs[i] = this->refs[i].offset;
      }
      def->nrefs = nrefs;
      def->size = this->size;
      def->handle = this->handle;
      def->encoding = this->encoding;
      def->handle = handleSize?(ITypeHandle*)&buffer[handleOff7]:this->handle;
      return def;
    }
    template <typename CTypeHandle>
    TypeDef build() {
      TypeDef def = this->build(sizeof(CTypeHandle));
      def->handle = new (def->handle) CTypeHandle();
      return def;
    }
  };

}
