#include "stdafx.h"

#include "cstringw.h"

// ǿ��ָ��Ϊ UNICODE

#ifndef UNICODE
#define UNICODE_NOT_DEFINED
#define UNICODE
#endif

#include "cstring_imp.inl"

#ifdef UNICODE_NOT_DEFINED
#undef UNICODE
#endif

