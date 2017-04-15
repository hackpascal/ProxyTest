#include "stdafx.h"

#include "cstringa.h"


// 强制指定为 ANSI

#ifdef UNICODE
#define UNICODE_DEFINED
#undef UNICODE
#endif

#include "cstring_imp.inl"

#ifdef UNICODE_DEFINED
#define UNICODE 1
#endif
