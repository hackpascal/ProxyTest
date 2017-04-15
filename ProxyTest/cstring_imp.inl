#undef LPTSTR
#undef LPCTSTR
#undef TCHAR
#undef _T
#undef _tcslen
#undef _tcscpy_s
#undef _tcsncpy_s
#undef _tcscat_s
#undef _tcscmp
#undef _tcsicmp
#undef _tcsncmp
#undef _tcslwr_s
#undef _tcsupr_s
#undef _tcsstr
#undef _tcschr
#undef _tcsrchr
#undef _vsctprintf
#undef _vstprintf_s

#ifdef UNICODE
#define CStringT	CStringW
#define	STRING_RECORD	STRING_RECORDW
#define	PSTRING_RECORD	PSTRING_RECORDW
#define LPTSTR	LPWSTR
#define LPCTSTR	LPCWSTR
#define TCHAR WCHAR
#define _T(_x)	L ## _x
#define _tcslen	wcslen
#define _tcscpy_s	wcscpy_s
#define _tcsncpy_s	wcsncpy_s
#define _tcscat_s	wcscat_s
#define _tcscmp	wcscmp
#define _tcsicmp	_wcsicmp
#define _tcsncmp	wcsncmp
#define _tcslwr_s	_wcslwr_s
#define _tcsupr_s	_wcsupr_s
#define _tcsstr	wcsstr
#define _tcschr	wcschr
#define _tcsrchr	wcsrchr
#define _vsctprintf	_vscwprintf
#define _vstprintf_s vswprintf_s
#else
#define CStringT	CStringA
#define	STRING_RECORD	STRING_RECORDA
#define	PSTRING_RECORD	PSTRING_RECORDA
#define LPTSTR	LPSTR
#define LPCTSTR	LPCSTR
#define TCHAR CHAR
#define _T(_x)	_x
#define _tcslen	strlen
#define _tcscpy_s	strcpy_s
#define _tcsncpy_s	strncpy_s
#define _tcscat_s	strcat_s
#define _tcscmp	strcmp
#define _tcsicmp	_stricmp
#define _tcsncmp	strncmp
#define _tcslwr_s	_strlwr_s
#define _tcsupr_s	_strupr_s
#define _tcsstr	strstr
#define _tcschr	strchr
#define _tcsrchr	strrchr
#define _vsctprintf	_vscprintf
#define _vstprintf_s vsprintf_s
#endif

CRITICAL_SECTION CStringT::m_pCriticalSection = {};		// 临界区对象，用于将多线程调用线性化
PSTRING_RECORD CStringT::m_pStrings = NULL;				// 字符串记录表
DWORD CStringT::m_dwStringsCapacity = 0;				// 字符串记录表容量
DWORD CStringT::m_dwMaxPosition = -1;					// 字符串记录表当前使用的最大值
LONG CStringT::m_lInitRef = 0;							// 类初始化计数器

// 以下下划线开头的内联字符串处理函数是为了弥补 CRT 函数的不足

//
//   函数: const char *_strrstr(const char *str1, const char *str2)
//
//   目的: strstr 的逆向查找版
//
inline const char *_strrstr(const char *str1, const char *str2)
{
	size_t s1len = strlen(str1);
	size_t s2len = strlen(str2);
	char *s = const_cast<char *>(str1) + s1len - s2len;

	if (s1len < s2len) return NULL;

	while (s >= str1)
	{
		if (!strncmp(s, str2, s2len))
			return s;
		s--;
	}

	return NULL;
}

//
//   函数: const char *_strrstr(char *str1, const char *str2)
//
//   目的: strstr 的逆向查找版
//
inline char *_strrstr(char *str1, const char *str2)
{
	size_t s1len = strlen(str1);
	size_t s2len = strlen(str2);
	char *s = str1 + s1len - s2len;

	if (s1len < s2len) return NULL;

	while (s >= str1)
	{
		if (!strncmp(s, str2, s2len))
			return s;
		s--;
	}

	return NULL;
}

//
//   函数: const char *_strirstr(const char *str1, const char *str2)
//
//   目的: strstr 的逆向查找并忽略大小写版
//
inline const char *_strirstr(const char *str1, const char *str2)
{
	size_t s1len = strlen(str1);
	size_t s2len = strlen(str2);
	char *s = const_cast<char *>(str1) + s1len - s2len;

	if (s1len < s2len) return NULL;

	while (s >= str1)
	{
		if (!_strnicmp(s, str2, s2len))
			return s;
		s--;
	}

	return NULL;
}

//
//   函数: char *_strirstr(char *str1, const char *str2)
//
//   目的: strstr 的逆向查找并忽略大小写版
//
inline char *_strirstr(char *str1, const char *str2)
{
	size_t s1len = strlen(str1);
	size_t s2len = strlen(str2);
	char *s = str1 + s1len - s2len;

	if (s1len < s2len) return NULL;

	while (s >= str1)
	{
		if (!_strnicmp(s, str2, s2len))
			return s;
		s--;
	}

	return NULL;
}

//
//   函数: const char *_stristr(const char *str1, const char *str2)
//
//   目的: strstr 的忽略大小写版
//
inline const char *_stristr(const char *str1, const char *str2)
{
	size_t s1len = strlen(str1);
	size_t s2len = strlen(str2);
	char *s = const_cast<char *>(str1);
	char *m = s + s1len - s2len;

	if (s1len < s2len) return NULL;

	while (s <= m)
	{
		if (!_strnicmp(s, str2, s2len))
			return s;
		s++;
	}

	return NULL;
}

//
//   函数: char *_stristr(char *str1, const char *str2)
//
//   目的: strstr 的忽略大小写版
//
inline char *_stristr(char *str1, const char *str2)
{
	size_t s1len = strlen(str1);
	size_t s2len = strlen(str2);
	char *s = str1;
	char *m = s + s1len - s2len;

	if (s1len < s2len) return NULL;

	while (s <= m)
	{
		if (!_strnicmp(s, str2, s2len))
			return s;
		s++;
	}

	return NULL;
}

//
//   函数: const wchar_t *_wcsrstr(const wchar_t *str1, const wchar_t *str2)
//
//   目的: wcsstr 的逆向查找版
//
inline const wchar_t *_wcsrstr(const wchar_t *str1, const wchar_t *str2)
{
	size_t s1len = wcslen(str1);
	size_t s2len = wcslen(str2);
	wchar_t *s = const_cast<wchar_t *>(str1) + s1len - s2len;

	if (s1len < s2len) return NULL;

	while (s >= str1)
	{
		if (!wcsncmp(s, str2, s2len))
			return s;
		s--;
	}

	return NULL;
}

//
//   函数: wchar_t *_wcsrstr(wchar_t *str1, const wchar_t *str2)
//
//   目的: wcsstr 的逆向查找版
//
inline wchar_t *_wcsrstr(wchar_t *str1, const wchar_t *str2)
{
	size_t s1len = wcslen(str1);
	size_t s2len = wcslen(str2);
	wchar_t *s = str1 + s1len - s2len;

	if (s1len < s2len) return NULL;

	while (s >= str1)
	{
		if (!wcsncmp(s, str2, s2len))
			return s;
		s--;
	}

	return NULL;
}

//
//   函数: const wchar_t *_wcsirstr(const wchar_t *str1, const wchar_t *str2)
//
//   目的: wcsstr 的逆向查找并忽略大小写版
//
inline const wchar_t *_wcsirstr(const wchar_t *str1, const wchar_t *str2)
{
	size_t s1len = wcslen(str1);
	size_t s2len = wcslen(str2);
	wchar_t *s = const_cast<wchar_t *>(str1) + s1len - s2len;

	if (s1len < s2len) return NULL;

	while (s >= str1)
	{
		if (!_wcsnicmp(s, str2, s2len))
			return s;
		s--;
	}

	return NULL;
}

//
//   函数: wchar_t *_wcsirstr(wchar_t *str1, const wchar_t *str2)
//
//   目的: wcsstr 的逆向查找并忽略大小写版
//
inline wchar_t *_wcsirstr(wchar_t *str1, const wchar_t *str2)
{
	size_t s1len = wcslen(str1);
	size_t s2len = wcslen(str2);
	wchar_t *s = str1 + s1len - s2len;

	if (s1len < s2len) return NULL;

	while (s >= str1)
	{
		if (!_wcsnicmp(s, str2, s2len))
			return s;
		s--;
	}

	return NULL;
}

//
//   函数: const wchar_t *_wcsistr(const wchar_t *str1, const wchar_t *str2)
//
//   目的: wcsstr 的忽略大小写版
//
inline const wchar_t *_wcsistr(const wchar_t *str1, const wchar_t *str2)
{
	size_t s1len = wcslen(str1);
	size_t s2len = wcslen(str2);
	wchar_t *s = const_cast<wchar_t *>(str1);
	wchar_t *m = s + s1len - s2len;

	if (s1len < s2len) return NULL;

	while (s <= m)
	{
		if (!_wcsnicmp(s, str2, s2len))
			return s;
		s++;
	}

	return NULL;
}

//
//   函数: wchar_t *_wcsistr(wchar_t *str1, const wchar_t *str2)
//
//   目的: wcsstr 的忽略大小写版
//
inline wchar_t *_wcsistr(wchar_t *str1, const wchar_t *str2)
{
	size_t s1len = wcslen(str1);
	size_t s2len = wcslen(str2);
	wchar_t *s = str1;
	wchar_t *m = s + s1len - s2len;

	if (s1len < s2len) return NULL;

	while (s <= m)
	{
		if (!_wcsnicmp(s, str2, s2len))
			return s;
		s++;
	}

	return NULL;
}

