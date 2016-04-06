/* 7z.h -- 7z interface
2015-11-18 : Igor Pavlov : Public domain */

#ifndef __7Z_H
#define __7Z_H

#include "7zTypes.h"

EXTERN_C_BEGIN

#define k7zStartHeaderSize 0x20
#define k7zSignatureSize 6

extern const Byte k7zSignature[k7zSignatureSize];

//CSzData �ṹ�壬��ʱ��֪����ɶ����ʾһ��byte��ʽ������
typedef struct
{
  const Byte *Data;
  size_t Size;
} CSzData;

/* CSzCoderInfo & CSzFolder support only default methods */
//CSzCoderInfo ������Ϣ֮��Ľṹ�壬�������ļ���������Ϣ�ṹ��,�ں���
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

//CSzBond �ṹ�壬��¼����������֮�ࣿ�ں���
//UInt32 InIndex,OutIndex;
typedef struct
{
  UInt32 InIndex;
  UInt32 OutIndex;
} CSzBond;


#define SZ_NUM_CODERS_IN_FOLDER_MAX 4
#define SZ_NUM_BONDS_IN_FOLDER_MAX 3
#define SZ_NUM_PACK_STREAMS_IN_FOLDER_MAX 4
//CSzFolder Folder�������Ϊ7zѹ���Ļ�����λ������һ���򼸸��ļ��ļ��ϡ��ں���
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

//��������������幦�ܣ�������˼�ǻ�ȡ��һ��Folder����
SRes SzGetNextFolderItem(CSzFolder *f, CSzData *sd);

//�ṹ�壬��������幦�ܣ�Ӧ����NTFS���ļ�����ʱ�䣬Ҳ���ܴ���һ��64λ���������ں���
//UInt32 Low,High;
typedef struct
{
  UInt32 Low;
  UInt32 High;
} CNtfsFileTime;
//�ṹ�壬��������������������ݣ��ں���
//Byte *Defs;
//UInt32 *Vals;
typedef struct
{
  Byte *Defs; /* MSB 0 bit numbering */
  UInt32 *Vals;
} CSzBitUi32s;

//�ṹ�壬���������������������
//�ں���
//Byte *Defs;
//CNtfsFileTime *Vals;
typedef struct
{
  Byte *Defs; /* MSB 0 bit numbering */
  // UInt64 *Vals;
  CNtfsFileTime *Vals;
} CSzBitUi64s;

//�궨�壬������˼��������
//pӦ�������飬i
#define SzBitArray_Check(p, i) (((p)[(i) >> 3] & (0x80 >> ((i) & 7))) != 0)
//�궨�壬������˼��ֵ���
#define SzBitWithVals_Check(p, i) ((p)->Defs && ((p)->Defs[(i) >> 3] & (0x80 >> ((i) & 7))) != 0)

//CSzAr �ṹ�壬�����Ǵ�����ĸ��������Լ�λ��֮��
//UInt32 NumPackStreams;			//����Ŀ
//UInt32 NumFolders;				//Folder��Ŀ
//
//UInt64 *PackPositions;			// NumPackStreams + 1	��λ��
//CSzBitUi32s FolderCRCs;			// NumFolders			Folder��CRC��
//
//size_t *FoCodersOffsets;			// NumFolders + 1		
//UInt32 *FoStartPackStreamIndex;	// NumFolders + 1
//UInt32 *FoToCoderUnpackSizes;		// NumFolders + 1
//Byte *FoToMainUnpackSizeIndex;	// NumFolders
//UInt64 *CoderUnpackSizes;			// for all coders in all folders
//
//Byte *CodersData;					// Coders����
typedef struct
{
  UInt32 NumPackStreams;			// ����Ŀ
  UInt32 NumFolders;				// 7z�ļ���Folder����Ŀ

  UInt64 *PackPositions;			// NumPackStreams + 1	��λ��
  CSzBitUi32s FolderCRCs;			// NumFolders			Folder��CRC��

  size_t *FoCodersOffsets;			// NumFolders + 1		
  UInt32 *FoStartPackStreamIndex;	// NumFolders + 1
  UInt32 *FoToCoderUnpackSizes;		// NumFolders + 1
  Byte *FoToMainUnpackSizeIndex;	// NumFolders
  UInt64 *CoderUnpackSizes;			// for all coders in all folders

  Byte *CodersData;					// Coders����
} CSzAr;
//������˼�ǻ�ȡFolder��ѹ��Ĵ�С
UInt64 SzAr_GetFolderUnpackSize(const CSzAr *p, UInt32 folderIndex);

//������˼�ǽ���Folder
SRes SzAr_DecodeFolder(const CSzAr *p,
	UInt32 folderIndex,
    ILookInStream *stream,
	UInt64 startPos,
    Byte *outBuffer,
	size_t outSize,
    ISzAlloc *allocMain);

