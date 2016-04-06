/* 7z.h -- 7z interface
2015-11-18 : Igor Pavlov : Public domain */

#ifndef __7Z_H
#define __7Z_H

#include "7zTypes.h"

EXTERN_C_BEGIN

#define k7zStartHeaderSize 0x20
#define k7zSignatureSize 6

extern const Byte k7zSignature[k7zSignatureSize];

//CSzData 结构体，暂时不知道是啥，表示一个byte格式的数组
typedef struct
{
  const Byte *Data;
  size_t Size;
} CSzData;

/* CSzCoderInfo & CSzFolder support only default methods */
//CSzCoderInfo 编码信息之类的结构体，可能是文件的属性信息结构体,内含：
//size_t PropsOffset;
//UInt32 MethodID;
//Byte NumStreams,PropsSize;
typedef struct
{
  size_t PropsOffset;
  UInt32 MethodID;
  Byte NumStreams;
  Byte PropsSize;
} CSzCoderInfo;

//CSzBond 结构体，记录输入输出编号之类？内含：
//UInt32 InIndex,OutIndex;
typedef struct
{
  UInt32 InIndex;
  UInt32 OutIndex;
} CSzBond;


#define SZ_NUM_CODERS_IN_FOLDER_MAX 4
#define SZ_NUM_BONDS_IN_FOLDER_MAX 3
#define SZ_NUM_PACK_STREAMS_IN_FOLDER_MAX 4
//CSzFolder Folder可以理解为7z压缩的基本单位，他是一个或几个文件的集合。内含：
//UInt32 NumCoder,NumBonds,NumPackStreams,UnpackStream,
//		 PackStream[SZ_NUM_PACK_STREAMS_IN_FOLDER_MAX];
//CSzBond Bonds[SZ_NUM_BONDS_IN_FOLDER_MAX];
//CSzCoderInfo Coders[SZ_NUM_CODERS_IN_FOLDER_MAX];
typedef struct
{
  UInt32 NumCoders;
  UInt32 NumBonds;
  UInt32 NumPackStreams;
  UInt32 UnpackStream;
  UInt32 PackStreams[SZ_NUM_PACK_STREAMS_IN_FOLDER_MAX];
  CSzBond Bonds[SZ_NUM_BONDS_IN_FOLDER_MAX];
  CSzCoderInfo Coders[SZ_NUM_CODERS_IN_FOLDER_MAX];
} CSzFolder;

//函数，不清楚具体功能，字面意思是获取下一个Folder对象
SRes SzGetNextFolderItem(CSzFolder *f, CSzData *sd);

//结构体，不清楚具体功能，应该是NTFS的文件创建时间，也可能代表一个64位整型数，内含：
//UInt32 Low,High;
typedef struct
{
  UInt32 Low;
  UInt32 High;
} CNtfsFileTime;
//结构体，可能是用作储存比特数据，内含：
//Byte *Defs;
//UInt32 *Vals;
typedef struct
{
  Byte *Defs; /* MSB 0 bit numbering */
  UInt32 *Vals;
} CSzBitUi32s;

//结构体，可能是用作储存比特数据
//内含：
//Byte *Defs;
//CNtfsFileTime *Vals;
typedef struct
{
  Byte *Defs; /* MSB 0 bit numbering */
  // UInt64 *Vals;
  CNtfsFileTime *Vals;
} CSzBitUi64s;

//宏定义，字面意思是数组检查
//p应该是数组，i
#define SzBitArray_Check(p, i) (((p)[(i) >> 3] & (0x80 >> ((i) & 7))) != 0)
//宏定义，字面意思是值检查
#define SzBitWithVals_Check(p, i) ((p)->Defs && ((p)->Defs[(i) >> 3] & (0x80 >> ((i) & 7))) != 0)

