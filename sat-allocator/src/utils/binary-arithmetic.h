#pragma once
#include "./bitwise.h"

template<int alignment, typename T>
inline T align(T offset) {return ((-offset)&(alignment-1))+offset;}

template<typename T>
inline T alignX(T offset, int alignment) {return ((-offset)&(alignment-1))+offset;}

inline int align1(int offset) {return offset;}
inline int align2(int offset) {return ((-offset)&1)+offset;}
inline int align4(int offset) {return ((-offset)&3)+offset;}
inline int align8(int offset) {return ((-offset)&7)+offset;}
inline int align16(int offset) {return ((-offset)&15)+offset;}
inline int align32(int offset) {return ((-offset)&31)+offset;}
inline int align64(int offset) {return ((-offset)&63)+offset;}
inline int alignX(int offset, int align) {return ((-offset)&(align-1))+offset;}
