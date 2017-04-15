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

#define STRINGS_CAP_INCREASEMENT	100			// 字符串记录表容量增量
#define	STRING_ALLOCATE_FAILURE	(DWORD) -1		// 无效的字符串记录 ID
#define STRING_MIN_ALIGN_SIZE	64				// 字符串记录容量对齐值

typedef DWORD dword;

typedef class CStringT
{
protected:
	// 使用静态成员以使所有字符串类共享一个字符串记录表
	static LONG m_lInitRef;							// 类初始化计数器
	static CRITICAL_SECTION m_pCriticalSection;		// 临界区对象，用于将多线程调用线性化
	static PSTRING_RECORD m_pStrings;				// 字符串记录表
	static DWORD m_dwStringsCapacity;				// 字符串记录表容量
	static DWORD m_dwMaxPosition;					// 字符串记录表当前使用的最大值

	// 公用构造函数
	void CommonConstructor();

	// 销毁字符串表的记录
	void DestroyStringRecord(STRING_RECORD &pRecord);
	// 分配字符串记录
	DWORD AllocateRecord();
	// 分配字符串记录并赋值
	DWORD AllocateRecord(LPCTSTR lpString, int cchSize = -1);
	// 设置字符串记录容量
	void SetRecordCapacity(STRING_RECORD &pRecord, DWORD cchSize);
	// 设置字符串记录容量，通过 ID
	void SetRecordCapacity(DWORD dwRecordId, DWORD cchSize);
	// 设置字符串记录值
	BOOL SetRecordString(STRING_RECORD &pRecord, LPCTSTR lpString, int cchSize = -1);
	// 设置字符串记录值，通过 ID
	BOOL SetRecordString(DWORD dwRecordId, LPCTSTR lpString, int cchSize = -1);
	// 增加字符串记录引用次数
	void AddReferenceCount(STRING_RECORD &pRecord);
	// 增加字符串记录引用次数，通过 ID
	void AddReferenceCount(DWORD dwRecordId);
	// 减少字符串记录引用次数
	void ReleaseReferenceCount(STRING_RECORD &pRecord);
	// 减少字符串记录引用次数，通过 ID
	void ReleaseReferenceCount(DWORD dwRecordId);
	// 查找字符串记录，通过指针比较
	DWORD FindRecord(LPCTSTR lpAddress);
	// 锁定字符串记录，将多线程操作线性化
	void Lock();
	// 解锁字符串记录
	void Unlock();

public:
	// 字符串记录表的起始地址，用于判断两个字符串类是否使用同一个字符串记录表
	LPCVOID RefAddress() const;

private:
	// 用于对齐长度
	inline DWORD AlignSize(DWORD nSize)
	{
		DWORD dwRest = nSize % STRING_MIN_ALIGN_SIZE;

		if (dwRest) nSize += STRING_MIN_ALIGN_SIZE - dwRest;

		return nSize;
	}

protected:
	DWORD m_dwStringId;			// 当前使用的字符串记录 ID

#ifdef UNICODE
	LPSTR m_lpStrConv;			// 用于获取字符串值时进行类型转换的临时存储
#else
	LPWSTR m_lpStrConv;
#endif
	DWORD m_cchStrConv;			// 临时存储字符串的容量
	BOOL m_bStrConvModified;	// 字符串记录的内容是否被修改过

public:
	// 默认构造函数
	CStringT();
	// 构造拷贝函数
	CStringT(const CStringT &s);
	// 构造并赋值，Unicode 字符串，可指定长度
	CStringT(LPCWSTR lpString, int cchSize);
	// 构造并赋值，Unicode 字符串
	CStringT(LPCWSTR lpString);
	// 构造并赋值，ANSI 字符串，可指定长度
	CStringT(LPCSTR lpString, int cchSize);
	// 构造并赋值，ANSI 字符串
	CStringT(LPCSTR lpString);
	// 构造并填充字符
	CStringT(const int Ch, size_t nCount);
	// 构造并将字符串值设置为一个字符
	CStringT(const int Ch);
	// 析构函数
	~CStringT();

private:
	// 将 ANSI 字符串转换为 Unicode 字符串，需释放返回字符串
	LPWSTR ToWide(LPCSTR lpString);
	// 将 Unicode 字符串转换为 ANSI 字符串，需释放返回字符串
	LPSTR ToAnsi(LPCWSTR lpString);
	// 释放字符串
	void FreeConv(PVOID pAddress);
	
	// 字符串填充的实现
	void FillImpl(LPCTSTR lpString, DWORD nCount);
	// 字符串比较的实现
	int CompareImpl(LPCTSTR lpString, BOOL bIgnoreCase = FALSE);
	// 字符串追加的实现
	CStringT &AppendImpl(LPCTSTR lpString);
	// 字符串合并的实现
	LPCTSTR ConcatImpl(LPCTSTR lpString);
	// 字符串查找的实现
	DWORD FindImpl(LPCTSTR lpString, DWORD nPos = 0, BOOL bReverse = FALSE, BOOL bIgnoreCase = FALSE);
	// 字符串替换的实现
	CStringT &ReplaceImpl(LPCTSTR lpFind, LPCTSTR lpReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	// 字符串选段替换的实现
	CStringT &ReplaceImpl(DWORD nStart, DWORD nLength, LPCTSTR lpReplace);
	// 字符串格式化的实现
	CStringT &FormatImpl(LPCTSTR lpFormat, va_list arglist);
	// 构造拷贝（类拷贝）的实现
	CStringT &ConstructCopy(LPCVOID lpRefAddr, LPCTSTR lpString, DWORD dwStringId);

public:
	// 返回字符串记录 ID
	DWORD GetStringId();
	// 返回字符串缓冲区的起始地址
	LPCTSTR StringBuffer();
	
	// 在字符串记录被多次引用时，复制一份，使之脱离之前的引用
	// 在修改字符串前必须调用
	void Detach();
	
	// 获取字符串的某个字符
	TCHAR GetChar(DWORD nIndex);
	// 设置字符串的某个字符
	void SetChar(DWORD nIndex, TCHAR cNewChar);

	// 获取字符串长度
	DWORD GetLength();
	// 获取字符串缓冲区容量
	DWORD GetCapacity();
	// 设置字符串缓冲区容量
	void SetCapacity(DWORD dwLen);

	// 反转字符串
	void Reverse();
	// 将字符串转换为小写
	void ToLowerCase();
	// 将字符串转换为大写
	void ToUpperCase();

	// 填充字符串
	void Fill(LPCWSTR lpString, DWORD nCount);
	void Fill(LPCSTR lpString, DWORD nCount);
	void Fill(const int Ch, DWORD nCount);

	// 字符串比较
	int Compare(LPCWSTR lpString, BOOL bIgnoreCase = FALSE);
	int Compare(LPCSTR lpString, BOOL bIgnoreCase = FALSE);
	int Compare(const int Ch, BOOL bIgnoreCase = FALSE);

	// 设置字符串的值
	CStringT &SetString(LPCWSTR lpString, int cchSize = -1);
	CStringT &SetString(LPCSTR lpString, int cchSize = -1);
	CStringT &SetString(const int Ch);

	// 字符串追加值
	CStringT &Append(LPCWSTR lpString);
	CStringT &Append(LPCSTR lpString);
	CStringT &Append(const int Ch);

	// 字符串连接，返回新字符串，不改变现有字符串
	LPCTSTR Concat(LPCWSTR lpString);
	LPCTSTR Concat(LPCSTR lpString);
	LPCTSTR Concat(const int Ch);
	
	// 字符串查找
	DWORD Find(LPCWSTR lpString, DWORD nPos = 0, BOOL bReverse = FALSE, BOOL bIgnoreCase = FALSE);
	DWORD Find(LPCSTR lpString, DWORD nPos = 0, BOOL bReverse = FALSE, BOOL bIgnoreCase = FALSE);
	DWORD Find(const int Ch, DWORD nPos = 0, BOOL bReverse = FALSE, BOOL bIgnoreCase = FALSE);

	// 字符串替换，将指定字符串替换为另一个字符串
	CStringT &Replace(LPCWSTR lpFind, LPCWSTR lpReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(LPCSTR lpFind, LPCSTR lpReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(LPCWSTR lpFind, LPCSTR lpReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(LPCSTR lpFind, LPCWSTR lpReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(const int cFind, LPCWSTR lpReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(const int cFind, LPCSTR lpReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(LPCWSTR lpFind, const int cReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(LPCSTR lpFind, const int cReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	CStringT &Replace(const int cFind, const int cReplace, DWORD nStart = 0, BOOL bReplaceOnce = FALSE, BOOL bIgnoreCase = FALSE);
	// 字符串替换，将某一段字符串替换为另一个字符串
	CStringT &Replace(DWORD nStart, DWORD nLength, LPCWSTR lpReplace);
	CStringT &Replace(DWORD nStart, DWORD nLength, LPCSTR lpReplace);
	CStringT &Replace(DWORD nStart, DWORD nLength, const int cReplace);

	// 字符串插入
	CStringT &Insert(DWORD nStart, LPCWSTR lpString);
	CStringT &Insert(DWORD nStart, LPCSTR lpString);
	CStringT &Insert(DWORD nStart, const int Ch);

	// 删除某一段字符串
	CStringT &Delete(DWORD nStart, DWORD nLength);

	// 获取子串，从 nStart 到 nEnd
	LPCTSTR SubString(DWORD nStart, DWORD nEnd);
	// 获取子串，从 nStart 开始的 nLength 个
	LPCTSTR SubStr(DWORD nStart, DWORD nLength);
	// 获取左边 nLength 个字符的子串
	LPCTSTR LeftStr(DWORD nLength);
	// 获取右边 nLength 个字符的子串
	LPCTSTR RightStr(DWORD nLength);

	// 去除字符串两边的空格
	CStringT &Trim();
	// 去除字符串左边的空格
	CStringT &TrimLeft();
	// 去除字符串右边的空格
	CStringT &TrimRight();

	// 字符串格式化
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

	// 类型转换运算符重载，在字符串类作为右值时使用
	operator LPCWSTR();
	operator LPCSTR();

	__declspec( property( get=GetLength )) DWORD Length;
	__declspec( property( get=StringBuffer )) LPTSTR Buffer;


} CStringT;

#undef CStringT
#undef STRING_RECORD
#undef PSTRING_RECORD