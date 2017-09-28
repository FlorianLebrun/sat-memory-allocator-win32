
template <typename TEntry>
struct SegmentRef {
  uintptr_t index;
  SegmentRef() {
    this->index = 0;
  }
  SegmentRef(uintptr_t index) {
    this->index = index;
  }
  void operator = (uintptr_t index) {
    this->index = index;
  }
  TEntry* operator ->() {
    return (TEntry*)&g_SATable[index];
  }
  char* operator [] (int offset) {
    return (char*)((this->index << SAT::cSegmentSizeL2) + offset);
  }
};

#define assert_unmanaged(x)

// Unmanaged heap reference
template <typename T>
struct ref {
  T* _ptr;
  __forceinline ref() {
    this->_ptr = 0;
  }
  __forceinline ref(T* _ptr) {
    assert_unmanaged_ptr(_ptr);
    this->_ptr = _ptr;
  }
  __forceinline T* operator -> () {
    return this->_ptr;
  }
  __forceinline T* operator = (T* _ptr) {
    assert_unmanaged_ptr(_ptr);
    return this->_ptr = _ptr;
  }
};

// Managed heap reference
template <typename T>
struct mref {

};

// Cross heap reference
template <typename T>
struct xref {
  T* _ptr;

};
