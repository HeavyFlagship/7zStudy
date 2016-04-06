/* 7zMain.c - Test application for 7z Decoder
2015-08-02 : Igor Pavlov : Public domain */

#include "Precomp.h"

#include <stdio.h>
#include <string.h>

#include "../../7z.h"
#include "../../7zAlloc.h"
#include "../../7zBuf.h"
#include "../../7zCrc.h"
#include "../../7zFile.h"
#include "../../7zVersion.h"

#ifndef USE_WINDOWS_FILE
/* for mkdir */
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <errno.h>
#endif
#endif

static ISzAlloc g_Alloc = { SzAlloc, SzFree };

//字面意思是确保缓存大小
static int Buf_EnsureSize(CBuf *dest, size_t size)
{
	if (dest->size >= size)
		return 1;
	Buf_Free(dest, &g_Alloc);
	return Buf_Create(dest, size, &g_Alloc);
}

#ifndef _WIN32
#define _USE_UTF8
#endif

/* #define _USE_UTF8 */

#ifdef _USE_UTF8

#define _UTF8_START(n) (0x100 - (1 << (7 - (n))))

#define _UTF8_RANGE(n) (((UInt32)1) << ((n) * 5 + 6))

#define _UTF8_HEAD(n, val) ((Byte)(_UTF8_START(n) + (val >> (6 * (n)))))
#define _UTF8_CHAR(n, val) ((Byte)(0x80 + (((val) >> (6 * (n))) & 0x3F)))

static size_t Utf16_To_Utf8_Calc(const UInt16 *src, const UInt16 *srcLim)
{
	size_t size = 0;
	for (;;)
	{
		UInt32 val;
		if (src == srcLim)
			return size;

		size++;
		val = *src++;

		if (val < 0x80)
			continue;

		if (val < _UTF8_RANGE(1))
		{
			size++;
			continue;
		}

		if (val >= 0xD800 && val < 0xDC00 && src != srcLim)
		{
			UInt32 c2 = *src;
			if (c2 >= 0xDC00 && c2 < 0xE000)
			{
				src++;
				size += 3;
				continue;
			}
		}

		size += 2;
	}
}

static Byte *Utf16_To_Utf8(Byte *dest, const UInt16 *src, const UInt16 *srcLim)
{
	for (;;)
	{
		UInt32 val;
		if (src == srcLim)
			return dest;

		val = *src++;

		if (val < 0x80)
		{
			*dest++ = (char)val;
			continue;
		}

		if (val < _UTF8_RANGE(1))
		{
			dest[0] = _UTF8_HEAD(1, val);
			dest[1] = _UTF8_CHAR(0, val);
			dest += 2;
			continue;
		}

		if (val >= 0xD800 && val < 0xDC00 && src != srcLim)
		{
			UInt32 c2 = *src;
			if (c2 >= 0xDC00 && c2 < 0xE000)
			{
				src++;
				val = (((val - 0xD800) << 10) | (c2 - 0xDC00)) + 0x10000;
				dest[0] = _UTF8_HEAD(3, val);
				dest[1] = _UTF8_CHAR(2, val);
				dest[2] = _UTF8_CHAR(1, val);
				dest[3] = _UTF8_CHAR(0, val);
				dest += 4;
				continue;
			}
		}

		dest[0] = _UTF8_HEAD(2, val);
		dest[1] = _UTF8_CHAR(1, val);
		dest[2] = _UTF8_CHAR(0, val);
		dest += 3;
	}
}

static SRes Utf16_To_Utf8Buf(CBuf *dest, const UInt16 *src, size_t srcLen)
{
	size_t destLen = Utf16_To_Utf8_Calc(src, src + srcLen);
	destLen += 1;
	if (!Buf_EnsureSize(dest, destLen))
		return SZ_ERROR_MEM;
	*Utf16_To_Utf8(dest->data, src, src + srcLen) = 0;
	return SZ_OK;
}

#endif