//
//   函数: const char *_strichr(const char *str1, const char s2)
//
//   目的: strchr 的忽略大小写版
//
inline const char *_strichr(const char *str1, const char s2)
{
	char *s = const_cast<char *>(str1);

	while(*s)
	{
		if (_tolower(*s) == _tolower(s2))
			return s;
		s++;
	}

	return NULL;
}

//
//   函数: char *_strichr(char *str1, const char s2)
//
//   目的: strchr 的忽略大小写版
//
inline char *_strichr(char *str1, const char s2)
{
	char *s = str1;

	while(*s)
	{
		if (_tolower(*s) == _tolower(s2))
			return s;
		s++;
	}

	return NULL;
}

//
//   函数: const char *_strirchr(const char *str1, const char s2)
//
//   目的: strchr 的逆向查找并忽略大小写版
//
inline const char *_strirchr(const char *str1, const char s2)
{
	char *s = const_cast<char *>(str1) + strlen(str1) - 1;

	while(s >= str1)
	{
		if (_tolower(*s) == _tolower(s2))
			return s;
		s--;
	}

	return NULL;
}

//
//   函数: char *_strirchr(char *str1, const char s2)
//
//   目的: strchr 的逆向查找并忽略大小写版
//
inline char *_strirchr(char *str1, const char s2)
{
	char *s = str1 + strlen(str1) - 1;

	while(s >= str1)
	{
		if (_tolower(*s) == _tolower(s2))
			return s;
		s--;
	}

	return NULL;
}

//
//   函数: const wchar_t *_wcsichr(const wchar_t *str1, const wchar_t s2)
//
//   目的: wcschr 的忽略大小写版
//
inline const wchar_t *_wcsichr(const wchar_t *str1, const wchar_t s2)
{
	wchar_t *s = const_cast<wchar_t *>(str1);

	while(*s)
	{
		if (_tolower(*s) == _tolower(s2))
			return s;
		s++;
	}

	return NULL;
}

//
//   函数: wchar_t *_wcsichr(wchar_t *str1, const wchar_t s2)
//
//   目的: wcschr 的忽略大小写版
//
inline wchar_t *_wcsichr(wchar_t *str1, const wchar_t s2)
{
	wchar_t *s = str1;

	while(*s)
	{
		if (_tolower(*s) == _tolower(s2))
			return s;
		s++;
	}

	return NULL;
}

//
//   函数: wchar_t *_wcsichr(wchar_t *str1, const wchar_t s2)
//
//   目的: wcschr 的逆向查找并忽略大小写版
//
inline const wchar_t *_wcsirchr(const wchar_t *str1, const wchar_t s2)
{
	wchar_t *s = const_cast<wchar_t *>(str1) + wcslen(str1) - 1;

	while(s >= str1)
	{
		if (_tolower(*s) == _tolower(s2))
			return s;
		s--;
	}

	return NULL;
}

//
//   函数: wchar_t *_wcsirchr(wchar_t *str1, const wchar_t s2)
//
//   目的: wcschr 的逆向查找并忽略大小写版
//
inline wchar_t *_wcsirchr(wchar_t *str1, const wchar_t s2)
{
	wchar_t *s = str1 + wcslen(str1) - 1;

	while(s >= str1)
	{
		if (_tolower(*s) == _tolower(s2))
			return s;
		s--;
	}

	return NULL;
}


#ifdef UNICODE
#define _tcsrstr	_wcsrstr
#define _tcsistr	_wcsistr
#define _tcsirstr	_wcsirstr
#define _tcsristr	_wcsirstr
#define _tcsichr	_wcsichr
#define _tcsirchr	_wcsirchr
#define _tcsrichr	_wcsirchr
#else
#define _tcsrstr	_strrstr
#define _tcsistr	_stristr
#define _tcsirstr	_strirstr
#define _tcsristr	_strirstr
#define _tcsichr	_strichr
#define _tcsirchr	_strirchr
#define _tcsrichr	_strirchr
#endif

//
//   函数: void CStringT::CommonConstructor()
//
//   目的: 初始化字符串类
//
//   注释: 若为模块内的第一次初始化，则同时初始化字符串记录表
//
void CStringT::CommonConstructor()
{
	// InterlockedIncrement 通过指针增加初始化次数的值，但会使线程并行操作线性化
	if (InterlockedIncrement(&m_lInitRef) == 1)
	{
		// 初始化临界区
		InitializeCriticalSection(&m_pCriticalSection);

		__try
		{
			// 锁定线程，以防止其他字符串类在初始化后访问未经初始化的字符串记录表
			Lock();

			// 设置字符串记录表初始容量
			m_dwStringsCapacity = STRINGS_CAP_INCREASEMENT;
			m_pStrings = (PSTRING_RECORD) malloc(m_dwStringsCapacity * sizeof (STRING_RECORD));
			ZeroMemory(m_pStrings, m_dwStringsCapacity * sizeof (STRING_RECORD));
			
			// 初始化第一个字符串记录，一个空字符串
			// 这个字符串将作为以后所有新初始化的字符串类的默认字符串
			// 且在模块生命周期内都不会被释放
			m_dwMaxPosition = 0;
			m_pStrings[0].dwRefCount = 1;
			m_pStrings[0].dwCapacity = STRING_MIN_ALIGN_SIZE;
			m_pStrings[0].lpBuffer = (LPTSTR) malloc(m_pStrings[0].dwCapacity * sizeof(TCHAR));
			m_pStrings[0].lpBuffer[0] = '\0';
		}
		__finally
		{
			// 解锁
			Unlock();
		}
	}

	// 设置默认字符串
	m_dwStringId = 0;
	m_lpStrConv = NULL;
	m_cchStrConv = 0;
	m_bStrConvModified = TRUE;
}

//
//   函数: CStringT::CStringT()
//
//   目的: 默认构造函数
//
CStringT::CStringT()
{
	CommonConstructor();
}

//
//   函数: CStringT::~CStringT()
//
//   目的: 析构函数
//
//   注释: 若析构此类后再无其他类使用，则同时销毁字符串记录表
//
CStringT::~CStringT()
{
	// 初始化次数减一
	if (InterlockedDecrement(&m_lInitRef) == 0)
	{
		__try
		{
			Lock();

			// 销毁字符串记录表并将其设置为初始状态
			for (DWORD i = 0; i < m_dwStringsCapacity; i++)
			{
				if (m_pStrings[i].lpBuffer) DestroyStringRecord(m_pStrings[i]);
			}

			free(m_pStrings);

			m_dwMaxPosition = -1;
			m_dwStringsCapacity = 0;
		}
		__finally
		{
			Unlock();
		}

		// 销毁临界区
		DeleteCriticalSection(&m_pCriticalSection);
	}
	else
	{
		// 当前使用的字符串记录引用次数减一
		if (m_dwStringId != 0) ReleaseReferenceCount(m_dwStringId);
	}

	// 释放类型转换用的临时存储
	if (m_lpStrConv) free(m_lpStrConv);
}

//
//   函数: void CStringT::DestroyStringRecord(STRING_RECORD &pRecord)
//
//   目的: 销毁字符串记录并将其设为未使用状态
//
void CStringT::DestroyStringRecord(STRING_RECORD &pRecord)
{
	if (pRecord.lpBuffer) free(pRecord.lpBuffer);
	pRecord.lpBuffer = NULL;
	pRecord.dwCapacity = 0;
	pRecord.dwRefCount = 0;
	pRecord.bFakeRef = FALSE;
}

//
//   函数: DWORD CStringT::AllocateRecord()
//
//   目的: 分配字符串记录，并设置默认字符串（空串）
//
DWORD CStringT::AllocateRecord()
{
	DWORD dwId = 0;
	DWORD dwOldCapacity;

	__try
	{
		// 查找字符串记录表内有无未使用的记录
		for (DWORD i = 0; i < m_dwStringsCapacity; i++)
		{
			if (!m_pStrings[i].dwRefCount)
			{
				m_pStrings[i].dwRefCount = 1;
				m_pStrings[i].bFakeRef = FALSE;
				if (!m_pStrings[i].lpBuffer)
				{
					m_pStrings[i].dwCapacity = STRING_MIN_ALIGN_SIZE;
					m_pStrings[i].lpBuffer = (LPTSTR) malloc(m_pStrings[i].dwCapacity * sizeof (TCHAR));
				}
				m_pStrings[i].lpBuffer[0] = '\0';

				dwId = i;
				if (dwId > m_dwMaxPosition) m_dwMaxPosition = dwId;
				__leave;
			}
		}

		// 没有可用的记录，则需给字符串记录表扩容
		dwOldCapacity = m_dwStringsCapacity;
		m_dwStringsCapacity += STRINGS_CAP_INCREASEMENT;

		m_pStrings = (PSTRING_RECORD) realloc(m_pStrings, m_dwStringsCapacity * sizeof(STRING_RECORD));
		ZeroMemory(&m_pStrings[dwOldCapacity], (m_dwStringsCapacity - dwOldCapacity) * sizeof(STRING_RECORD));

		dwId = dwOldCapacity;
		m_pStrings[dwId].dwCapacity = STRING_MIN_ALIGN_SIZE;
		m_pStrings[dwId].lpBuffer = (LPTSTR) malloc(m_pStrings[dwId].dwCapacity * sizeof (TCHAR));
		m_pStrings[dwId].lpBuffer[0] = '\0';
		m_pStrings[dwId].dwRefCount = 1;
		m_dwMaxPosition = dwId;
	}
	__finally
	{

	}

	return dwId;
}