//CSzArEx �ṹ�壬7z�ĵ����ݿ�
//CSzAr db;						//�������ļ�ͷ
//
//UInt64 startPosAfterHeader;	//�ļ�ͷ���ѹ�����ݿ�ʼλ��
//UInt64 dataPos;				//dataλ��
//
//UInt32 NumFiles;				//�ļ���Ŀ
//
//UInt64 *UnpackPositions;		//NumFiles + 1 ������ÿ���ļ���λ��
//
//Byte *IsDirs;					//��Ŀ¼��־
//CSzBitUi32s CRCs;				//CRCsУ����
//
//CSzBitUi32s Attribs;			//����
//CSzBitUi64s MTime;
//CSzBitUi64s CTime;
//
//UInt32 *FolderToFile;			// NumFolders + 1	�����Ǵ���Folder��ӦFile�����
//UInt32 *FileToFolder;			// NumFiles			�����Ǵ���File��ӦFolder�����
//
//size_t *FileNameOffsets;		//�ļ���������/* in 2-byte steps */
//Byte *FileNames;				//UTF-16-LE �ļ���	
typedef struct
{
  CSzAr db;						//�������ļ�ͷ

  UInt64 startPosAfterHeader;	//�ļ�ͷ���ѹ�����ݿ�ʼλ��
  UInt64 dataPos;				//dataλ��
  
  UInt32 NumFiles;				//�ļ���Ŀ

  UInt64 *UnpackPositions;		// NumFiles + 1 ������ÿ���ļ���λ��
  // Byte *IsEmptyFiles;

  Byte *IsDirs;					//��Ŀ¼��־
  CSzBitUi32s CRCs;				//CRCsУ����

  CSzBitUi32s Attribs;			//����
  // CSzBitUi32s Parents;
  CSzBitUi64s MTime;
  CSzBitUi64s CTime;

  UInt32 *FolderToFile;			// NumFolders + 1	�����Ǵ���Folder��ӦFile�����
  UInt32 *FileToFolder;			// NumFiles			�����Ǵ���File��ӦFolder�����

  size_t *FileNameOffsets;		//�ļ�������	/* in 2-byte steps */
  Byte *FileNames;				//�ļ���		/* UTF-16-LE */
} CSzArEx;

//�������;
#define SzArEx_IsDir(p, i) (SzBitArray_Check((p)->IsDirs, i))			
//�����������;��������˼�ǻ�ȡ�ļ�����
#define SzArEx_GetFileSize(p, i) ((p)->UnpackPositions[(i) + 1] - (p)->UnpackPositions[i])

//��ʼ��
void SzArEx_Init(CSzArEx *p);
//�ͷŴ�������
void SzArEx_Free(CSzArEx *p, ISzAlloc *alloc);
//������˼�ǻ�ȡFolder��λ��
UInt64 SzArEx_GetFolderStreamPos(const CSzArEx *p, UInt32 folderIndex, UInt32 indexInFolder);
//������˼�ǻ�ȡFolder����ѹ����Ĵ�С
int SzArEx_GetFolderFullPackSize(const CSzArEx *p, UInt32 folderIndex, UInt64 *resSize);

/*
if dest == NULL, the return value specifies the required size of the buffer,
  in 16-bit characters, including the null-terminating character.
if dest != NULL, the return value specifies the number of 16-bit characters that
  are written to the dest, including the null-terminating character. */

//������˼����������UTF16��ʽ�ļ���
//���dest == NULL������ֵ��16bit��ĸΪ��λָ������ռ�����Ĵ�С������null-terminating����ĸ
//���dest != NULL���򷵻�д��dest��16bit��ĸ��������������ֹ����
//CSzArEx *p		Ӧ���Ǳ���ѹ���ļ������ݽṹָ��
//size_t fileIndex	�²����ļ���ţ�Ӧ����ѹ���ļ��ж�Ӧ�������ֵ�����
//UInt16 *dest		�²����ļ��������飬��ʮ��λ���δ���UTF16�ַ���
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

/*	��ȡ��������ѹ���ĵ�����ȡ�ļ�
	*outBuffer ��һ�����ĵ���һ�ε���֮ǰ������0
	��ȡ���壺
		�������Ҫ��ѹ����ļ������ڵ���ǰ���Է�����Щֵ��
		*blockIndex,
		*outBuffer,
		*outBufferSize
		����԰�*outBuffer�����̶��Ļ������顣�������ĵ���solid���������ӽ�ѹ�ٶȡ�

		���Ҫʹ���ⲿ���������������������������Ϊstatic���ⲿ�ĺ����

		���������ջ��棬�ͷ�*outBuffer����������Ϊ0
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
�򿪺������ݲ������;
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
