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

CRITICAL_SECTION CStringT::m_pCriticalSection = {};		// �ٽ����������ڽ����̵߳������Ի�
PSTRING_RECORD CStringT::m_pStrings = NULL;				// �ַ�����¼��
DWORD CStringT::m_dwStringsCapacity = 0;				// �ַ�����¼������
DWORD CStringT::m_dwMaxPosition = -1;					// �ַ�����¼��ǰʹ�õ����ֵ
LONG CStringT::m_lInitRef = 0;							// ���ʼ��������

// �����»��߿�ͷ�������ַ�����������Ϊ���ֲ� CRT �����Ĳ���

//
//   ����: const char *_strrstr(const char *str1, const char *str2)
//
//   Ŀ��: strstr ��������Ұ�
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
//   ����: const char *_strrstr(char *str1, const char *str2)
//
//   Ŀ��: strstr ��������Ұ�
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
//   ����: const char *_strirstr(const char *str1, const char *str2)
//
//   Ŀ��: strstr ��������Ҳ����Դ�Сд��
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
//   ����: char *_strirstr(char *str1, const char *str2)
//
//   Ŀ��: strstr ��������Ҳ����Դ�Сд��
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
//   ����: const char *_stristr(const char *str1, const char *str2)
//
//   Ŀ��: strstr �ĺ��Դ�Сд��
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
//   ����: char *_stristr(char *str1, const char *str2)
//
//   Ŀ��: strstr �ĺ��Դ�Сд��
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
//   ����: const wchar_t *_wcsrstr(const wchar_t *str1, const wchar_t *str2)
//
//   Ŀ��: wcsstr ��������Ұ�
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
//   ����: wchar_t *_wcsrstr(wchar_t *str1, const wchar_t *str2)
//
//   Ŀ��: wcsstr ��������Ұ�
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
//   ����: const wchar_t *_wcsirstr(const wchar_t *str1, const wchar_t *str2)
//
//   Ŀ��: wcsstr ��������Ҳ����Դ�Сд��
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
//   ����: wchar_t *_wcsirstr(wchar_t *str1, const wchar_t *str2)
//
//   Ŀ��: wcsstr ��������Ҳ����Դ�Сд��
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
//   ����: const wchar_t *_wcsistr(const wchar_t *str1, const wchar_t *str2)
//
//   Ŀ��: wcsstr �ĺ��Դ�Сд��
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
//   ����: wchar_t *_wcsistr(wchar_t *str1, const wchar_t *str2)
//
//   Ŀ��: wcsstr �ĺ��Դ�Сд��
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
//   ����: const char *_strichr(const char *str1, const char s2)
//
//   Ŀ��: strchr �ĺ��Դ�Сд��
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
//   ����: char *_strichr(char *str1, const char s2)
//
//   Ŀ��: strchr �ĺ��Դ�Сд��
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
//   ����: const char *_strirchr(const char *str1, const char s2)
//
//   Ŀ��: strchr ��������Ҳ����Դ�Сд��
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
//   ����: char *_strirchr(char *str1, const char s2)
//
//   Ŀ��: strchr ��������Ҳ����Դ�Сд��
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
//   ����: const wchar_t *_wcsichr(const wchar_t *str1, const wchar_t s2)
//
//   Ŀ��: wcschr �ĺ��Դ�Сд��
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
//   ����: wchar_t *_wcsichr(wchar_t *str1, const wchar_t s2)
//
//   Ŀ��: wcschr �ĺ��Դ�Сд��
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
//   ����: wchar_t *_wcsichr(wchar_t *str1, const wchar_t s2)
//
//   Ŀ��: wcschr ��������Ҳ����Դ�Сд��
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
//   ����: wchar_t *_wcsirchr(wchar_t *str1, const wchar_t s2)
//
//   Ŀ��: wcschr ��������Ҳ����Դ�Сд��
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
//   ����: void CStringT::CommonConstructor()
//
//   Ŀ��: ��ʼ���ַ�����
//
//   ע��: ��Ϊģ���ڵĵ�һ�γ�ʼ������ͬʱ��ʼ���ַ�����¼��
//
void CStringT::CommonConstructor()
{
	// InterlockedIncrement ͨ��ָ�����ӳ�ʼ��������ֵ������ʹ�̲߳��в������Ի�
	if (InterlockedIncrement(&m_lInitRef) == 1)
	{
		// ��ʼ���ٽ���
		InitializeCriticalSection(&m_pCriticalSection);

		__try
		{
			// �����̣߳��Է�ֹ�����ַ������ڳ�ʼ�������δ����ʼ�����ַ�����¼��
			Lock();

			// �����ַ�����¼���ʼ����
			m_dwStringsCapacity = STRINGS_CAP_INCREASEMENT;
			m_pStrings = (PSTRING_RECORD) malloc(m_dwStringsCapacity * sizeof (STRING_RECORD));
			ZeroMemory(m_pStrings, m_dwStringsCapacity * sizeof (STRING_RECORD));
			
			// ��ʼ����һ���ַ�����¼��һ�����ַ���
			// ����ַ�������Ϊ�Ժ������³�ʼ�����ַ������Ĭ���ַ���
			// ����ģ�����������ڶ����ᱻ�ͷ�
			m_dwMaxPosition = 0;
			m_pStrings[0].dwRefCount = 1;
			m_pStrings[0].dwCapacity = STRING_MIN_ALIGN_SIZE;
			m_pStrings[0].lpBuffer = (LPTSTR) malloc(m_pStrings[0].dwCapacity * sizeof(TCHAR));
			m_pStrings[0].lpBuffer[0] = '\0';
		}
		__finally
		{
			// ����
			Unlock();
		}
	}

	// ����Ĭ���ַ���
	m_dwStringId = 0;
	m_lpStrConv = NULL;
	m_cchStrConv = 0;
	m_bStrConvModified = TRUE;
}

