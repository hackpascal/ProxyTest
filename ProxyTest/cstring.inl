#undef LPTSTR
#undef LPCTSTR
#undef TCHAR

#ifdef UNICODE
#define CStringT	CStringW
#define	STRING_RECORD	STRING_RECORDW
#define	PSTRING_RECORD	PSTRING_RECORDW
#define LPTSTR	LPWSTR
#define LPCTSTR	LPCWSTR
#define TCHAR WCHAR
#else
#define CStringT	CStringA
#define	STRING_RECORD	STRING_RECORDA
#define	PSTRING_RECORD	PSTRING_RECORDA
#define LPTSTR	LPSTR
#define LPCTSTR	LPCSTR
#define TCHAR CHAR
#endif

typedef struct STRING_RECORD
{
	LPTSTR lpBuffer;
	DWORD dwCapacity;
	DWORD dwRefCount;
	BOOL bFakeRef;
} STRING_RECORD, *PSTRING_RECORD;

#define STRINGS_CAP_INCREASEMENT	100			// �ַ�����¼����������
#define	STRING_ALLOCATE_FAILURE	(DWORD) -1		// ��Ч���ַ�����¼ ID
#define STRING_MIN_ALIGN_SIZE	64				// �ַ�����¼��������ֵ

typedef DWORD dword;