static SRes Utf16_To_Char(CBuf *buf, const UInt16 *s
#ifndef _USE_UTF8
	, UINT codePage
#endif
	)
{
	unsigned len = 0;
	for (len = 0; s[len] != 0; len++);

#ifndef _USE_UTF8
	{
		unsigned size = len * 3 + 100;
		if (!Buf_EnsureSize(buf, size))
			return SZ_ERROR_MEM;
		{
			buf->data[0] = 0;
			if (len != 0)
			{
				char defaultChar = '_';
				BOOL defUsed;
				unsigned numChars = 0;
				numChars = WideCharToMultiByte(codePage, 0, s, len, (char *)buf->data, size, &defaultChar, &defUsed);
				if (numChars == 0 || numChars >= size)
					return SZ_ERROR_FAIL;
				buf->data[numChars] = 0;
			}
			return SZ_OK;
		}
	}
#else
	return Utf16_To_Utf8Buf(buf, s, len);
#endif
}

#ifdef _WIN32
#ifndef USE_WINDOWS_FILE
static UINT g_FileCodePage = CP_ACP;
#endif
#define MY_FILE_CODE_PAGE_PARAM ,g_FileCodePage
#else
#define MY_FILE_CODE_PAGE_PARAM
#endif
//字面意思，创建路径文件夹
static WRes MyCreateDir(const UInt16 *name)
{
#ifdef USE_WINDOWS_FILE

	return CreateDirectoryW(name, NULL) ? 0 : GetLastError();

#else

	CBuf buf;
	WRes res;
	Buf_Init(&buf);
	RINOK(Utf16_To_Char(&buf, name MY_FILE_CODE_PAGE_PARAM));

	res =
#ifdef _WIN32
		_mkdir((const char *)buf.data)
#else
		mkdir((const char *)buf.data, 0777)
#endif
		== 0 ? 0 : errno;
	Buf_Free(&buf, &g_Alloc);
	return res;

#endif
}
//字面意思输出文件，文件名是UTF16字符串
static WRes OutFile_OpenUtf16(CSzFile *p, const UInt16 *name)
{
#ifdef USE_WINDOWS_FILE
	return OutFile_OpenW(p, name);
#else
	CBuf buf;
	WRes res;
	Buf_Init(&buf);
	RINOK(Utf16_To_Char(&buf, name MY_FILE_CODE_PAGE_PARAM));
	res = OutFile_Open(p, (const char *)buf.data);
	Buf_Free(&buf, &g_Alloc);
	return res;
#endif
}
//字面意思，打印字符串
static SRes PrintString(const UInt16 *s)
{
	CBuf buf;
	SRes res;
	Buf_Init(&buf);
	res = Utf16_To_Char(&buf, s
#ifndef _USE_UTF8
		, CP_OEMCP
#endif
		);
	if (res == SZ_OK)
		fputs((const char *)buf.data, stdout);
	Buf_Free(&buf, &g_Alloc);
	return res;
}
//把无符号64位长整型数，转换成字符串表示
static void UInt64ToStr(UInt64 value, char *s)
{
	char temp[32];
	int pos = 0;
	do
	{
		temp[pos++] = (char)('0' + (unsigned)(value % 10));
		value /= 10;
	} while (value != 0);
	do
		*s++ = temp[--pos];
	while (pos);
	*s = '\0';
}
//无符号整型数，转化成字符串表示，带有效数字
static char *UIntToStr(char *s, unsigned value, int numDigits)
{
	char temp[16];
	int pos = 0;
	do
		temp[pos++] = (char)('0' + (value % 10));
	while (value /= 10);
	for (numDigits -= pos; numDigits > 0; numDigits--)
		*s++ = '0';
	do
		*s++ = temp[--pos];
	while (pos);
	*s = '\0';
	return s;
}
//两位数整形，转换为字符串
static void UIntToStr_2(char *s, unsigned value)
{
	s[0] = (char)('0' + (value / 10));
	s[1] = (char)('0' + (value % 10));
}

