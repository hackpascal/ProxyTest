#include "stdafx.h"

#include "cstringa.h"


// ǿ��ָ��Ϊ ANSI

#ifdef UNICODE
#define UNICODE_DEFINED
#undef UNICODE
#endif

#include "cstring_imp.inl"

#ifdef UNICODE_DEFINED
#define UNICODE 1
#endif