//CSzAr 结构体，可能是储存包的各种属性以及位置之类
//UInt32 NumPackStreams;			//包数目
//UInt32 NumFolders;				//Folder数目
//
//UInt64 *PackPositions;			// NumPackStreams + 1	包位置
//CSzBitUi32s FolderCRCs;			// NumFolders			Folder的CRC码
//
//size_t *FoCodersOffsets;			// NumFolders + 1		
//UInt32 *FoStartPackStreamIndex;	// NumFolders + 1
//UInt32 *FoToCoderUnpackSizes;		// NumFolders + 1
//Byte *FoToMainUnpackSizeIndex;	// NumFolders
//UInt64 *CoderUnpackSizes;			// for all coders in all folders
//
//Byte *CodersData;					// Coders数据
typedef struct
{
  UInt32 NumPackStreams;			// 包数目
  UInt32 NumFolders;				// 7z文件中Folder的数目

  UInt64 *PackPositions;			// NumPackStreams + 1	包位置
  CSzBitUi32s FolderCRCs;			// NumFolders			Folder的CRC码

  size_t *FoCodersOffsets;			// NumFolders + 1		
  UInt32 *FoStartPackStreamIndex;	// NumFolders + 1
  UInt32 *FoToCoderUnpackSizes;		// NumFolders + 1
  Byte *FoToMainUnpackSizeIndex;	// NumFolders
  UInt64 *CoderUnpackSizes;			// for all coders in all folders

  Byte *CodersData;					// Coders数据
} CSzAr;
//字面意思是获取Folder解压后的大小
UInt64 SzAr_GetFolderUnpackSize(const CSzAr *p, UInt32 folderIndex);

//字面意思是解码Folder
SRes SzAr_DecodeFolder(const CSzAr *p,
	UInt32 folderIndex,
    ILookInStream *stream,
	UInt64 startPos,
    Byte *outBuffer,
	size_t outSize,
    ISzAlloc *allocMain);

//CSzArEx 结构体，7z文档数据库
//CSzAr db;						//可能是文件头
//
//UInt64 startPosAfterHeader;	//文件头后的压缩数据开始位置
//UInt64 dataPos;				//data位置
//
//UInt32 NumFiles;				//文件数目
//
//UInt64 *UnpackPositions;		//NumFiles + 1 可能是每个文件的位置
//
//Byte *IsDirs;					//空目录标志
//CSzBitUi32s CRCs;				//CRCs校验码
//
//CSzBitUi32s Attribs;			//属性
//CSzBitUi64s MTime;
//CSzBitUi64s CTime;
//
//UInt32 *FolderToFile;			// NumFolders + 1	可能是储存Folder对应File的序号
//UInt32 *FileToFolder;			// NumFiles			可能是储存File对应Folder的序号
//
//size_t *FileNameOffsets;		//文件名补偿，/* in 2-byte steps */
//Byte *FileNames;				//UTF-16-LE 文件名	
typedef struct
{
  CSzAr db;						//可能是文件头

  UInt64 startPosAfterHeader;	//文件头后的压缩数据开始位置
  UInt64 dataPos;				//data位置
  
  UInt32 NumFiles;				//文件数目

  UInt64 *UnpackPositions;		// NumFiles + 1 可能是每个文件的位置
  // Byte *IsEmptyFiles;

  Byte *IsDirs;					//空目录标志
  CSzBitUi32s CRCs;				//CRCs校验码

  CSzBitUi32s Attribs;			//属性
  // CSzBitUi32s Parents;
  CSzBitUi64s MTime;
  CSzBitUi64s CTime;

  UInt32 *FolderToFile;			// NumFolders + 1	可能是储存Folder对应File的序号
  UInt32 *FileToFolder;			// NumFiles			可能是储存File对应Folder的序号

  size_t *FileNameOffsets;		//文件名补偿	/* in 2-byte steps */
  Byte *FileNames;				//文件名		/* UTF-16-LE */
} CSzArEx;

//不清楚用途
#define SzArEx_IsDir(p, i) (SzBitArray_Check((p)->IsDirs, i))			
//不清楚具体用途，字面意思是获取文件长度
#define SzArEx_GetFileSize(p, i) ((p)->UnpackPositions[(i) + 1] - (p)->UnpackPositions[i])