//
//   函数: DWORD CStringT::AllocateRecord(LPCTSTR lpString, int cchSize)
//
//   目的: 分配字符串记录，并设字符串内容。cchSize 指定要复制字符串的长度
//
DWORD CStringT::AllocateRecord(LPCTSTR lpString, int cchSize)
{
	DWORD dwId = AllocateRecord();

	// 设置内容
	SetRecordString(m_pStrings[dwId], lpString, cchSize);

	return dwId;
}

//
//   函数: void CStringT::SetRecordCapacity(STRING_RECORD &pRecord, DWORD cchSize)
//
//   目的: 设置字符串记录容量
//
void CStringT::SetRecordCapacity(STRING_RECORD &pRecord, DWORD cchSize)
{
	if (pRecord.dwCapacity < cchSize)
	{
		pRecord.dwCapacity = AlignSize(cchSize);

		if (!pRecord.lpBuffer)
			pRecord.lpBuffer = (LPTSTR) malloc(pRecord.dwCapacity * sizeof (TCHAR));
		else
			pRecord.lpBuffer = (LPTSTR) realloc(pRecord.lpBuffer, pRecord.dwCapacity * sizeof (TCHAR));

		pRecord.lpBuffer[cchSize - 1] = '\0';
	}
}

//
//   函数: void CStringT::SetRecordCapacity(DWORD dwRecordId, DWORD cchSize)
//
//   目的: 通过 ID 设置字符串记录容量
//
void CStringT::SetRecordCapacity(DWORD dwRecordId, DWORD cchSize)
{
	if (dwRecordId >= m_dwStringsCapacity && !m_pStrings[dwRecordId].dwRefCount) return;

	SetRecordCapacity(m_pStrings[dwRecordId], cchSize);

}

//
//   函数: BOOL CStringT::SetRecordString(STRING_RECORD &pRecord, LPCTSTR lpString, int cchSize)
//
//   目的: 设置字符串记录值。cchSize 指定要复制字符串的长度
//
BOOL CStringT::SetRecordString(STRING_RECORD &pRecord, LPCTSTR lpString, int cchSize)
{
	BOOL bResult = FALSE;

	// 若字符串地址为 NULL，则将其视作空串
	if (lpString)
	{
		if (cchSize == -1) cchSize = _tcslen(lpString);
	}else
	{
		cchSize = 0;
	}

	// 重新设置容量
	SetRecordCapacity(pRecord, cchSize + 1);
		
	if (lpString)
		_tcscpy_s(pRecord.lpBuffer, pRecord.dwCapacity, lpString);
	else
		pRecord.lpBuffer[0] = '\0';

	bResult = TRUE;

	return bResult;
}

//
//   函数: BOOL CStringT::SetRecordString(DWORD dwRecordId, LPCTSTR lpString, int cchSize)
//
//   目的: 通过 ID 设置字符串记录值。cchSize 指定要复制字符串的长度
//
BOOL CStringT::SetRecordString(DWORD dwRecordId, LPCTSTR lpString, int cchSize)
{
	BOOL bResult = FALSE;

	if (dwRecordId >= m_dwStringsCapacity && !m_pStrings[dwRecordId].dwRefCount) return FALSE;

	bResult = SetRecordString(m_pStrings[dwRecordId], lpString, cchSize);

	return bResult;
}

//
//   函数: void CStringT::AddReferenceCount(STRING_RECORD &pRecord)
//
//   目的: 增加字符串记录引用次数
//
void CStringT::AddReferenceCount(STRING_RECORD &pRecord)
{
	if (pRecord.dwRefCount && !pRecord.bFakeRef) pRecord.dwRefCount++;
	pRecord.bFakeRef = FALSE;
}

//
//   函数: void CStringT::AddReferenceCount(DWORD dwRecordId)
//
//   目的: 通过 ID 增加字符串记录引用次数
//
void CStringT::AddReferenceCount(DWORD dwRecordId)
{
	if ((dwRecordId >= m_dwStringsCapacity) || (dwRecordId == 0)) return;

	AddReferenceCount(m_pStrings[dwRecordId]);
}

//
//   函数: void CStringT::ReleaseReferenceCount(STRING_RECORD &pRecord)
//
//   目的: 减少字符串记录引用次数
//
//   注释: 若字符串记录引用次数减为零，则销毁此字符串记录
//
void CStringT::ReleaseReferenceCount(STRING_RECORD &pRecord)
{
	if (pRecord.dwRefCount) pRecord.dwRefCount--;

	if (!pRecord.dwRefCount)
		DestroyStringRecord(pRecord);
}

//
//   函数: void CStringT::ReleaseReferenceCount(DWORD dwRecordId)
//
//   目的: 通过 ID 减少字符串记录引用次数
//
void CStringT::ReleaseReferenceCount(DWORD dwRecordId)
{
	if ((dwRecordId >= m_dwStringsCapacity ) || (dwRecordId == 0)) return;

	ReleaseReferenceCount(m_pStrings[dwRecordId]);

	if (dwRecordId == m_dwMaxPosition) m_dwMaxPosition--;
}

//
//   函数: DWORD CStringT::FindRecord(LPCTSTR lpAddress)
//
//   目的: 在字符串记录中查找字符串地址，以避免多次复制同一字符串
//
DWORD CStringT::FindRecord(LPCTSTR lpAddress)
{
	DWORD dwId = STRING_ALLOCATE_FAILURE;

	for (DWORD i = 0; i <= m_dwMaxPosition; i++)
	{
		if (m_pStrings[i].dwRefCount)
			if (m_pStrings[i].lpBuffer == lpAddress)
			{
				dwId = i;
				break;
			}
	}

	return dwId;
}

//
//   函数: void CStringT::Lock()
//
//   目的: 线程锁定，以阻止其他使用此字符串记录表的操作同时进行
//
//   注释: 必须与 Unlock() 配对使用，否则会造成线程死锁
//
void CStringT::Lock()
{
	EnterCriticalSection(&m_pCriticalSection);
}

//
//   函数: void CStringT::Unlock()
//
//   目的: 线程解锁
//
void CStringT::Unlock()
{
	LeaveCriticalSection(&m_pCriticalSection);
}

//
//   函数: LPCVOID CStringT::RefAddress() const
//
//   目的: 返回字符串记录表的起始地址，用于判断两个字符串类是否使用同一个字符串记录表
//
LPCVOID CStringT::RefAddress() const
{
	return &m_pStrings;
}




//
//   函数: CStringT::CStringT(const CStringT &s)
//
//   目的: 构造拷贝函数
//
CStringT::CStringT(const CStringT &s)
{
	CommonConstructor();
	ConstructCopy(const_cast<CStringT *>(&s)->RefAddress(), const_cast<CStringT *>(&s)->StringBuffer(), const_cast<CStringT *>(&s)->GetStringId());
}

//
//   函数: CStringT::CStringT(LPCWSTR lpString, int cchSize)
//
//   目的: 构造并赋值，Unicode 字符串，可指定长度
//
CStringT::CStringT(LPCWSTR lpString, int cchSize)
{
	CommonConstructor();
	SetString(lpString, cchSize);
}

//
//   函数: CStringT::CStringT(LPCWSTR lpString)
//
//   目的: 构造并赋值，Unicode 字符串
//
CStringT::CStringT(LPCWSTR lpString)
{
	CommonConstructor();
	SetString(lpString);
}

//
//   函数: CStringT::CStringT(LPCSTR lpString, int cchSize)
//
//   目的: 构造并赋值，ANSI 字符串，可指定长度
//
CStringT::CStringT(LPCSTR lpString, int cchSize)
{
	CommonConstructor();
	SetString(lpString, cchSize);
}

//
//   函数: CStringT::CStringT(LPCSTR lpString)
//
//   目的: 构造并赋值，ANSI 字符串
//
CStringT::CStringT(LPCSTR lpString)
{
	CommonConstructor();
	SetString(lpString);
}

//
//   函数: CStringT::CStringT(const int Ch, size_t nCount)
//
//   目的: 构造并填充字符
//
CStringT::CStringT(const int Ch, size_t nCount)
{
	CommonConstructor();
	Fill(Ch, nCount);
}

//
//   函数: CStringT::CStringT(const int Ch)
//
//   目的: 构造并将字符串值设置为一个字符
//
CStringT::CStringT(const int Ch)
{
	CommonConstructor();
	Fill(Ch, 1);
}

//
//   函数: LPWSTR CStringT::ToWide(LPCSTR lpString)
//
//   目的: 将 ANSI 字符串转换为 Unicode 字符串
//
//   注释: 需释放返回字符串
//
LPWSTR CStringT::ToWide(LPCSTR lpString)
{
	int cchSize;
	LPWSTR lpResult;

	if (!lpString) return NULL;

	cchSize = MultiByteToWideChar(CP_ACP, 0, lpString, -1, NULL, 0);
	lpResult = (LPWSTR) malloc(cchSize * sizeof (WCHAR));
	MultiByteToWideChar(CP_ACP, 0, lpString, -1, lpResult, cchSize);

	return lpResult;
}

//
//   函数: LPSTR CStringT::ToAnsi(LPCWSTR lpString)
//
//   目的: 将 Unicode 字符串转换为 ANSI 字符串
//
//   注释: 需释放返回字符串
//
LPSTR CStringT::ToAnsi(LPCWSTR lpString)
{
	int cchSize;
	LPSTR lpResult;

	if (!lpString) return NULL;

	cchSize = WideCharToMultiByte(CP_ACP, 0, lpString, -1, NULL, 0, NULL, NULL);
	lpResult = (LPSTR) malloc(cchSize * sizeof (CHAR));
	WideCharToMultiByte(CP_ACP, 0, lpString, -1, lpResult, cchSize, NULL, NULL);

	return lpResult;
}

