#pragma once

template<int alignment, typename T>
inline T align(T offset) {return ((-offset)&(alignment-1))+offset;}

template<typename T>
inline T alignX(T offset, int alignment) {return ((-offset)&(alignment-1))+offset;}

inline int alignX(int offset, int align) {return ((-offset)&(align-1))+offset;}

inline __int64 alignX(__int64 offset, __int64 align) { return ((-offset) & (align - 1)) + offset; }