#define PERIOD_4 (4 * 365 + 1)
#define PERIOD_100 (PERIOD_4 * 25 - 1)
#define PERIOD_400 (PERIOD_100 * 4 + 1)
//将文件时间（不确定）转换为字符串
static void ConvertFileTimeToString(const CNtfsFileTime *nt, char *s)
{
	unsigned year, mon, hour, min, sec;
	Byte ms[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	unsigned t;
	UInt32 v;
	UInt64 v64 = nt->Low | ((UInt64)nt->High << 32);
	v64 /= 10000000;
	sec = (unsigned)(v64 % 60); v64 /= 60;
	min = (unsigned)(v64 % 60); v64 /= 60;
	hour = (unsigned)(v64 % 24); v64 /= 24;

	v = (UInt32)v64;

	year = (unsigned)(1601 + v / PERIOD_400 * 400);
	v %= PERIOD_400;

	t = v / PERIOD_100; if (t == 4) t = 3; year += t * 100; v -= t * PERIOD_100;
	t = v / PERIOD_4;   if (t == 25) t = 24; year += t * 4;   v -= t * PERIOD_4;
	t = v / 365;        if (t == 4) t = 3; year += t;       v -= t * 365;

	if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
		ms[1] = 29;
	for (mon = 0;; mon++)
	{
		unsigned s = ms[mon];
		if (v < s)
			break;
		v -= s;
	}
	s = UIntToStr(s, year, 4); *s++ = '-';
	UIntToStr_2(s, mon + 1); s[2] = '-'; s += 3;
	UIntToStr_2(s, (unsigned)v + 1); s[2] = ' '; s += 3;
	UIntToStr_2(s, hour); s[2] = ':'; s += 3;
	UIntToStr_2(s, min); s[2] = ':'; s += 3;
	UIntToStr_2(s, sec); s[2] = 0;
}
//输出错误代码
void PrintError(char *sz)
{
	printf("\nERROR: %s\n", sz);
}
//获取属性字符串
static void GetAttribString(UInt32 wa, Bool isDir, char *s)
{
#ifdef USE_WINDOWS_FILE
	s[0] = (char)(((wa & FILE_ATTRIBUTE_DIRECTORY) != 0 || isDir) ? 'D' : '.');
	s[1] = (char)(((wa & FILE_ATTRIBUTE_READONLY) != 0) ? 'R' : '.');
	s[2] = (char)(((wa & FILE_ATTRIBUTE_HIDDEN) != 0) ? 'H' : '.');
	s[3] = (char)(((wa & FILE_ATTRIBUTE_SYSTEM) != 0) ? 'S' : '.');
	s[4] = (char)(((wa & FILE_ATTRIBUTE_ARCHIVE) != 0) ? 'A' : '.');
	s[5] = 0;
#else
	s[0] = (char)(((wa & (1 << 4)) != 0 || isDir) ? 'D' : '.');
	s[1] = 0;
#endif
}

// #define NUM_PARENTS_MAX 128
//主函数
int MY_CDECL main(int numargs, char *args[])
{
	CFileInStream archiveStream;		//可能是用于储存压缩文件相关的数据
	CLookToRead lookStream;				//不知道是啥
	CSzArEx db;							//应该是储存压缩文件的结构体
	SRes res;							//函数报错临时变量
	ISzAlloc allocImp;					//主内存池分配函数
	ISzAlloc allocTempImp;				//临时内存池分配函数
	UInt16 *temp = NULL;				//主要用于临时储存压缩文件中文件名，应该是相对路径。
	size_t tempSize = 0;				//可能是临时储存数组大小
	// UInt32 parents[NUM_PARENTS_MAX];

	printf("\n7z ANSI-C Decoder " MY_VERSION_COPYRIGHT_DATE "\n\n");

	if (numargs == 1)
	{
		printf(
			"Usage: 7zDec <command> <archive_name>\n\n"
			"<Commands>\n"
			"  e: Extract files from archive (without using directory names)\n"
			"  l: List contents of archive\n"
			"  t: Test integrity of archive\n"
			"  x: eXtract files with full paths\n");
		return 0;
	}

	if (numargs < 3)
	{
		PrintError("incorrect command");
		return 1;
	}

#if defined(_WIN32) && !defined(USE_WINDOWS_FILE) && !defined(UNDER_CE)
	g_FileCodePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
#endif
	//分配内存空间的函数指针赋值
	allocImp.Alloc = SzAlloc;
	allocImp.Free = SzFree;

	allocTempImp.Alloc = SzAllocTemp;
	allocTempImp.Free = SzFreeTemp;
	//打开文件
#ifdef UNDER_CE
	if (InFile_OpenW(&archiveStream.file, L"\test.7z"))
#else
	if (InFile_Open(&archiveStream.file, args[2]))
#endif
	{
		PrintError("can not open input file");
		return 1;
	}
	//各种初始化
	FileInStream_CreateVTable(&archiveStream);
	LookToRead_CreateVTable(&lookStream, False);

	lookStream.realStream = &archiveStream.s;
	LookToRead_Init(&lookStream);

	CrcGenerateTable();

	SzArEx_Init(&db);

	res = SzArEx_Open(&db, &lookStream.s, &allocImp, &allocTempImp);
	//确定已经把压缩文件载入，决定后面的操作
	if (res == SZ_OK)
	{
		char *command = args[1];
		int listCommand = 0, testCommand = 0, fullPaths = 0;

		if (strcmp(command, "l") == 0) listCommand = 1;
		else if (strcmp(command, "t") == 0) testCommand = 1;
		else if (strcmp(command, "e") == 0) {}
		else if (strcmp(command, "x") == 0) { fullPaths = 1; }
		else
		{
			PrintError("incorrect command");
			res = SZ_ERROR_FAIL;
		}
		//判断指令无误，执行后面操作。
		if (res == SZ_OK)
		{
			UInt32 i;

			/*
			if you need cache, use these 3 variables.
			if you use external function, you can make these variable as static.
			如果你需要缓存，用这三个变量
			如果你需要用外部函数，你可以声明这三这个变量是静态的。
			*/
			UInt32 blockIndex = 0xFFFFFFFF;			/* it can have any value before first call (if outBuffer = 0) */
			Byte *outBuffer = 0;					/* it must be 0 before first call for each new archive. */
			size_t outBufferSize = 0;				/* it can have any value before first call (if outBuffer = 0) */

			//这里应该是按照压缩文件内的目录读取文件
			for (i = 0; i < db.NumFiles; i++)
			{
				size_t offset = 0;				//补偿
				size_t outSizeProcessed = 0;	//
				// const CSzFileItem *f = db.Files + i;
				size_t len;
				unsigned isDir = SzArEx_IsDir(&db, i);	//不清楚具体作用，可能用于判断当前数据段代表的是否是空文件夹
				/*这里判断是否是 既不是列表模式，也不是全路径解压模式时，该字段只是空文件夹的情况，
				若是这种情况则跳过该空文件夹，否则会继续进行，并列出文件夹列表或生成相应目录*/
				if (listCommand == 0 && isDir && !fullPaths)
					continue;
				len = SzArEx_GetFileNameUtf16(&db, i, NULL);
				// len = SzArEx_GetFullNameLen(&db, i);

				//这里重新分配压缩文件名数组的空间
				if (len > tempSize)
				{
					SzFree(NULL, temp);
					tempSize = len;
					temp = (UInt16 *)SzAlloc(NULL, tempSize * sizeof(temp[0]));
					if (!temp)
					{
						res = SZ_ERROR_MEM;
						break;
					}
				}

				SzArEx_GetFileNameUtf16(&db, i, temp);
				/*
				if (SzArEx_GetFullNameUtf16_Back(&db, i, temp + len) != temp)
				{
				  res = SZ_ERROR_FAIL;
				  break;
				}
				*/

				if (listCommand)
				{
					char attr[8], s[32], t[32];
					UInt64 fileSize;

					GetAttribString(SzBitWithVals_Check(&db.Attribs, i) ? db.Attribs.Vals[i] : 0, isDir, attr);

					fileSize = SzArEx_GetFileSize(&db, i);
					UInt64ToStr(fileSize, s);

					if (SzBitWithVals_Check(&db.MTime, i))
						ConvertFileTimeToString(&db.MTime.Vals[i], t);
					else
					{
						size_t j;
						for (j = 0; j < 19; j++)
							t[j] = ' ';
						t[j] = '\0';
					}

					printf("%s %s %10s  ", t, attr, s);
					res = PrintString(temp);
					if (res != SZ_OK)
						break;
					if (isDir)
						printf("/");
					printf("\n");
					continue;
				}

				fputs(testCommand ?
					"Testing    " :
					"Extracting ",
					stdout);
				res = PrintString(temp);
				if (res != SZ_OK)
					break;

				if (isDir)
					printf("/");
				else
				{
					//将文件解压，具体解压到内存还是某块硬盘区域不清楚，解压同时也检测了文件完整性
					res = SzArEx_Extract(&db, &lookStream.s, i,
						&blockIndex, &outBuffer, &outBufferSize,
						&offset, &outSizeProcessed,
						&allocImp, &allocTempImp);
					if (res != SZ_OK)
						break;
				}

				//若不是测试完整性模式，则为解压模式，将文件输出
				if (!testCommand)
				{
					CSzFile outFile;
					size_t processedSize;
					size_t j;
					UInt16 *name = (UInt16 *)temp;
					const UInt16 *destPath = (const UInt16 *)name;	//猜测是解压路径

					for (j = 0; name[j] != 0; j++)
						//这里猜测是，7z将路径中的'\'保存为'/'用于识别，然后创建路径之后再将其替换为'\'
						//如果参数设定不是全路径，那就把路径截掉，意思是所有文件忽略文件结构直接输出
						if (name[j] == '/')
						{
							if (fullPaths)
							{
								name[j] = 0;
								MyCreateDir(name);
								name[j] = CHAR_PATH_SEPARATOR;
							}
							else
								destPath = name + j + 1;
						}

					//如果当前数据是路径，就创建一个路径
					//否则就把文件输出
					if (isDir)
					{
						MyCreateDir(destPath);
						printf("\n");
						continue;
					}
					else if (OutFile_OpenUtf16(&outFile, destPath))
					{
						PrintError("can not open output file");
						res = SZ_ERROR_FAIL;
						break;
					}

					processedSize = outSizeProcessed;

					if (File_Write(&outFile, outBuffer + offset, &processedSize) != 0 || processedSize != outSizeProcessed)
					{
						PrintError("can not write output file");
						res = SZ_ERROR_FAIL;
						break;
					}

					if (File_Close(&outFile))
					{
						PrintError("can not close output file");
						res = SZ_ERROR_FAIL;
						break;
					}

#ifdef USE_WINDOWS_FILE
					//设置文件属性
					if (SzBitWithVals_Check(&db.Attribs, i))
						SetFileAttributesW(destPath, db.Attribs.Vals[i]);
#endif
				}
				printf("\n");
			}
			IAlloc_Free(&allocImp, outBuffer);
		}//指令判断结束
	}//压缩文件打开判断结束

	SzArEx_Free(&db, &allocImp);
	SzFree(NULL, temp);

	File_Close(&archiveStream.file);
	//输出一切ok
	if (res == SZ_OK)
	{
		printf("\nEverything is Ok\n");
		return 0;
	}

	if (res == SZ_ERROR_UNSUPPORTED)
		PrintError("decoder doesn't support this archive");
	else if (res == SZ_ERROR_MEM)
		PrintError("can not allocate memory");
	else if (res == SZ_ERROR_CRC)
		PrintError("CRC error");
	else
		printf("\nERROR #%d\n", res);

	return 1;
}