//
//   函数: void CStringT::FreeConv(LPVOID pAddress)
//
//   目的: 释放字符串，并带有 NULL 检查
//
void CStringT::FreeConv(LPVOID pAddress)
{
	if (pAddress) free(pAddress);
}

//
//   函数: DWORD CStringT::GetStringId()
//
//   目的: 返回字符串记录 ID
//
DWORD CStringT::GetStringId()
{
	return m_dwStringId;
}

//
//   函数: LPCTSTR CStringT::StringBuffer()
//
//   目的: 返回字符串缓冲区的起始地址
//
LPCTSTR CStringT::StringBuffer()
{
	return m_pStrings[m_dwStringId].lpBuffer;
}

//
//   函数: void CStringT::Detach()
//
//   目的: 分离字符串记录，使之不被其他字符串类引用
//
//   注释: 若字符串记录被多次引用，则将其复制一份，否则不作任何改动
//
void CStringT::Detach()
{
	DWORD dwOldId;

	__try
	{
		// 所有的改动在进行前都必须进行线程锁定
		Lock();

		// 若不是初始字符串且其引用次数为1，则不作任何操作
		if (m_dwStringId != 0 && m_pStrings[m_dwStringId].dwRefCount == 1)
			__leave;

		dwOldId = m_dwStringId;

		// 复制一份字符串记录
		m_dwStringId = AllocateRecord(m_pStrings[dwOldId].lpBuffer);
		// 减少之前的字符串记录引用次数
		ReleaseReferenceCount(dwOldId);
	}
	__finally
	{
		Unlock();
	}
}

//
//   函数: TCHAR CStringT::GetChar(DWORD nIndex)
//
//   目的: 获取字符串的某个字符
//
TCHAR CStringT::GetChar(DWORD nIndex)
{
	if (nIndex >= m_pStrings[m_dwStringId].dwCapacity)
		return '\0';

	return m_pStrings[m_dwStringId].lpBuffer[nIndex];
}

//
//   函数: void CStringT::SetChar(DWORD nIndex, TCHAR cNewChar)
//
//   目的: 设置字符串的某个字符
//
void CStringT::SetChar(DWORD nIndex, TCHAR cNewChar)
{
	__try
	{
		Lock();

		// 下标越界检查
		if (nIndex >= m_pStrings[m_dwStringId].dwCapacity)
			__leave;

		// 若修改的和之前的字符相同，则不作处理
		if (cNewChar == m_pStrings[m_dwStringId].lpBuffer[nIndex])
			__leave;

		Detach();

		m_pStrings[m_dwStringId].lpBuffer[nIndex] = cNewChar;
	}
	__finally
	{
		Unlock();
	}

	m_bStrConvModified = TRUE;
}

//
//   函数: DWORD CStringT::GetLength()
//
//   目的: 获取字符串长度
//
DWORD CStringT::GetLength()
{
	return _tcslen(m_pStrings[m_dwStringId].lpBuffer);
}

//
//   函数: DWORD CStringT::GetCapacity()
//
//   目的: 获取字符串缓冲区容量
//
DWORD CStringT::GetCapacity()
{
	return m_pStrings[m_dwStringId].dwCapacity;
}

//
//   函数: DWORD CStringT::GetCapacity()
//
//   目的: 设置字符串缓冲区容量
//
void CStringT::SetCapacity(DWORD dwLen)
{
	__try
	{
		Lock();

		// 若此字符串记录未被其他字符串类引用，且字符串记录容量未改变，则不作任何操作
		if ((AlignSize(dwLen) == m_pStrings[m_dwStringId].dwCapacity) && (m_pStrings[m_dwStringId].dwRefCount == 1))
			__leave;

		Detach();

		SetRecordCapacity(m_dwStringId, dwLen);
	}
	__finally
	{
		Unlock();
	}

	m_bStrConvModified = TRUE;
}

//
//   函数: void CStringT::Reverse()
//
//   目的: 反转字符串
//
void CStringT::Reverse()
{
	LPTSTR lpBuffer;
	DWORD cchLen;
	TCHAR cTemp;

	__try
	{
		Lock();

		// 字符串长度小于2，则不需作任何改动
		if (_tcslen(m_pStrings[m_dwStringId].lpBuffer) < 2)
			__leave;

		Detach();

		lpBuffer = m_pStrings[m_dwStringId].lpBuffer;
		cchLen = _tcslen(lpBuffer);

		for (DWORD i = 0; i < cchLen / 2; i++)
		{
			cTemp = lpBuffer[i];
			lpBuffer[i] = lpBuffer[cchLen - i - 1];
			lpBuffer[cchLen - i - 1] = cTemp;
		}
	}
	__finally
	{
		Unlock();
	}

	m_bStrConvModified = TRUE;
}

//
//   函数: void CStringT::ToLowerCase()
//
//   目的: 将字符串转换为小写
//
void CStringT::ToLowerCase()
{
	__try
	{
		Lock();

		Detach();

		_tcslwr_s(m_pStrings[m_dwStringId].lpBuffer, _tcslen(m_pStrings[m_dwStringId].lpBuffer) + 1);
	}
	__finally
	{
		Unlock();
	}

	m_bStrConvModified = TRUE;
}

//
//   函数: void CStringT::ToUpperCase()
//
//   目的: 将字符串转换为大写
//
void CStringT::ToUpperCase()
{
	__try
	{
		Lock();

		Detach();

		_tcsupr_s(m_pStrings[m_dwStringId].lpBuffer, _tcslen(m_pStrings[m_dwStringId].lpBuffer) + 1);
	}
	__finally
	{
		Unlock();
	}

	m_bStrConvModified = TRUE;
}

//
//   函数: void CStringT::FillImpl(LPCTSTR lpString, DWORD nCount)
//
//   目的: 字符串填充的实现
//
void CStringT::FillImpl(LPCTSTR lpString, DWORD nCount)
{
	if (!lpString || *lpString == '\0' || !nCount) return;

	__try
	{
		Lock();

		Detach();

		// 将字符串设置为空串
		m_pStrings[m_dwStringId].lpBuffer[0] = '\0';

		// 计算空间大小
		SetRecordCapacity(m_dwStringId, _tcslen(lpString) * nCount + 1);

		// 调用 strcat 函数进行填充
		for (size_t i = 0; i < nCount; i++)
			_tcscat_s(m_pStrings[m_dwStringId].lpBuffer, m_pStrings[m_dwStringId].dwCapacity, lpString);
	}
	__finally
	{
		Unlock();
	}

	m_bStrConvModified = TRUE;
}

void CStringT::Fill(LPCWSTR lpString, DWORD nCount)
{
#ifdef UNICODE
	FillImpl(lpString, nCount);
#else
	LPTSTR pTemp = ToAnsi(lpString);

	FillImpl(pTemp, nCount);

	FreeConv(pTemp);
#endif
}


void CStringT::Fill(LPCSTR lpString, DWORD nCount)
{
#ifdef UNICODE
	LPTSTR pTemp = ToWide(lpString);

	FillImpl(pTemp, nCount);

	FreeConv(pTemp);
#else
	FillImpl(lpString, nCount);
#endif
}

//
//   函数: void CStringT::Fill(const int Ch, DWORD nCount)
//
//   目的: 用字符填充字符串
//
void CStringT::Fill(const int Ch, DWORD nCount)
{
	TCHAR c;

	if (!Ch || !nCount) return;

	__try
	{
		Lock();

		Detach();

#ifdef UNICODE
		c = Ch & 0xFFFF;
#else
		c = Ch & 0xFF;
#endif

		SetRecordCapacity(m_dwStringId, nCount + 1);

		for (size_t i = 0; i < nCount; i++)
			m_pStrings[m_dwStringId].lpBuffer[i] = c;

		m_pStrings[m_dwStringId].lpBuffer[nCount] = '\0';
	}
	__finally
	{
		Unlock();
	}

	m_bStrConvModified = TRUE;
}

//
//   函数: CStringT &CStringT::ConstructCopy(LPCVOID lpRefAddr, LPCTSTR lpString, DWORD dwStringId)
//
//   目的: 构造拷贝（类拷贝）函数
//
CStringT &CStringT::ConstructCopy(LPCVOID lpRefAddr, LPCTSTR lpString, DWORD dwStringId)
{
	__try
	{
		Lock();

		// 解除当前字符串记录的引用
		ReleaseReferenceCount(m_dwStringId);

		// 两个字符串类是否使用同一个字符串记录表
		if (RefAddress() != lpRefAddr)
		{
			// 未使用相同的字符串记录表，则新建一个字符串记录
			m_dwStringId = AllocateRecord(lpString);
		}
		else
		{
			// 否则直接增加字符串记录的引用次数
			m_dwStringId = dwStringId;
			AddReferenceCount(m_dwStringId);
		}
	}
	__finally
	{
		Unlock();
	}

	m_bStrConvModified = TRUE;

	return *this;
}

//
//   函数: int CStringT::CompareImpl(LPCTSTR lpString, BOOL bIgnoreCase)
//
//   目的: 字符串比较函数的实现，bIgnoreCase 确定是否区分大小写
//
int CStringT::CompareImpl(LPCTSTR lpString, BOOL bIgnoreCase)
{
	int bResult;

	if (!lpString) lpString = _T("");

	if (bIgnoreCase)
		bResult = _tcsicmp(m_pStrings[m_dwStringId].lpBuffer, lpString);
	else
		bResult = _tcscmp(m_pStrings[m_dwStringId].lpBuffer, lpString);

	return bResult;
}

int CStringT::Compare(LPCWSTR lpString, BOOL bIgnoreCase)
{
	int bResult;
#ifdef UNICODE
	bResult = CompareImpl(lpString, bIgnoreCase);
#else
	LPTSTR pTemp = ToAnsi(lpString);

	bResult = CompareImpl(pTemp, bIgnoreCase);

	FreeConv(pTemp);
#endif

	return bResult;
}