//
//   ����: CStringT::CStringT()
//
//   Ŀ��: Ĭ�Ϲ��캯��
//
CStringT::CStringT()
{
	CommonConstructor();
}

//
//   ����: CStringT::~CStringT()
//
//   Ŀ��: ��������
//
//   ע��: ���������������������ʹ�ã���ͬʱ�����ַ�����¼��
//
CStringT::~CStringT()
{
	// ��ʼ��������һ
	if (InterlockedDecrement(&m_lInitRef) == 0)
	{
		__try
		{
			Lock();

			// �����ַ�����¼����������Ϊ��ʼ״̬
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

		// �����ٽ���
		DeleteCriticalSection(&m_pCriticalSection);
	}
	else
	{
		// ��ǰʹ�õ��ַ�����¼���ô�����һ
		if (m_dwStringId != 0) ReleaseReferenceCount(m_dwStringId);
	}

	// �ͷ�����ת���õ���ʱ�洢
	if (m_lpStrConv) free(m_lpStrConv);
}

//
//   ����: void CStringT::DestroyStringRecord(STRING_RECORD &pRecord)
//
//   Ŀ��: �����ַ�����¼��������Ϊδʹ��״̬
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
//   ����: DWORD CStringT::AllocateRecord()
//
//   Ŀ��: �����ַ�����¼��������Ĭ���ַ������մ���
//
DWORD CStringT::AllocateRecord()
{
	DWORD dwId = 0;
	DWORD dwOldCapacity;

	__try
	{
		// �����ַ�����¼��������δʹ�õļ�¼
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

		// û�п��õļ�¼��������ַ�����¼������
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
//   ����: DWORD CStringT::AllocateRecord(LPCTSTR lpString, int cchSize)
//
//   Ŀ��: �����ַ�����¼�������ַ������ݡ�cchSize ָ��Ҫ�����ַ����ĳ���
//
DWORD CStringT::AllocateRecord(LPCTSTR lpString, int cchSize)
{
	DWORD dwId = AllocateRecord();

	// ��������
	SetRecordString(m_pStrings[dwId], lpString, cchSize);

	return dwId;
}

//
//   ����: void CStringT::SetRecordCapacity(STRING_RECORD &pRecord, DWORD cchSize)
//
//   Ŀ��: �����ַ�����¼����
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
//   ����: void CStringT::SetRecordCapacity(DWORD dwRecordId, DWORD cchSize)
//
//   Ŀ��: ͨ�� ID �����ַ�����¼����
//
void CStringT::SetRecordCapacity(DWORD dwRecordId, DWORD cchSize)
{
	if (dwRecordId >= m_dwStringsCapacity && !m_pStrings[dwRecordId].dwRefCount) return;

	SetRecordCapacity(m_pStrings[dwRecordId], cchSize);

}

//
//   ����: BOOL CStringT::SetRecordString(STRING_RECORD &pRecord, LPCTSTR lpString, int cchSize)
//
//   Ŀ��: �����ַ�����¼ֵ��cchSize ָ��Ҫ�����ַ����ĳ���
//
BOOL CStringT::SetRecordString(STRING_RECORD &pRecord, LPCTSTR lpString, int cchSize)
{
	BOOL bResult = FALSE;

	// ���ַ�����ַΪ NULL�����������մ�
	if (lpString)
	{
		if (cchSize == -1) cchSize = _tcslen(lpString);
	}else
	{
		cchSize = 0;
	}

	// ������������
	SetRecordCapacity(pRecord, cchSize + 1);
		
	if (lpString)
		_tcscpy_s(pRecord.lpBuffer, pRecord.dwCapacity, lpString);
	else
		pRecord.lpBuffer[0] = '\0';

	bResult = TRUE;

	return bResult;
}

//
//   ����: BOOL CStringT::SetRecordString(DWORD dwRecordId, LPCTSTR lpString, int cchSize)
//
//   Ŀ��: ͨ�� ID �����ַ�����¼ֵ��cchSize ָ��Ҫ�����ַ����ĳ���
//
BOOL CStringT::SetRecordString(DWORD dwRecordId, LPCTSTR lpString, int cchSize)
{
	BOOL bResult = FALSE;

	if (dwRecordId >= m_dwStringsCapacity && !m_pStrings[dwRecordId].dwRefCount) return FALSE;

	bResult = SetRecordString(m_pStrings[dwRecordId], lpString, cchSize);

	return bResult;
}

//
//   ����: void CStringT::AddReferenceCount(STRING_RECORD &pRecord)
//
//   Ŀ��: �����ַ�����¼���ô���
//
void CStringT::AddReferenceCount(STRING_RECORD &pRecord)
{
	if (pRecord.dwRefCount && !pRecord.bFakeRef) pRecord.dwRefCount++;
	pRecord.bFakeRef = FALSE;
}

//
//   ����: void CStringT::AddReferenceCount(DWORD dwRecordId)
//
//   Ŀ��: ͨ�� ID �����ַ�����¼���ô���
//
void CStringT::AddReferenceCount(DWORD dwRecordId)
{
	if ((dwRecordId >= m_dwStringsCapacity) || (dwRecordId == 0)) return;

	AddReferenceCount(m_pStrings[dwRecordId]);
}

//
//   ����: void CStringT::ReleaseReferenceCount(STRING_RECORD &pRecord)
//
//   Ŀ��: �����ַ�����¼���ô���
//
//   ע��: ���ַ�����¼���ô�����Ϊ�㣬�����ٴ��ַ�����¼
//
void CStringT::ReleaseReferenceCount(STRING_RECORD &pRecord)
{
	if (pRecord.dwRefCount) pRecord.dwRefCount--;

	if (!pRecord.dwRefCount)
		DestroyStringRecord(pRecord);
}

//
//   ����: void CStringT::ReleaseReferenceCount(DWORD dwRecordId)
//
//   Ŀ��: ͨ�� ID �����ַ�����¼���ô���
//
void CStringT::ReleaseReferenceCount(DWORD dwRecordId)
{
	if ((dwRecordId >= m_dwStringsCapacity ) || (dwRecordId == 0)) return;

	ReleaseReferenceCount(m_pStrings[dwRecordId]);

	if (dwRecordId == m_dwMaxPosition) m_dwMaxPosition--;
}

//
//   ����: DWORD CStringT::FindRecord(LPCTSTR lpAddress)
//
//   Ŀ��: ���ַ�����¼�в����ַ�����ַ���Ա����θ���ͬһ�ַ���
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
//   ����: void CStringT::Lock()
//
//   Ŀ��: �߳�����������ֹ����ʹ�ô��ַ�����¼��Ĳ���ͬʱ����
//
//   ע��: ������ Unlock() ���ʹ�ã����������߳�����
//
void CStringT::Lock()
{
	EnterCriticalSection(&m_pCriticalSection);
}

//
//   ����: void CStringT::Unlock()
//
//   Ŀ��: �߳̽���
//
void CStringT::Unlock()
{
	LeaveCriticalSection(&m_pCriticalSection);
}

//
//   ����: LPCVOID CStringT::RefAddress() const
//
//   Ŀ��: �����ַ�����¼�����ʼ��ַ�������ж������ַ������Ƿ�ʹ��ͬһ���ַ�����¼��
//
LPCVOID CStringT::RefAddress() const
{
	return &m_pStrings;
}




//
//   ����: CStringT::CStringT(const CStringT &s)
//
//   Ŀ��: ���쿽������
//
CStringT::CStringT(const CStringT &s)
{
	CommonConstructor();
	ConstructCopy(const_cast<CStringT *>(&s)->RefAddress(), const_cast<CStringT *>(&s)->StringBuffer(), const_cast<CStringT *>(&s)->GetStringId());
}

//
//   ����: CStringT::CStringT(LPCWSTR lpString, int cchSize)
//
//   Ŀ��: ���첢��ֵ��Unicode �ַ�������ָ������
//
CStringT::CStringT(LPCWSTR lpString, int cchSize)
{
	CommonConstructor();
	SetString(lpString, cchSize);
}

//
//   ����: CStringT::CStringT(LPCWSTR lpString)
//
//   Ŀ��: ���첢��ֵ��Unicode �ַ���
//
CStringT::CStringT(LPCWSTR lpString)
{
	CommonConstructor();
	SetString(lpString);
}

//
//   ����: CStringT::CStringT(LPCSTR lpString, int cchSize)
//
//   Ŀ��: ���첢��ֵ��ANSI �ַ�������ָ������
//
CStringT::CStringT(LPCSTR lpString, int cchSize)
{
	CommonConstructor();
	SetString(lpString, cchSize);
}

//
//   ����: CStringT::CStringT(LPCSTR lpString)
//
//   Ŀ��: ���첢��ֵ��ANSI �ַ���
//
CStringT::CStringT(LPCSTR lpString)
{
	CommonConstructor();
	SetString(lpString);
}

//
//   ����: CStringT::CStringT(const int Ch, size_t nCount)
//
//   Ŀ��: ���첢����ַ�
//
CStringT::CStringT(const int Ch, size_t nCount)
{
	CommonConstructor();
	Fill(Ch, nCount);
}

//
//   ����: CStringT::CStringT(const int Ch)
//
//   Ŀ��: ���첢���ַ���ֵ����Ϊһ���ַ�
//
CStringT::CStringT(const int Ch)
{
	CommonConstructor();
	Fill(Ch, 1);
}

//
//   ����: LPWSTR CStringT::ToWide(LPCSTR lpString)
//
//   Ŀ��: �� ANSI �ַ���ת��Ϊ Unicode �ַ���
//
//   ע��: ���ͷŷ����ַ���
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
//   ����: LPSTR CStringT::ToAnsi(LPCWSTR lpString)
//
//   Ŀ��: �� Unicode �ַ���ת��Ϊ ANSI �ַ���
//
//   ע��: ���ͷŷ����ַ���
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
//   ����: void CStringT::FreeConv(LPVOID pAddress)
//
//   Ŀ��: �ͷ��ַ����������� NULL ���
//
void CStringT::FreeConv(LPVOID pAddress)
{
	if (pAddress) free(pAddress);
}

//
//   ����: DWORD CStringT::GetStringId()
//
//   Ŀ��: �����ַ�����¼ ID
//
DWORD CStringT::GetStringId()
{
	return m_dwStringId;
}

//
//   ����: LPCTSTR CStringT::StringBuffer()
//
//   Ŀ��: �����ַ�������������ʼ��ַ
//
LPCTSTR CStringT::StringBuffer()
{
	return m_pStrings[m_dwStringId].lpBuffer;
}

//
//   ����: void CStringT::Detach()
//
//   Ŀ��: �����ַ�����¼��ʹ֮���������ַ���������
//
//   ע��: ���ַ�����¼��������ã����临��һ�ݣ��������κθĶ�
//
void CStringT::Detach()
{
	DWORD dwOldId;

	__try
	{
		// ���еĸĶ��ڽ���ǰ����������߳�����
		Lock();

		// �����ǳ�ʼ�ַ����������ô���Ϊ1�������κβ���
		if (m_dwStringId != 0 && m_pStrings[m_dwStringId].dwRefCount == 1)
			__leave;

		dwOldId = m_dwStringId;

		// ����һ���ַ�����¼
		m_dwStringId = AllocateRecord(m_pStrings[dwOldId].lpBuffer);
		// ����֮ǰ���ַ�����¼���ô���
		ReleaseReferenceCount(dwOldId);
	}
	__finally
	{
		Unlock();
	}
}

//
//   ����: TCHAR CStringT::GetChar(DWORD nIndex)
//
//   Ŀ��: ��ȡ�ַ�����ĳ���ַ�
//
TCHAR CStringT::GetChar(DWORD nIndex)
{
	if (nIndex >= m_pStrings[m_dwStringId].dwCapacity)
		return '\0';

	return m_pStrings[m_dwStringId].lpBuffer[nIndex];
}

//
//   ����: void CStringT::SetChar(DWORD nIndex, TCHAR cNewChar)
//
//   Ŀ��: �����ַ�����ĳ���ַ�
//
void CStringT::SetChar(DWORD nIndex, TCHAR cNewChar)
{
	__try
	{
		Lock();

		// �±�Խ����
		if (nIndex >= m_pStrings[m_dwStringId].dwCapacity)
			__leave;

		// ���޸ĵĺ�֮ǰ���ַ���ͬ����������
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
//   ����: DWORD CStringT::GetLength()
//
//   Ŀ��: ��ȡ�ַ�������
//
DWORD CStringT::GetLength()
{
	return _tcslen(m_pStrings[m_dwStringId].lpBuffer);
}

//
//   ����: DWORD CStringT::GetCapacity()
//
//   Ŀ��: ��ȡ�ַ�������������
//
DWORD CStringT::GetCapacity()
{
	return m_pStrings[m_dwStringId].dwCapacity;
}

//
//   ����: DWORD CStringT::GetCapacity()
//
//   Ŀ��: �����ַ�������������
//
void CStringT::SetCapacity(DWORD dwLen)
{
	__try
	{
		Lock();

		// �����ַ�����¼δ�������ַ��������ã����ַ�����¼����δ�ı䣬�����κβ���
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
//   ����: void CStringT::Reverse()
//
//   Ŀ��: ��ת�ַ���
//
void CStringT::Reverse()
{
	LPTSTR lpBuffer;
	DWORD cchLen;
	TCHAR cTemp;

	__try
	{
		Lock();

		// �ַ�������С��2���������κθĶ�
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
//   ����: void CStringT::ToLowerCase()
//
//   Ŀ��: ���ַ���ת��ΪСд
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
//   ����: void CStringT::ToUpperCase()
//
//   Ŀ��: ���ַ���ת��Ϊ��д
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
//   ����: void CStringT::FillImpl(LPCTSTR lpString, DWORD nCount)
//
//   Ŀ��: �ַ�������ʵ��
//
void CStringT::FillImpl(LPCTSTR lpString, DWORD nCount)
{
	if (!lpString || *lpString == '\0' || !nCount) return;

	__try
	{
		Lock();

		Detach();

		// ���ַ�������Ϊ�մ�
		m_pStrings[m_dwStringId].lpBuffer[0] = '\0';

		// ����ռ��С
		SetRecordCapacity(m_dwStringId, _tcslen(lpString) * nCount + 1);

		// ���� strcat �����������
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
//   ����: void CStringT::Fill(const int Ch, DWORD nCount)
//
//   Ŀ��: ���ַ�����ַ���
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
//   ����: CStringT &CStringT::ConstructCopy(LPCVOID lpRefAddr, LPCTSTR lpString, DWORD dwStringId)
//
//   Ŀ��: ���쿽�����࿽��������
//
CStringT &CStringT::ConstructCopy(LPCVOID lpRefAddr, LPCTSTR lpString, DWORD dwStringId)
{
	__try
	{
		Lock();

		// �����ǰ�ַ�����¼������
		ReleaseReferenceCount(m_dwStringId);

		// �����ַ������Ƿ�ʹ��ͬһ���ַ�����¼��
		if (RefAddress() != lpRefAddr)
		{
			// δʹ����ͬ���ַ�����¼�����½�һ���ַ�����¼
			m_dwStringId = AllocateRecord(lpString);
		}
		else
		{
			// ����ֱ�������ַ�����¼�����ô���
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
//   ����: int CStringT::CompareImpl(LPCTSTR lpString, BOOL bIgnoreCase)
//
//   Ŀ��: �ַ����ȽϺ�����ʵ�֣�bIgnoreCase ȷ���Ƿ����ִ�Сд
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
//   ����: CStringT &CStringT::SetString(LPCWSTR lpString, int cchSize)
//
//   Ŀ��: �����ַ�����ֵ��Unicode �ַ���
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
		// ���ַ������Ͷ��ڹ��̵��ַ�������
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
		// ����ת����ֱ�����
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
//   ����: CStringT &CStringT::SetString(LPCSTR lpString, int cchSize)
//
//   Ŀ��: �����ַ�����ֵ��ANSI �ַ���
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
//   ����: CStringT &CStringT::AppendImpl(LPCTSTR lpString)
//
//   Ŀ��: �ַ���׷�Ӻ�����ʵ��
//
CStringT &CStringT::AppendImpl(LPCTSTR lpString)
{
	DWORD cchLen;

	if (*lpString == '\0' || !lpString) return *this;

	__try
	{
		Lock();

		Detach();

		// ����ռ��С
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
//   ����: LPCTSTR CStringT::ConcatImpl(LPCWSTR lpString)
//
//   Ŀ��: �ַ������Ӻ�����ʵ��
//
//   ע��: �˲�������ı䱾�ַ�������������һ�����ַ�������������ӵ��ַ�����¼����
//         ����ʹ���ַ�������Ϊ��ֵ�����ܴ��ַ�����������ܻ�����ڴ��˷ѣ�����й¶��
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

		// ����ռ��С
		dwLen = _tcslen(m_pStrings[m_dwStringId].lpBuffer) + _tcslen(lpString) + 1;

		// �����µ��ַ�����¼
		dwNewId = AllocateRecord();
		SetRecordCapacity(dwNewId, dwLen);

		lpResult = m_pStrings[dwNewId].lpBuffer;

		_tcscpy_s(lpResult, dwLen, m_pStrings[m_dwStringId].lpBuffer);
		_tcscat_s(lpResult, dwLen, lpString);

		// �˱�Ǳ������ַ�����¼δ���κ��ַ��������ã������ܱ�����
		// ���µ��ַ��������ô��ַ�����¼ʱ��Ҫ���������ô���
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
//   ����: DWORD CStringT::FindImpl(LPCTSTR lpString, DWORD nPos, BOOL bReverse, BOOL bIgnoreCase)
//
//   Ŀ��: �ַ������Һ�����ʵ�֡�nPos ָ����ʼ�Ĳ���λ�ã��� 0 ��ʼ��bReverse ָ���Ƿ�������ң�
//         bIgnoreCase ָ���Ƿ���Դ�Сд
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
//   ����: DWORD CStringT::Find(const int Ch, DWORD nPos, BOOL bReverse, BOOL bIgnoreCase)
//
//   Ŀ��: ���ַ����в����ַ���nPos ָ����ʼ�Ĳ���λ�ã��� 0 ��ʼ��bReverse ָ���Ƿ�������ң�
//         bIgnoreCase ָ���Ƿ���Դ�Сд
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
//   ����: CStringT &CStringT::ReplaceImpl(LPCTSTR lpFind, LPCTSTR lpReplace, DWORD nStart, BOOL bReplaceOnce, BOOL bIgnoreCase)
//
//   Ŀ��: �ַ����滻������ʵ�֡�nStart ָ����ʼ�Ĳ���λ�ã��� 0 ��ʼ��bReplaceOnce ָ���Ƿ�ֻ�滻��һ���ҵ����ִ���
//         bIgnoreCase ָ���Ƿ���Դ�Сд
//
CStringT &CStringT::ReplaceImpl(LPCTSTR lpFind, LPCTSTR lpReplace, DWORD nStart, BOOL bReplaceOnce, BOOL bIgnoreCase)
{
	DWORD dwFinalSize;
	int nSizeDiff, nFind, nReplace;
	LPTSTR lpBuffer, lpTemp, lpToReplace, lpFirst, lpLast;

	__try
	{
		Lock();

		// ���ҵ��ַ����Ƿ���Ч���Ƿ�Ϊ��
		if (!lpFind || *lpFind == '\0')
			__leave;

		// ���滻�ַ�����Ч����Ĭ����Ϊ�մ�
		if (!lpReplace) lpReplace = _T("");

		// �����ִ�Сд������£��������ַ�����ͬ�������κβ���
		if ((!bIgnoreCase && !_tcscmp(lpFind, lpReplace)))
			__leave;

		// ��ʼλ���Ƿ���Ч
		if (nStart > _tcslen(m_pStrings[m_dwStringId].lpBuffer) - _tcslen(lpFind))
			__leave;

		Detach();

		lpBuffer = m_pStrings[m_dwStringId].lpBuffer;
		nFind = _tcslen(lpFind);
		nReplace = _tcslen(lpReplace);
		nSizeDiff = nReplace - nFind;

		// �������滻���ַ������ȴ��ڲ��ҵ��ַ�������������
		if (nSizeDiff > 0)
		{
			DWORD dwCount = 0, dwPos = 0;

			// ͳ�Ʊ����ҵ��ַ������ֵĴ���
			while ((DWORD) -1 != (dwPos = Find(lpFind, dwPos, FALSE, bIgnoreCase)))
			{
				dwCount++;
				dwPos += nFind;
			}
			// ��ֻ�滻���ҵ��ĵ�һ���ִ�
			if (dwCount && bReplaceOnce) dwCount = 1;

			if (dwCount)
			{
				// ����ռ��С
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
			// �����ַ������Ȳ���ͬ�������ƶ�������Ӵ�
			if (nSizeDiff)
			{
				if (nSizeDiff > 0)
				{
					// ����ƶ�
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
					// ��ǰ�ƶ�
					lpFirst = lpTemp + nReplace;
					lpLast = lpBuffer + _tcslen(lpBuffer) + nSizeDiff;
					while (lpFirst <= lpLast)
					{
						*lpFirst = *(lpFirst - nSizeDiff);
						lpFirst++;
					}
				}
			}

			// �滻�ַ���
			lpToReplace = const_cast<LPTSTR>(lpReplace);
			while (*lpToReplace)
			{
				*lpTemp++ = *lpToReplace++;
			}

			// ��ֻ�滻һ�Σ�������ѭ��
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
//   ����: CStringT &CStringT::ReplaceImpl(DWORD nStart, DWORD nLength, LPCTSTR lpReplace)
//
//   Ŀ��: �ַ����滻������ʵ�֣�ʵ�ֽ��ַ�����ĳһ���滻Ϊ��һ���ַ�����nStart ָ����ʼλ�ã��� 0 ��ʼ��
//         nLength ָ��Ҫ�滻���ַ����ĳ���
//
//   ע��: �ú���ͬʱҲʵ���ַ�����ɾ���Ͳ���
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

		// �ж������Ƿ�Ϸ�
		if (nStart + nLength > nOrigBuff)
		{
			if (nStart >= nOrigBuff)
			{
				// ����ʼλ�ô����ַ������ȣ���ʵ�ʲ�����Ϊ׷��
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
			// �����滻���ַ�����ԭ�ַ�����ͬ�������κβ���
			if (!_tcsncmp(m_pStrings[m_dwStringId].lpBuffer + nStart, lpReplace, nReplace))
				__leave;
		}

		Detach();

		// ���滻��ĳ��ȴ���ԭ���ȣ�������
		if (nSizeDiff > 0)
		{
			SetRecordCapacity(m_dwStringId, nOrigBuff + nSizeDiff + 1);
		}
		
		lpBuffer = m_pStrings[m_dwStringId].lpBuffer + nStart;

		// �ƶ��ַ���
		if (nSizeDiff)
		{
			if (nSizeDiff > 0)
			{
				// ����ƶ�
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
				// ��ǰ�ƶ�
				lpFirst = lpBuffer + nReplace;
				lpLast = lpBuffer + _tcslen(lpBuffer) + nSizeDiff;
				while (lpFirst <= lpLast)
				{
					*lpFirst = *(lpFirst - nSizeDiff);
					lpFirst++;
				}
			}
		}

		// �滻�ַ���
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
//   ����: CStringT &CStringT::Insert(DWORD nStart, LPCWSTR lpString)
//
//   Ŀ��: ��ָ��λ�ò����ַ���
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
//   ����: CStringT &CStringT::Delete(DWORD nStart, DWORD nLength)
//
//   Ŀ��: ɾ��ָ����Χ���ַ���
//
CStringT &CStringT::Delete(DWORD nStart, DWORD nLength)
{
	return ReplaceImpl(nStart, nLength, _T(""));
}

//
//   ����: LPCTSTR CStringT::SubString(DWORD nStart, DWORD nEnd)
//
//   Ŀ��: ��ȡ�ַ�����nStart ָ����ʼλ�ã�nEnd ָ������λ��
//
//   ע��: �˲�������ı䱾�ַ�������������һ�����ַ�������������ӵ��ַ�����¼����
//         ����ʹ���ַ�������Ϊ��ֵ�����ܴ��ַ�����������ܻ�����ڴ��˷ѣ�����й¶��
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

		// �ж������Ƿ���ȷ
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
//   ����: LPCTSTR CStringT::SubStr(DWORD nStart, DWORD nLength)
//
//   Ŀ��: ��ȡ�ַ�����nStart ָ����ʼλ�ã�nLength ָ����ȡ����
//
//   ע��: �˲�������ı䱾�ַ�������������һ�����ַ�������������ӵ��ַ�����¼����
//         ����ʹ���ַ�������Ϊ��ֵ�����ܴ��ַ�����������ܻ�����ڴ��˷ѣ�����й¶��
//
LPCTSTR CStringT::SubStr(DWORD nStart, DWORD nLength)
{
	return SubString(nStart, nStart + nLength - 1);
}

//
//   ����: LPCTSTR CStringT::LeftStr(DWORD nLength)
//
//   Ŀ��: ����߽�ȡ�ַ�����nLength ָ����ȡ����
//
//   ע��: �˲�������ı䱾�ַ�������������һ�����ַ�������������ӵ��ַ�����¼����
//         ����ʹ���ַ�������Ϊ��ֵ�����ܴ��ַ�����������ܻ�����ڴ��˷ѣ�����й¶��
//
LPCTSTR CStringT::LeftStr(DWORD nLength)
{
	return SubString(0, nLength - 1);
}

//
//   ����: LPCTSTR CStringT::RightStr(DWORD nLength)
//
//   Ŀ��: ���ұ߽�ȡ�ַ�����nLength ָ����ȡ����
//
//   ע��: �˲�������ı䱾�ַ�������������һ�����ַ�������������ӵ��ַ�����¼����
//         ����ʹ���ַ�������Ϊ��ֵ�����ܴ��ַ�����������ܻ�����ڴ��˷ѣ�����й¶��
//
LPCTSTR CStringT::RightStr(DWORD nLength)
{
	DWORD dwLen = _tcslen(m_pStrings[m_dwStringId].lpBuffer);
	return SubString(dwLen - nLength, dwLen - 1);
}

//
//   ����: CStringT &CStringT::Trim()
//
//   Ŀ��: ȥ���ַ������ߵĿո�
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

			// ȥ����ߵĿո���Ҫ�ƶ�������ַ���
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
//   ����: CStringT &CStringT::TrimLeft()
//
//   Ŀ��: ȥ���ַ�����ߵĿո�
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
//   ����: CStringT &CStringT::TrimLeft()
//
//   Ŀ��: ȥ���ַ����ұߵĿո�
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
//   ����: CStringT &CStringT::FormatImpl(LPCTSTR lpFormat, va_list arglist)
//
//   Ŀ��: �ַ�����ʽ��������ʵ��
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
//   ����: CStringT::operator LPCWSTR()
//
//   Ŀ��: ����ת����������أ����ַ�������Ϊ������ֵʱ�����ַ�����������ַ
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
//   ����: CStringT::operator LPCSTR()
//
//   Ŀ��: ����ת����������أ����ַ�������Ϊ������ֵʱ�����ַ�����������ַ
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