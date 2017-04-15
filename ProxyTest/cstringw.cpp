#include "stdafx.h"

#include "cstringw.h"

// 强制指定为 UNICODE

#ifndef UNICODE
#define UNICODE_NOT_DEFINED
#define UNICODE
#endif

#include "cstring_imp.inl"

#ifdef UNICODE_NOT_DEFINED
#undef UNICODE
#endif