int CStringT::Compare(LPCSTR lpString, BOOL bIgnoreCase)
{
	int bResult;
#ifdef UNICODE
	LPTSTR pTemp = ToWide(lpString);

	bResult = CompareImpl(pTemp, bIgnoreCase);

	FreeConv(pTemp);
#else
	bResult = CompareImpl(lpString, bIgnoreCase);
#endif

	return bResult;
}

int CStringT::Compare(const int Ch, BOOL bIgnoreCase)
{
	TCHAR c[2];
#ifdef UNICODE
	c[0] = Ch & 0xFFFF;
#else
	c[0] = Ch & 0xFF;
#endif
	c[1] = '\0';

	return CompareImpl((LPTSTR) c, bIgnoreCase);
}

//
//   函数: CStringT &CStringT::SetString(LPCWSTR lpString, int cchSize)
//
//   目的: 设置字符串的值，Unicode 字符串
//
CStringT &CStringT::SetString(LPCWSTR lpString, int cchSize)
{
	__try
	{
		Lock();

		ReleaseReferenceCount(m_dwStringId);

		if (!lpString)
		{
			m_dwStringId = 0;
			__leave;
		}

#ifdef UNICODE
		// 若字符串类型对于工程的字符串类型
		m_dwStringId = FindRecord(lpString);
		if (m_dwStringId != STRING_ALLOCATE_FAILURE && ((size_t) cchSize >= _tcslen(m_pStrings[m_dwStringId].lpBuffer) || cchSize < 0))
		{
			AddReferenceCount(m_dwStringId);
		}
		else
		{
			m_dwStringId = AllocateRecord(lpString);
			if (cchSize >= 0 && (size_t) cchSize < _tcslen(m_pStrings[m_dwStringId].lpBuffer))
				m_pStrings[m_dwStringId].lpBuffer[cchSize] = '\0';
		}
#else
		// 否则转换后直接添加
		m_dwStringId = AllocateRecord();

		SetRecordCapacity(m_dwStringId, (DWORD) WideCharToMultiByte(CP_ACP, 0, lpString, cchSize, NULL, 0, NULL, NULL));

		WideCharToMultiByte(CP_ACP, 0, lpString, -1, m_pStrings[m_dwStringId].lpBuffer, m_pStrings[m_dwStringId].dwCapacity, NULL, NULL);
#endif
	}
	__finally
	{
		Unlock();
	}

	m_bStrConvModified = TRUE;

	return *this;
}

//
//   函数: CStringT &CStringT::SetString(LPCSTR lpString, int cchSize)
//
//   目的: 设置字符串的值，ANSI 字符串
//
CStringT &CStringT::SetString(LPCSTR lpString, int cchSize)
{
	__try
	{
		Lock();

		ReleaseReferenceCount(m_dwStringId);

		if (!lpString)
		{
			m_dwStringId = 0;
			__leave;
		}

#ifdef UNICODE
		m_dwStringId = AllocateRecord();

		SetRecordCapacity(m_dwStringId, (DWORD) MultiByteToWideChar(CP_ACP, 0, lpString, cchSize, NULL, 0));

		MultiByteToWideChar(CP_ACP, 0, lpString, -1, m_pStrings[m_dwStringId].lpBuffer, m_pStrings[m_dwStringId].dwCapacity);
#else
		m_dwStringId = FindRecord(lpString);
		if (m_dwStringId != STRING_ALLOCATE_FAILURE && (cchSize == _tcslen(m_pStrings[m_dwStringId].lpBuffer) || cchSize < 0))
		{
			AddReferenceCount(m_dwStringId);
		}
		else
		{
			m_dwStringId = AllocateRecord(lpString);
			if (cchSize >= 0 && cchSize < (int) _tcslen(m_pStrings[m_dwStringId].lpBuffer))
				m_pStrings[m_dwStringId].lpBuffer[cchSize] = '\0';
		}
#endif
	}
	__finally
	{
		Unlock();
	}

	m_bStrConvModified = TRUE;

	return *this;
}

CStringT &CStringT::SetString(const int Ch)
{
	TCHAR c[2];
#ifdef UNICODE
	c[0] = Ch & 0xFFFF;
#else
	c[0] = Ch & 0xFF;
#endif
	c[1] = '\0';

	return SetString((LPTSTR) c);
}

//
//   函数: CStringT &CStringT::AppendImpl(LPCTSTR lpString)
//
//   目的: 字符串追加函数的实现
//
CStringT &CStringT::AppendImpl(LPCTSTR lpString)
{
	DWORD cchLen;

	if (*lpString == '\0' || !lpString) return *this;

	__try
	{
		Lock();

		Detach();

		// 计算空间大小
		cchLen = _tcslen(m_pStrings[m_dwStringId].lpBuffer) + _tcslen(lpString) + 1;

		SetRecordCapacity(m_dwStringId, cchLen);

		_tcscat_s(m_pStrings[m_dwStringId].lpBuffer, cchLen, lpString);
	}
	__finally
	{
		Unlock();
	}

	m_bStrConvModified = TRUE;

	return *this;
}

CStringT &CStringT::Append(LPCWSTR lpString)
{
#ifdef UNICODE
	AppendImpl(lpString);
#else
	LPTSTR pTemp = ToAnsi(lpString);

	AppendImpl(pTemp);

	FreeConv(pTemp);
#endif

	return *this;
}

CStringT &CStringT::Append(LPCSTR lpString)
{
#ifdef UNICODE
	LPTSTR pTemp = ToWide(lpString);

	Append(pTemp);

	FreeConv(pTemp);
#else
	AppendImpl(lpString);
#endif

	return *this;
}

CStringT &CStringT::Append(const int Ch)
{
	TCHAR s[2];
#ifdef UNICODE
	s[0] = Ch & 0xFFFF;
#else
	s[0] = Ch & 0xFF;
#endif
	s[1] = '\0';

	return AppendImpl((LPTSTR) s);
}

//
//   函数: LPCTSTR CStringT::ConcatImpl(LPCWSTR lpString)
//
//   目的: 字符串连接函数的实现
//
//   注释: 此操作不会改变本字符串，而是生成一个新字符串，并将其添加到字符串记录表中
//         尽量使用字符串类作为左值来接受此字符串，否则可能会造成内存浪费（不会泄露）
//
LPCTSTR CStringT::ConcatImpl(LPCTSTR lpString)
{
	DWORD dwLen;
	DWORD dwNewId;
	LPTSTR lpResult = NULL;

	__try
	{
		Lock();

		lpResult = m_pStrings[m_dwStringId].lpBuffer;

		if (*lpString == '\0' || !lpString)
			__leave;

		// 计算空间大小
		dwLen = _tcslen(m_pStrings[m_dwStringId].lpBuffer) + _tcslen(lpString) + 1;

		// 申请新的字符串记录
		dwNewId = AllocateRecord();
		SetRecordCapacity(dwNewId, dwLen);

		lpResult = m_pStrings[dwNewId].lpBuffer;

		_tcscpy_s(lpResult, dwLen, m_pStrings[m_dwStringId].lpBuffer);
		_tcscat_s(lpResult, dwLen, lpString);

		// 此标记表明此字符串记录未被任何字符串类引用，但不能被销毁
		// 在新的字符串类引用此字符串记录时不要增加其引用次数
		m_pStrings[dwNewId].bFakeRef = TRUE;
	}
	__finally
	{
		Unlock();
	}

	return lpResult;
}

LPCTSTR CStringT::Concat(LPCWSTR lpString)
{
	LPCTSTR lpResult;
#ifdef UNICODE
	lpResult = ConcatImpl(lpString);
#else
	LPTSTR pTemp = ToAnsi(lpString);

	lpResult = ConcatImpl(pTemp);

	FreeConv(pTemp);
#endif

	return lpResult;
}

LPCTSTR CStringT::Concat(LPCSTR lpString)
{
	LPCTSTR lpResult;
#ifdef UNICODE
	LPWSTR pTemp = ToWide(lpString);

	lpResult = ConcatImpl(pTemp);

	FreeConv(pTemp);
#else
	lpResult = ConcatImpl(lpString);
#endif

	return lpResult;
}

LPCTSTR CStringT::Concat(const int Ch)
{
	TCHAR s[2];
#ifdef UNICODE
	s[0] = Ch & 0xFFFF;
#else
	s[0] = Ch & 0xFF;
#endif
	s[1] = '\0';

	return ConcatImpl((LPTSTR) s);
}

//
//   函数: DWORD CStringT::FindImpl(LPCTSTR lpString, DWORD nPos, BOOL bReverse, BOOL bIgnoreCase)
//
//   目的: 字符串查找函数的实现。nPos 指定起始的查找位置，从 0 开始；bReverse 指定是否逆向查找；
//         bIgnoreCase 指定是否忽略大小写
//
DWORD CStringT::FindImpl(LPCTSTR lpString, DWORD nPos, BOOL bReverse, BOOL bIgnoreCase)
{
	LPTSTR lpHaystack, lpPos;
	DWORD dwResult;

	lpHaystack = m_pStrings[m_dwStringId].lpBuffer + nPos;

	if (!bReverse)
		if (!bIgnoreCase)
			lpPos = _tcsstr(lpHaystack, lpString);
		else
			lpPos = _tcsistr(lpHaystack, lpString);
	else
		if (!bIgnoreCase)
			lpPos = _tcsrstr(lpHaystack, lpString);
		else
			lpPos = _tcsirstr(lpHaystack, lpString);

	if (lpPos == NULL)
		dwResult = (DWORD) -1;
	else
		dwResult = (DWORD) (lpPos - lpHaystack) + nPos;

	return dwResult;
}