//初始化
void SzArEx_Init(CSzArEx *p);
//释放储存区域
void SzArEx_Free(CSzArEx *p, ISzAlloc *alloc);
//字面意思是获取Folder流位置
UInt64 SzArEx_GetFolderStreamPos(const CSzArEx *p, UInt32 folderIndex, UInt32 indexInFolder);
//字面意思是获取Folder整个压缩后的大小
int SzArEx_GetFolderFullPackSize(const CSzArEx *p, UInt32 folderIndex, UInt64 *resSize);

/*
if dest == NULL, the return value specifies the required size of the buffer,
  in 16-bit characters, including the null-terminating character.
if dest != NULL, the return value specifies the number of 16-bit characters that
  are written to the dest, including the null-terminating character. */

//字面意思返回完整的UTF16格式文件名
//如果dest == NULL，返回值以16bit字母为单位指定缓存空间请求的大小，包括null-terminating的字母
//如果dest != NULL，则返回写入dest的16bit字母的数量，包括终止符。
//CSzArEx *p		应该是保存压缩文件的数据结构指针
//size_t fileIndex	猜测是文件编号，应该是压缩文件中对应储存名字的数组
//UInt16 *dest		猜测是文件名的数组，用十六位整形储存UTF16字符串
size_t SzArEx_GetFileNameUtf16(const CSzArEx *p, size_t fileIndex, UInt16 *dest);

/*
size_t SzArEx_GetFullNameLen(const CSzArEx *p, size_t fileIndex);
UInt16 *SzArEx_GetFullNameUtf16_Back(const CSzArEx *p, size_t fileIndex, UInt16 *dest);
*/



/*
  SzArEx_Extract extracts file from archive

  *outBuffer must be 0 before first call for each new archive.

  Extracting cache:
    If you need to decompress more than one file, you can send
    these values from previous call:
      *blockIndex,
      *outBuffer,
      *outBufferSize
    You can consider "*outBuffer" as cache of solid block. If your archive is solid,
    it will increase decompression speed.
  
    If you use external function, you can declare these 3 cache variables
    (blockIndex, outBuffer, outBufferSize) as static in that external function.
    
    Free *outBuffer and set *outBuffer to 0, if you want to flush cache.
*/

/*	提取函数，从压缩文档中提取文件
	*outBuffer 在一个新文档第一次调用之前必须是0
	提取缓冲：
		如果你需要解压多个文件，你在调用前可以发送这些值：
		*blockIndex,
		*outBuffer,
		*outBufferSize
		你可以把*outBuffer当做固定的缓存区块。如果你的文档是solid，它会增加解压速度。

		如果要使用外部函数，你可以声明这三个变量作为static在外部的函数里。

		如果你想清空缓存，释放*outBuffer或者设置他为0
*/
SRes SzArEx_Extract(
    const CSzArEx *db,
    ILookInStream *inStream,
    UInt32 fileIndex,         /* index of file */
    UInt32 *blockIndex,       /* index of solid block */
    Byte **outBuffer,         /* pointer to pointer to output buffer (allocated with allocMain) */
    size_t *outBufferSize,    /* buffer size for output buffer */
    size_t *offset,           /* offset of stream for required file in *outBuffer */
    size_t *outSizeProcessed, /* size of file in *outBuffer */
    ISzAlloc *allocMain,
    ISzAlloc *allocTemp);


/*
打开函数，暂不清楚用途
SzArEx_Open Errors:
SZ_ERROR_NO_ARCHIVE
SZ_ERROR_ARCHIVE
SZ_ERROR_UNSUPPORTED
SZ_ERROR_MEM
SZ_ERROR_CRC
SZ_ERROR_INPUT_EOF
SZ_ERROR_FAIL
*/
SRes SzArEx_Open(CSzArEx *p, ILookInStream *inStream,
    ISzAlloc *allocMain, ISzAlloc *allocTemp);

EXTERN_C_END

#endif
