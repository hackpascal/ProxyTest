#pragma once

#include <Windows.h>

// ǿ��ָ��Ϊ ANSI

#ifdef UNICODE
#define UNICODE_DEFINED
#undef UNICODE
#endif

#include "cstring.inl"

#ifdef UNICODE_DEFINED
#define UNICODE 1
#endif