DWORD CStringT::Find(LPCWSTR lpString, DWORD nPos, BOOL bReverse, BOOL bIgnoreCase)
{
	DWORD dwResult;

#ifdef UNICODE
	dwResult = FindImpl(lpString, nPos, bReverse, bIgnoreCase);
#else
	LPTSTR pTemp = ToAnsi(lpString);

	dwResult = FindImpl(pTemp, nPos, bReverse, bIgnoreCase);

	FreeConv(pTemp);
#endif

	return dwResult;
}

DWORD CStringT::Find(LPCSTR lpString, DWORD nPos, BOOL bReverse, BOOL bIgnoreCase)
{
	DWORD dwResult;

#ifdef UNICODE
	LPTSTR pTemp = ToWide(lpString);

	dwResult = FindImpl(pTemp, nPos, bReverse, bIgnoreCase);

	FreeConv(pTemp);
#else
	dwResult = FindImpl(lpString, nPos, bReverse, bIgnoreCase);
#endif

	return dwResult;
}

//
//   函数: DWORD CStringT::Find(const int Ch, DWORD nPos, BOOL bReverse, BOOL bIgnoreCase)
//
//   目的: 在字符串中查找字符。nPos 指定起始的查找位置，从 0 开始；bReverse 指定是否逆向查找；
//         bIgnoreCase 指定是否忽略大小写
//
DWORD CStringT::Find(const int Ch, DWORD nPos, BOOL bReverse, BOOL bIgnoreCase)
{
	LPTSTR lpHaystack, lpPos;
	DWORD dwResult;

#ifdef UNICODE
	TCHAR cNeedle = Ch & 0xFFFF;
#else
	TCHAR cNeedle = Ch & 0xFF;
#endif

	lpHaystack = m_pStrings[m_dwStringId].lpBuffer + nPos;

	if (!bReverse)
		if (!bIgnoreCase)
			lpPos = _tcschr(lpHaystack, cNeedle);
		else
			lpPos = _tcsichr(lpHaystack, cNeedle);
	else
		if (!bIgnoreCase)
			lpPos = _tcsrchr(lpHaystack, cNeedle);
		else
			lpPos = _tcsirchr(lpHaystack, cNeedle);

	if (lpPos == NULL)
		dwResult = (DWORD) -1;
	else
		dwResult = (DWORD) (lpPos - lpHaystack) + nPos;

	return dwResult;
}

//
//   函数: CStringT &CStringT::ReplaceImpl(LPCTSTR lpFind, LPCTSTR lpReplace, DWORD nStart, BOOL bReplaceOnce, BOOL bIgnoreCase)
//
//   目的: 字符串替换函数的实现。nStart 指定起始的查找位置，从 0 开始；bReplaceOnce 指定是否只替换第一个找到的字串；
//         bIgnoreCase 指定是否忽略大小写
//
CStringT &CStringT::ReplaceImpl(LPCTSTR lpFind, LPCTSTR lpReplace, DWORD nStart, BOOL bReplaceOnce, BOOL bIgnoreCase)
{
	DWORD dwFinalSize;
	int nSizeDiff, nFind, nReplace;
	LPTSTR lpBuffer, lpTemp, lpToReplace, lpFirst, lpLast;

	__try
	{
		Lock();

		// 查找的字符串是否有效，是否为空
		if (!lpFind || *lpFind == '\0')
			__leave;

		// 若替换字符串无效，则默认其为空串
		if (!lpReplace) lpReplace = _T("");

		// 在区分大小写的情况下，若两个字符串相同，则不作任何操作
		if ((!bIgnoreCase && !_tcscmp(lpFind, lpReplace)))
			__leave;

		// 起始位置是否有效
		if (nStart > _tcslen(m_pStrings[m_dwStringId].lpBuffer) - _tcslen(lpFind))
			__leave;

		Detach();

		lpBuffer = m_pStrings[m_dwStringId].lpBuffer;
		nFind = _tcslen(lpFind);
		nReplace = _tcslen(lpReplace);
		nSizeDiff = nReplace - nFind;

		// 若用于替换的字符串长度大于查找的字符串，则需扩容
		if (nSizeDiff > 0)
		{
			DWORD dwCount = 0, dwPos = 0;

			// 统计被查找的字符串出现的次数
			while ((DWORD) -1 != (dwPos = Find(lpFind, dwPos, FALSE, bIgnoreCase)))
			{
				dwCount++;
				dwPos += nFind;
			}
			// 若只替换查找到的第一个字串
			if (dwCount && bReplaceOnce) dwCount = 1;

			if (dwCount)
			{
				// 计算空间大小
				dwFinalSize = _tcslen(lpBuffer) + nSizeDiff * dwCount + 1;
				SetRecordCapacity(m_dwStringId, dwFinalSize);
			}
		}

		lpBuffer += nStart;

		if (!bIgnoreCase)
			lpTemp = _tcsstr(lpBuffer, lpFind);
		else
			lpTemp = _tcsistr(lpBuffer, lpFind);

		while (lpTemp)
		{
			// 若两字符串长度不相同，则需移动后面的子串
			if (nSizeDiff)
			{
				if (nSizeDiff > 0)
				{
					// 向后移动
					lpFirst = lpTemp + nFind;
					lpLast = lpBuffer + _tcslen(lpBuffer);

					while (lpLast >= lpFirst)
					{
						*(lpLast + nSizeDiff) = *lpLast;
						lpLast--;
					}
				}
				else
				{
					// 向前移动
					lpFirst = lpTemp + nReplace;
					lpLast = lpBuffer + _tcslen(lpBuffer) + nSizeDiff;
					while (lpFirst <= lpLast)
					{
						*lpFirst = *(lpFirst - nSizeDiff);
						lpFirst++;
					}
				}
			}

			// 替换字符串
			lpToReplace = const_cast<LPTSTR>(lpReplace);
			while (*lpToReplace)
			{
				*lpTemp++ = *lpToReplace++;
			}

			// 若只替换一次，则跳出循环
			if (bReplaceOnce) break;

			if (!bIgnoreCase)
				lpTemp = _tcsstr(lpTemp, lpFind);
			else
				lpTemp = _tcsistr(lpTemp, lpFind);
		}
	}
	__finally
	{
		Unlock();
	}

	m_bStrConvModified = TRUE;

	return *this;
}

CStringT &CStringT::Replace(LPCWSTR lpFind, LPCWSTR lpReplace, DWORD nStart, BOOL bReplaceOnce, BOOL bIgnoreCase)
{
#ifdef UNICODE
	ReplaceImpl(lpFind, lpReplace, nStart, bReplaceOnce, bIgnoreCase);
#else
	LPTSTR lpTempFind = ToAnsi(lpFind);
	LPTSTR lpTempReplace = ToAnsi(lpReplace);

	ReplaceImpl(lpTempFind, lpTempReplace, nStart, bReplaceOnce, bIgnoreCase);

	FreeConv(lpTempFind);
	FreeConv(lpTempReplace);
#endif

	return *this;
}

CStringT &CStringT::Replace(LPCSTR lpFind, LPCSTR lpReplace, DWORD nStart, BOOL bReplaceOnce, BOOL bIgnoreCase)
{
#ifdef UNICODE
	LPTSTR lpTempFind = ToWide(lpFind);
	LPTSTR lpTempReplace = ToWide(lpReplace);

	ReplaceImpl(lpTempFind, lpTempReplace, nStart, bReplaceOnce, bIgnoreCase);

	FreeConv(lpTempFind);
	FreeConv(lpTempReplace);
#else
	ReplaceImpl(lpFind, lpReplace, nStart, bReplaceOnce, bIgnoreCase);
#endif

	return *this;
}

CStringT &CStringT::Replace(LPCWSTR lpFind, LPCSTR lpReplace, DWORD nStart, BOOL bReplaceOnce, BOOL bIgnoreCase)
{
#ifdef UNICODE
	LPTSTR lpTempReplace = ToWide(lpReplace);

	ReplaceImpl(lpFind, lpTempReplace, nStart, bReplaceOnce, bIgnoreCase);

	FreeConv(lpTempReplace);
#else
	LPTSTR lpTempFind = ToAnsi(lpFind);

	ReplaceImpl(lpTempFind, lpReplace, nStart, bReplaceOnce, bIgnoreCase);

	FreeConv(lpTempFind);
#endif

	return *this;
}

CStringT &CStringT::Replace(LPCSTR lpFind, LPCWSTR lpReplace, DWORD nStart, BOOL bReplaceOnce, BOOL bIgnoreCase)
{
#ifdef UNICODE
	LPTSTR lpTempFind = ToWide(lpFind);

	ReplaceImpl(lpTempFind, lpReplace, nStart, bReplaceOnce, bIgnoreCase);

	FreeConv(lpTempFind);
#else
	LPTSTR lpTempReplace = ToAnsi(lpReplace);

	ReplaceImpl(lpFind, lpTempReplace, nStart, bReplaceOnce, bIgnoreCase);

	FreeConv(lpTempReplace);
#endif

	return *this;
}

CStringT &CStringT::Replace(const int cFind, LPCWSTR lpReplace, DWORD nStart, BOOL bReplaceOnce, BOOL bIgnoreCase)
{
	if ((cFind & 0xFFFF) == 0) return *this;

#ifdef UNICODE
	TCHAR pFind[2];
	pFind[0] = cFind & 0xFFFF;
	pFind[1] = '\0';

	ReplaceImpl(pFind, lpReplace, nStart, bReplaceOnce, bIgnoreCase);
#else
	TCHAR pFind[2];
	LPTSTR lpTempReplace = ToAnsi(lpReplace);
	pFind[0] = cFind & 0xFF;
	pFind[1] = '\0';

	ReplaceImpl(pFind, lpTempReplace, nStart, bReplaceOnce, bIgnoreCase);

	FreeConv(lpTempReplace);
#endif

	return *this;
}