typedef class CStringT
{
protected:
	// ʹ�þ�̬��Ա��ʹ�����ַ����๲��һ���ַ�����¼��
	static LONG m_lInitRef;							// ���ʼ��������
	static CRITICAL_SECTION m_pCriticalSection;		// �ٽ����������ڽ����̵߳������Ի�
	static PSTRING_RECORD m_pStrings;				// �ַ�����¼��
	static DWORD m_dwStringsCapacity;				// �ַ�����¼������
	static DWORD m_dwMaxPosition;					// �ַ�����¼��ǰʹ�õ����ֵ

	// ���ù��캯��
	void CommonConstructor();

	// �����ַ�����ļ�¼
	void DestroyStringRecord(STRING_RECORD &pRecord);
	// �����ַ�����¼
	DWORD AllocateRecord();
	// �����ַ�����¼����ֵ
	DWORD AllocateRecord(LPCTSTR lpString, int cchSize = -1);
	// �����ַ�����¼����
	void SetRecordCapacity(STRING_RECORD &pRecord, DWORD cchSize);
	// �����ַ�����¼������ͨ�� ID
	void SetRecordCapacity(DWORD dwRecordId, DWORD cchSize);
	// �����ַ�����¼ֵ
	BOOL SetRecordString(STRING_RECORD &pRecord, LPCTSTR lpString, int cchSize = -1);
	// �����ַ�����¼ֵ��ͨ�� ID
	BOOL SetRecordString(DWORD dwRecordId, LPCTSTR lpString, int cchSize = -1);
	// �����ַ�����¼���ô���
	void AddReferenceCount(STRING_RECORD &pRecord);
	// �����ַ�����¼���ô�����ͨ�� ID
	void AddReferenceCount(DWORD dwRecordId);
	// �����ַ�����¼���ô���
	void ReleaseReferenceCount(STRING_RECORD &pRecord);
	// �����ַ�����¼���ô�����ͨ�� ID
	void ReleaseReferenceCount(DWORD dwRecordId);
	// �����ַ�����¼��ͨ��ָ��Ƚ�
	DWORD FindRecord(LPCTSTR lpAddress);
	// �����ַ�����¼�������̲߳������Ի�
	void Lock();
	// �����ַ�����¼
	void Unlock();

public:
	// �ַ�����¼�����ʼ��ַ�������ж������ַ������Ƿ�ʹ��ͬһ���ַ�����¼��
	LPCVOID RefAddress() const;

private:
	// ���ڶ��볤��
	inline DWORD AlignSize(DWORD nSize)
	{
		DWORD dwRest = nSize % STRING_MIN_ALIGN_SIZE;

		if (dwRest) nSize += STRING_MIN_ALIGN_SIZE - dwRest;

		return nSize;
	}

protected:
	DWORD m_dwStringId;			// ��ǰʹ�õ��ַ�����¼ ID

#ifdef UNICODE
	LPSTR m_lpStrConv;			// ���ڻ�ȡ�ַ���ֵʱ��������ת������ʱ�洢
#else
	LPWSTR m_lpStrConv;
#endif
	DWORD m_cchStrConv;			// ��ʱ�洢�ַ���������
	BOOL m_bStrConvModified;	// �ַ�����¼�������Ƿ��޸Ĺ�

public:
	// Ĭ�Ϲ��캯��
	CStringT();
	// ���쿽������
	CStringT(const CStringT &s);
	// ���첢��ֵ��Unicode �ַ�������ָ������
	CStringT(LPCWSTR lpString, int cchSize);
	// ���첢��ֵ��Unicode �ַ���
	CStringT(LPCWSTR lpString);
	// ���첢��ֵ��ANSI �ַ�������ָ������
	CStringT(LPCSTR lpString, int cchSize);
	// ���첢��ֵ��ANSI �ַ���
	CStringT(LPCSTR lpString);
	// ���첢����ַ�
	CStringT(const int Ch, size_t nCount);
	// ���첢���ַ���ֵ����Ϊһ���ַ�
	CStringT(const int Ch);
	// ��������
	~CStringT();

private:
	// �� ANSI �ַ���ת��Ϊ Unicode �ַ��������ͷŷ����ַ���
	LPWSTR ToWide(LPCSTR lpString);
	// �� Unicode �ַ���ת��Ϊ ANSI �ַ��������ͷŷ����ַ���
	LPSTR ToAnsi(LPCWSTR lpString);
	// �ͷ��ַ���
	void FreeConv(PVOID pAddress);
	
	// �ַ�������ʵ��
	void FillImpl(LPCTSTR lpString, DWORD nCount);
	// �ַ����Ƚϵ�ʵ��
	int CompareImpl(LPCTSTR lpString, BOOL bIgnoreCase = FALSE);
	// �ַ���׷�ӵ�ʵ��
	CStringT &AppendImpl(LPCTSTR lpString);
	// �ַ����ϲ���ʵ��
	LPCTSTR ConcatImpl(LPCTSTR lpString);
	// �ַ������ҵ�ʵ��
	DWORD FindImpl(LPCTSTR lpString, DWORD nPos = 0, BOOL bReverse = FALSE, BOOL bIgnoreCase = FALSE);
	// �ַ����滻��ʵ��
	CStringT &ReplaceImpl(LPCTSTR lpFind, LPCTSTR lpReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	// �ַ���ѡ���滻��ʵ��
	CStringT &ReplaceImpl(DWORD nStart, DWORD nLength, LPCTSTR lpReplace);
	// �ַ�����ʽ����ʵ��
	CStringT &FormatImpl(LPCTSTR lpFormat, va_list arglist);
	// ���쿽�����࿽������ʵ��
	CStringT &ConstructCopy(LPCVOID lpRefAddr, LPCTSTR lpString, DWORD dwStringId);

public:
	// �����ַ�����¼ ID
	DWORD GetStringId();
	// �����ַ�������������ʼ��ַ
	LPCTSTR StringBuffer();
	
	// ���ַ�����¼���������ʱ������һ�ݣ�ʹ֮����֮ǰ������
	// ���޸��ַ���ǰ�������
	void Detach();
	
	// ��ȡ�ַ�����ĳ���ַ�
	TCHAR GetChar(DWORD nIndex);
	// �����ַ�����ĳ���ַ�
	void SetChar(DWORD nIndex, TCHAR cNewChar);

	// ��ȡ�ַ�������
	DWORD GetLength();
	// ��ȡ�ַ�������������
	DWORD GetCapacity();
	// �����ַ�������������
	void SetCapacity(DWORD dwLen);

	// ��ת�ַ���
	void Reverse();
	// ���ַ���ת��ΪСд
	void ToLowerCase();
	// ���ַ���ת��Ϊ��д
	void ToUpperCase();

	// ����ַ���
	void Fill(LPCWSTR lpString, DWORD nCount);
	void Fill(LPCSTR lpString, DWORD nCount);
	void Fill(const int Ch, DWORD nCount);

	// �ַ����Ƚ�
	int Compare(LPCWSTR lpString, BOOL bIgnoreCase = FALSE);
	int Compare(LPCSTR lpString, BOOL bIgnoreCase = FALSE);
	int Compare(const int Ch, BOOL bIgnoreCase = FALSE);

	// �����ַ�����ֵ
	CStringT &SetString(LPCWSTR lpString, int cchSize = -1);
	CStringT &SetString(LPCSTR lpString, int cchSize = -1);
	CStringT &SetString(const int Ch);

	// �ַ���׷��ֵ
	CStringT &Append(LPCWSTR lpString);
	CStringT &Append(LPCSTR lpString);
	CStringT &Append(const int Ch);

	// �ַ������ӣ��������ַ��������ı������ַ���
	LPCTSTR Concat(LPCWSTR lpString);
	LPCTSTR Concat(LPCSTR lpString);
	LPCTSTR Concat(const int Ch);
	
	// �ַ�������
	DWORD Find(LPCWSTR lpString, DWORD nPos = 0, BOOL bReverse = FALSE, BOOL bIgnoreCase = FALSE);
	DWORD Find(LPCSTR lpString, DWORD nPos = 0, BOOL bReverse = FALSE, BOOL bIgnoreCase = FALSE);
	DWORD Find(const int Ch, DWORD nPos = 0, BOOL bReverse = FALSE, BOOL bIgnoreCase = FALSE);

	// �ַ����滻����ָ���ַ����滻Ϊ��һ���ַ���
	CStringT &Replace(LPCWSTR lpFind, LPCWSTR lpReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(LPCSTR lpFind, LPCSTR lpReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(LPCWSTR lpFind, LPCSTR lpReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(LPCSTR lpFind, LPCWSTR lpReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(const int cFind, LPCWSTR lpReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(const int cFind, LPCSTR lpReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(LPCWSTR lpFind, const int cReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(LPCSTR lpFind, const int cReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(const int cFind, const int cReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	// �ַ����滻����ĳһ���ַ����滻Ϊ��һ���ַ���
	CStringT &Replace(DWORD nStart, DWORD nLength, LPCWSTR lpReplace);
	CStringT &Replace(DWORD nStart, DWORD nLength, LPCSTR lpReplace);
	CStringT &Replace(DWORD nStart, DWORD nLength, const int cReplace);

	// �ַ�������
	CStringT &Insert(DWORD nStart, LPCWSTR lpString);
	CStringT &Insert(DWORD nStart, LPCSTR lpString);
	CStringT &Insert(DWORD nStart, const int Ch);

	// ɾ��ĳһ���ַ���
	CStringT &Delete(DWORD nStart, DWORD nLength);

	// ��ȡ�Ӵ����� nStart �� nEnd
	LPCTSTR SubString(DWORD nStart, DWORD nEnd);
	// ��ȡ�Ӵ����� nStart ��ʼ�� nLength ��
	LPCTSTR SubStr(DWORD nStart, DWORD nLength);
	// ��ȡ��� nLength ���ַ����Ӵ�
	LPCTSTR LeftStr(DWORD nLength);
	// ��ȡ�ұ� nLength ���ַ����Ӵ�
	LPCTSTR RightStr(DWORD nLength);

	// ȥ���ַ������ߵĿո�
	CStringT &Trim();
	// ȥ���ַ�����ߵĿո�
	CStringT &TrimLeft();
	// ȥ���ַ����ұߵĿո�
	CStringT &TrimRight();

	// �ַ�����ʽ��
	CStringT &Format(LPCWSTR lpFormat, ...);
	CStringT &Format(LPCSTR lpFormat, ...);

	CStringT &operator =(const CStringT &s){ return ConstructCopy(const_cast<CStringT *>(&s)->RefAddress(), const_cast<CStringT *>(&s)->StringBuffer(), const_cast<CStringT *>(&s)->GetStringId()); }
	CStringT &operator =(LPCWSTR lpString){ return SetString(lpString); }
	CStringT &operator =(LPCSTR lpString){ return SetString(lpString); }
	CStringT &operator =(const int Ch){ return SetString(Ch); }

	LPCTSTR operator +(const CStringT &s){ return Concat(const_cast<CStringT *>(&s)->StringBuffer()); }
	LPCTSTR operator +(LPCWSTR s){ return Concat(s); }
	LPCTSTR operator +(LPCSTR s){ return Concat(s); }
	LPCTSTR operator +(const int Ch){ return Concat(Ch); }

	CStringT &operator +=(const CStringT &s){ return Append(const_cast<CStringT *>(&s)->StringBuffer()); }
	CStringT &operator +=(LPCWSTR s){ return Append(s); }
	CStringT &operator +=(LPCSTR s){ return Append(s); }
	CStringT &operator +=(const int Ch){ return Append(Ch); }

	bool operator ==(const CStringT &s){ return Compare(const_cast<CStringT *>(&s)->StringBuffer(), FALSE) == 0; }
	bool operator ==(LPCWSTR s){ return Compare(s, FALSE) == 0; }
	bool operator ==(LPCSTR s){ return Compare(s, FALSE) == 0; }
	bool operator ==(const int Ch){ return Compare(Ch, FALSE) == 0; }

	bool operator !=(const CStringT &s){ return Compare(const_cast<CStringT *>(&s)->StringBuffer(), FALSE) != 0; }
	bool operator !=(LPCWSTR s){ return Compare(s, FALSE) != 0; }
	bool operator !=(LPCSTR s){ return Compare(s, FALSE) != 0; }
	bool operator !=(const int Ch){ return Compare(Ch, FALSE) != 0; }

	bool operator >(const CStringT &s){ return Compare(const_cast<CStringT *>(&s)->StringBuffer(), FALSE) > 0; }
	bool operator >(LPCWSTR s){ return Compare(s, FALSE) > 0; }
	bool operator >(LPCSTR s){ return Compare(s, FALSE) > 0; }
	bool operator >(const int Ch){ return Compare(Ch, FALSE) > 0; }

	bool operator >=(const CStringT &s){ return Compare(const_cast<CStringT *>(&s)->StringBuffer(), FALSE) >= 0; }
	bool operator >=(LPCWSTR s){ return Compare(s, FALSE) >= 0; }
	bool operator >=(LPCSTR s){ return Compare(s, FALSE) >= 0; }
	bool operator >=(const int Ch){ return Compare(Ch, FALSE) >= 0; }

	bool operator <(const CStringT &s){ return Compare(const_cast<CStringT *>(&s)->StringBuffer(), FALSE) < 0; }
	bool operator <(LPCWSTR s){ return Compare(s, FALSE) < 0; }
	bool operator <(LPCSTR s){ return Compare(s, FALSE) < 0; }
	bool operator <(const int Ch){ return Compare(Ch, FALSE) < 0; }

	bool operator <=(const CStringT &s){ return Compare(const_cast<CStringT *>(&s)->StringBuffer(), FALSE) <= 0; }
	bool operator <=(LPCWSTR s){ return Compare(s, FALSE) <= 0; }
	bool operator <=(LPCSTR s){ return Compare(s, FALSE) <= 0; }
	bool operator <=(const int Ch){ return Compare(Ch, FALSE) <= 0; }

	// ����ת����������أ����ַ�������Ϊ��ֵʱʹ��
	operator LPCWSTR();
	operator LPCSTR();

	__declspec( property( get=GetLength )) DWORD Length;
	__declspec( property( get=StringBuffer )) LPTSTR Buffer;


} CStringT;

#undef CStringT
#undef STRING_RECORD
#undef PSTRING_RECORD