CStringT &CStringT::Replace(const int cFind, LPCSTR lpReplace, DWORD nStart, BOOL bReplaceOnce, BOOL bIgnoreCase)
{
	if ((cFind & 0xFFFF) == 0) return *this;

#ifdef UNICODE
	TCHAR pFind[2];
	LPTSTR lpTempReplace = ToWide(lpReplace);
	pFind[0] = cFind & 0xFFFF;
	pFind[1] = '\0';

	ReplaceImpl(pFind, lpTempReplace, nStart, bReplaceOnce, bIgnoreCase);

	FreeConv(lpTempReplace);
#else
	TCHAR pFind[2];
	pFind[0] = cFind & 0xFF;
	pFind[1] = '\0';

	ReplaceImpl(pFind, lpReplace, nStart, bReplaceOnce, bIgnoreCase);
#endif

	return *this;
}

CStringT &CStringT::Replace(LPCWSTR lpFind, const int cReplace, DWORD nStart, BOOL bReplaceOnce, BOOL bIgnoreCase)
{
#ifdef UNICODE
	TCHAR pReplace[2];
	pReplace[0] = cReplace & 0xFFFF;
	pReplace[1] = '\0';

	ReplaceImpl(lpFind, pReplace, nStart, bReplaceOnce, bIgnoreCase);
#else
	TCHAR pReplace[2];
	LPTSTR lpTempFind = ToAnsi(lpFind);
	pReplace[0] = cReplace & 0xFF;
	pReplace[1] = '\0';

	ReplaceImpl(lpTempFind, pReplace, nStart, bReplaceOnce, bIgnoreCase);

	FreeConv(lpTempFind);
#endif

	return *this;
}

CStringT &CStringT::Replace(LPCSTR lpFind, const int cReplace, DWORD nStart, BOOL bReplaceOnce, BOOL bIgnoreCase)
{
#ifdef UNICODE
	TCHAR pReplace[2];
	LPTSTR lpTempFind = ToWide(lpFind);
	pReplace[0] = cReplace & 0xFFFF;
	pReplace[1] = '\0';

	ReplaceImpl(lpTempFind, pReplace, nStart, bReplaceOnce, bIgnoreCase);

	FreeConv(lpTempFind);
#else
	TCHAR pReplace[2];
	pReplace[0] = cReplace & 0xFF;
	pReplace[1] = '\0';

	ReplaceImpl(lpFind, pReplace, nStart, bReplaceOnce, bIgnoreCase);
#endif

	return *this;
}

CStringT &CStringT::Replace(const int cFind, const int cReplace, DWORD nStart, BOOL bReplaceOnce, BOOL bIgnoreCase)
{
	if ((cFind & 0xFFFF) == 0) return *this;

#ifdef UNICODE
	TCHAR pFind[2], pReplace[2];
	pFind[0] = cFind & 0xFFFF;
	pFind[1] = '\0';
	pReplace[0] = cReplace & 0xFFFF;
	pReplace[1] = '\0';

	ReplaceImpl(pFind, pReplace, nStart, bReplaceOnce, bIgnoreCase);
#else
	TCHAR pFind[2], pReplace[2];
	pFind[0] = cFind & 0xFF;
	pFind[1] = '\0';
	pReplace[0] = cReplace & 0xFF;
	pReplace[1] = '\0';

	ReplaceImpl(pFind, pReplace, nStart, bReplaceOnce, bIgnoreCase);
#endif

	return *this;
}

//
//   函数: CStringT &CStringT::ReplaceImpl(DWORD nStart, DWORD nLength, LPCTSTR lpReplace)
//
//   目的: 字符串替换函数的实现，实现将字符串的某一段替换为另一个字符串。nStart 指定起始位置，从 0 开始；
//         nLength 指定要替换的字符串的长度
//
//   注释: 该函数同时也实现字符串的删除和插入
//
CStringT &CStringT::ReplaceImpl(DWORD nStart, DWORD nLength, LPCTSTR lpReplace)
{
	DWORD nOrigBuff, nReplace, nSizeDiff;
	LPTSTR lpBuffer, lpFirst, lpLast;

	__try
	{
		Lock();

		if (!lpReplace) lpReplace = _T("");

		nOrigBuff = _tcslen(m_pStrings[m_dwStringId].lpBuffer);

		// 判断数据是否合法
		if (nStart + nLength > nOrigBuff)
		{
			if (nStart >= nOrigBuff)
			{
				// 若起始位置大于字符串长度，则实际操作改为追加
				nStart = nOrigBuff;
				nLength = 0;
			}
			else
				nLength = nOrigBuff - nStart;
		}
		
		nReplace = _tcslen(lpReplace);
		nSizeDiff = nReplace - nLength;

		if (nSizeDiff == 0)
		{
			// 若被替换的字符串与原字符串相同，则不作任何操作
			if (!_tcsncmp(m_pStrings[m_dwStringId].lpBuffer + nStart, lpReplace, nReplace))
				__leave;
		}

		Detach();

		// 若替换后的长度大于原长度，则扩容
		if (nSizeDiff > 0)
		{
			SetRecordCapacity(m_dwStringId, nOrigBuff + nSizeDiff + 1);
		}
		
		lpBuffer = m_pStrings[m_dwStringId].lpBuffer + nStart;

		// 移动字符串
		if (nSizeDiff)
		{
			if (nSizeDiff > 0)
			{
				// 向后移动
				lpFirst = lpBuffer + nLength;
				lpLast = lpBuffer + _tcslen(lpBuffer);

				while (lpLast >= lpFirst)
				{
					*(lpLast + nSizeDiff) = *lpLast;
					lpLast--;
				}
			}
			else
			{
				// 向前移动
				lpFirst = lpBuffer + nReplace;
				lpLast = lpBuffer + _tcslen(lpBuffer) + nSizeDiff;
				while (lpFirst <= lpLast)
				{
					*lpFirst = *(lpFirst - nSizeDiff);
					lpFirst++;
				}
			}
		}

		// 替换字符串
		while (*lpReplace)
		{
			*lpBuffer++ = *lpReplace++;
		}
	}
	__finally
	{
		Unlock();
	}

	m_bStrConvModified = TRUE;

	return *this;
}

CStringT &CStringT::Replace(DWORD nStart, DWORD nLength, LPCWSTR lpReplace)
{
#ifdef UNICODE
	ReplaceImpl(nStart, nLength, lpReplace);
#else
	LPTSTR pTemp = ToAnsi(lpReplace);

	Replace(nStart, nLength, pTemp);

	FreeConv(pTemp);
#endif

	return *this;
}

CStringT &CStringT::Replace(DWORD nStart, DWORD nLength, LPCSTR lpReplace)
{
#ifdef UNICODE
	LPTSTR pTemp = ToWide(lpReplace);

	Replace(nStart, nLength, pTemp);

	FreeConv(pTemp);
#else
	ReplaceImpl(nStart, nLength, lpReplace);
#endif

	return *this;
}

CStringT &CStringT::Replace(DWORD nStart, DWORD nLength, const int cReplace)
{
	TCHAR c[2];
#ifdef UNICODE
	c[0] = cReplace & 0xFFFF;
#else
	c[0] = cReplace & 0xFF;
#endif
	c[1] = '\0';

	return ReplaceImpl(nStart, nLength, c);
}

//
//   函数: CStringT &CStringT::Insert(DWORD nStart, LPCWSTR lpString)
//
//   目的: 在指定位置插入字符串
//
CStringT &CStringT::Insert(DWORD nStart, LPCWSTR lpString)
{
#ifdef UNICODE
	ReplaceImpl(nStart, 0, lpString);
#else
	LPTSTR pTemp = ToAnsi(lpString);

	ReplaceImpl(nStart, 0, pTemp);

	FreeConv(pTemp);
#endif

	return *this;
}

CStringT &CStringT::Insert(DWORD nStart, LPCSTR lpString)
{
#ifdef UNICODE
	LPTSTR pTemp = ToWide(lpString);

	ReplaceImpl(nStart, 0, pTemp);

	FreeConv(pTemp);
#else
	ReplaceImpl(nStart, 0, lpString);
#endif

	return *this;
}

CStringT &CStringT::Insert(DWORD nStart, const int Ch)
{
	TCHAR c[2];
#ifdef UNICODE
	c[0] = Ch & 0xFFFF;
#else
	c[0] = Ch & 0xFF;
#endif
	c[1] = '\0';

	return ReplaceImpl(nStart, 0, c);
}

//
//   函数: CStringT &CStringT::Delete(DWORD nStart, DWORD nLength)
//
//   目的: 删除指定范围的字符串
//
CStringT &CStringT::Delete(DWORD nStart, DWORD nLength)
{
	return ReplaceImpl(nStart, nLength, _T(""));
}

//
//   函数: LPCTSTR CStringT::SubString(DWORD nStart, DWORD nEnd)
//
//   目的: 截取字符串。nStart 指定起始位置；nEnd 指定结束位置
//
//   注释: 此操作不会改变本字符串，而是生成一个新字符串，并将其添加到字符串记录表中
//         尽量使用字符串类作为左值来接受此字符串，否则可能会造成内存浪费（不会泄露）
//
LPCTSTR CStringT::SubString(DWORD nStart, DWORD nEnd)
{
	DWORD dwLen, dwTemp;
	DWORD dwNewId;
	LPTSTR lpResult = NULL;

	__try
	{
		Lock();

		lpResult = m_pStrings[0].lpBuffer;

		dwLen = _tcslen(m_pStrings[m_dwStringId].lpBuffer);

		// 判断数据是否正确
		if (nStart > nEnd)
		{
			dwTemp = nStart;
			nStart = nEnd;
			nEnd = dwTemp;
		}

		if (nStart == nEnd)
			__leave;

		if (nStart >= dwLen)
		{
			lpResult = m_pStrings[0].lpBuffer;
			__leave;
		}

		if (nEnd >= dwLen) nEnd = dwLen - 1;

		dwNewId = AllocateRecord();
		SetRecordCapacity(dwNewId, nEnd - nStart + 2);

		lpResult = m_pStrings[dwNewId].lpBuffer;

		_tcsncpy_s(lpResult, nEnd - nStart + 2, m_pStrings[m_dwStringId].lpBuffer + nStart, nEnd - nStart + 1);

		m_pStrings[dwNewId].bFakeRef = TRUE;
	}
	__finally
	{
		Unlock();
	}

	return lpResult;
}

//
//   函数: LPCTSTR CStringT::SubStr(DWORD nStart, DWORD nLength)
//
//   目的: 截取字符串。nStart 指定起始位置；nLength 指定截取长度
//
//   注释: 此操作不会改变本字符串，而是生成一个新字符串，并将其添加到字符串记录表中
//         尽量使用字符串类作为左值来接受此字符串，否则可能会造成内存浪费（不会泄露）
//
LPCTSTR CStringT::SubStr(DWORD nStart, DWORD nLength)
{
	return SubString(nStart, nStart + nLength - 1);
}

//
//   函数: LPCTSTR CStringT::LeftStr(DWORD nLength)
//
//   目的: 从左边截取字符串。nLength 指定截取长度
//
//   注释: 此操作不会改变本字符串，而是生成一个新字符串，并将其添加到字符串记录表中
//         尽量使用字符串类作为左值来接受此字符串，否则可能会造成内存浪费（不会泄露）
//
LPCTSTR CStringT::LeftStr(DWORD nLength)
{
	return SubString(0, nLength - 1);
}

//
//   函数: LPCTSTR CStringT::RightStr(DWORD nLength)
//
//   目的: 从右边截取字符串。nLength 指定截取长度
//
//   注释: 此操作不会改变本字符串，而是生成一个新字符串，并将其添加到字符串记录表中
//         尽量使用字符串类作为左值来接受此字符串，否则可能会造成内存浪费（不会泄露）
//
LPCTSTR CStringT::RightStr(DWORD nLength)
{
	DWORD dwLen = _tcslen(m_pStrings[m_dwStringId].lpBuffer);
	return SubString(dwLen - nLength, dwLen - 1);
}

//
//   函数: CStringT &CStringT::Trim()
//
//   目的: 去除字符串两边的空格
//
CStringT &CStringT::Trim()
{
	LPTSTR lpOldBuf, lpBuffer, lpLeft, lpRight;

	__try
	{
		Lock();

		lpOldBuf = m_pStrings[m_dwStringId].lpBuffer;

		lpLeft = lpOldBuf;
		lpRight = lpOldBuf + _tcslen(lpOldBuf) - 1;

		while (*lpLeft == '\0' || *lpLeft == '\t' || *lpLeft == '\n' || *lpLeft == '\r' || *lpLeft == ' ') lpLeft++;
		while (*lpRight == '\0' || *lpRight == '\t' || *lpRight == '\n' || *lpRight == '\r' || *lpRight == ' ') lpRight--;
		
		if ((lpLeft == lpOldBuf) && (lpRight == lpOldBuf + _tcslen(lpOldBuf) - 1))
			__leave;

		Detach();

		lpBuffer = m_pStrings[m_dwStringId].lpBuffer;

		*(lpBuffer + (lpRight - lpOldBuf) + 1) = '\0';

		if (lpLeft > lpOldBuf)
		{
			lpLeft = lpBuffer + (lpLeft - lpOldBuf);

			// 去除左边的空格需要移动后面的字符串
			while(*lpLeft)
			{
				*lpBuffer++ = *lpLeft++;
			}

			*lpBuffer = '\0';
		}
	}
	__finally
	{
		Unlock();
	}

	return *this;
}

//
//   函数: CStringT &CStringT::TrimLeft()
//
//   目的: 去除字符串左边的空格
//
CStringT &CStringT::TrimLeft()
{
	LPTSTR lpOldBuf, lpBuffer, lpLeft;

	__try
	{
		Lock();

		lpOldBuf = m_pStrings[m_dwStringId].lpBuffer;

		lpLeft = lpOldBuf;

		while (*lpLeft == '\0' || *lpLeft == '\t' || *lpLeft == '\n' || *lpLeft == '\r' || *lpLeft == ' ') lpLeft++;
		
		if (lpLeft == lpOldBuf)
			__leave;

		Detach();

		lpBuffer = m_pStrings[m_dwStringId].lpBuffer;

		if (lpLeft > lpOldBuf)
		{
			lpLeft = lpBuffer + (lpLeft - lpOldBuf);

			while(*lpLeft)
			{
				*lpBuffer++ = *lpLeft++;
			}

			*lpBuffer = '\0';
		}
	}
	__finally
	{
		Unlock();
	}

	return *this;
}

//
//   函数: CStringT &CStringT::TrimLeft()
//
//   目的: 去除字符串右边的空格
//
CStringT &CStringT::TrimRight()
{
	LPTSTR lpOldBuf, lpBuffer, lpRight;

	__try
	{
		Lock();

		lpOldBuf = m_pStrings[m_dwStringId].lpBuffer;

		lpRight = lpOldBuf + _tcslen(lpOldBuf) - 1;

		while (*lpRight == '\0' || *lpRight == '\t' || *lpRight == '\n' || *lpRight == '\r' || *lpRight == ' ') lpRight--;
		
		if (lpRight == lpOldBuf + _tcslen(lpOldBuf) - 1)
			__leave;

		Detach();

		lpBuffer = m_pStrings[m_dwStringId].lpBuffer;

		*(lpBuffer + (lpRight - lpOldBuf) + 1) = '\0';
	}
	__finally
	{
		Unlock();
	}

	return *this;
}

//
//   函数: CStringT &CStringT::FormatImpl(LPCTSTR lpFormat, va_list arglist)
//
//   目的: 字符串格式化函数的实现
//
CStringT &CStringT::FormatImpl(LPCTSTR lpFormat, va_list arglist)
{
	int nSize;
	LPTSTR pTemp;

	nSize = _vsctprintf(lpFormat, arglist);

	pTemp = (LPTSTR) malloc((nSize + 1) * sizeof (TCHAR));

	_vstprintf_s(pTemp, nSize + 1, lpFormat, arglist);
	SetString(pTemp);

	free(pTemp);

	return *this;
}

CStringT &CStringT::Format(LPCWSTR lpFormat, ...)
{
	va_list arglist;

	va_start(arglist, lpFormat);
#ifdef UNICODE
	FormatImpl(lpFormat, arglist);
#else
	LPTSTR pTemp = ToAnsi(lpFormat);

	FormatImpl(pTemp, arglist);

	FreeConv(pTemp);
#endif
	va_end(arglist);

	return *this;
}

CStringT &CStringT::Format(LPCSTR lpFormat, ...)
{
	va_list arglist;

	va_start(arglist, lpFormat);
#ifdef UNICODE
	LPTSTR pTemp = ToWide(lpFormat);

	FormatImpl(pTemp, arglist);

	FreeConv(pTemp);
#else
	FormatImpl(lpFormat, arglist);
#endif
	va_end(arglist);

	return *this;
}

//
//   函数: CStringT::operator LPCWSTR()
//
//   目的: 类型转换运算符重载，在字符串类作为常量右值时返回字符串缓冲区地址
//
CStringT::operator LPCWSTR()
{
#ifdef UNICODE
	return StringBuffer();
#else
	if (m_bStrConvModified)
	{
		DWORD cchSize = MultiByteToWideChar(CP_ACP, 0, StringBuffer(), -1, NULL, 0);
		if (m_cchStrConv < cchSize)
		{
			if (!m_lpStrConv)
				m_lpStrConv = (LPWSTR) malloc(cchSize * sizeof (WCHAR));
			else
				m_lpStrConv = (LPWSTR) realloc(m_lpStrConv, cchSize * sizeof (WCHAR));
			m_cchStrConv = cchSize;
		}
		MultiByteToWideChar(CP_ACP, 0, StringBuffer(), -1, m_lpStrConv, m_cchStrConv);

		m_bStrConvModified = FALSE;
	}

	return m_lpStrConv;
#endif
}

//
//   函数: CStringT::operator LPCSTR()
//
//   目的: 类型转换运算符重载，在字符串类作为常量右值时返回字符串缓冲区地址
//
CStringT::operator LPCSTR()
{
#ifdef UNICODE
	if (m_bStrConvModified)
	{
		DWORD cchSize = WideCharToMultiByte(CP_ACP, 0, StringBuffer(), -1, NULL, 0, NULL, NULL);
		if (m_cchStrConv < cchSize)
		{
			if (!m_lpStrConv)
				m_lpStrConv = (LPSTR) malloc(cchSize * sizeof (CHAR));
			else
				m_lpStrConv = (LPSTR) realloc(m_lpStrConv, cchSize * sizeof (CHAR));
			m_cchStrConv = cchSize;
		}
		WideCharToMultiByte(CP_ACP, 0, StringBuffer(), -1, m_lpStrConv, m_cchStrConv, NULL, NULL);

		m_bStrConvModified = FALSE;
	}

	return m_lpStrConv;
#else
	return StringBuffer();
#endif
}