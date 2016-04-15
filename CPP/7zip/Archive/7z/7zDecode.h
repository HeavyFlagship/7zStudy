// 7zDecode.h

#ifndef __7Z_DECODE_H
#define __7Z_DECODE_H

#include "../Common/CoderMixer2.h"

#include "7zIn.h"

namespace NArchive {
namespace N7z {

struct CBindInfoEx: public NCoderMixer2::CBindInfo
{
  CRecordVector<CMethodId> CoderMethodIDs;

  void Clear()
  {
    CBindInfo::Clear();
    CoderMethodIDs.Clear();
  }
};

class CDecoder
{
  bool _bindInfoPrev_Defined;
  CBindInfoEx _bindInfoPrev;
  
  bool _useMixerMT;
  
  #ifdef USE_MIXER_ST
    NCoderMixer2::CMixerST *_mixerST;
  #endif
  
  #ifdef USE_MIXER_MT
    NCoderMixer2::CMixerMT *_mixerMT;
  #endif
  
  NCoderMixer2::CMixer *_mixer;
  CMyComPtr<IUnknown> _mixerRef;

public:

  CDecoder(bool useMixerMT);
//HRESULT Decode(
//	  DECL_EXTERNAL_CODECS_LOC_VARS
//	  IInStream *inStream,		//读取流
//	  UInt64 startPos,			//开始位置
//	  const CFolders &folders,	//folder
//    unsigned folderIndex,		//folder的编号
//	  const UInt64 *unpackSize	// 如果unpackSize不存在，则解压整个
//								// 否则，只解压指定bytes的folder
//	  , ISequentialOutStream *outStream			//输出流
//	  , ICompressProgressInfo *compressProgress	//压缩过程
//	  , ISequentialInStream **inStreamMainRes	//不清楚
//
//	  _7Z_DECODER_CRYPRO_VARS_DECL				//加密参数	
//
//#if !defined(_7ZIP_ST) && !defined(_SFX)
//	  , bool mtMode, UInt32 numThreads			//多线程相关
//#endif
//	  );
  HRESULT Decode(
      DECL_EXTERNAL_CODECS_LOC_VARS
      IInStream *inStream,
      UInt64 startPos,
      const CFolders &folders, unsigned folderIndex,
      const UInt64 *unpackSize // if (!unpackSize), then full folder is required
                               // if (unpackSize), then only *unpackSize bytes from folder are required

      , ISequentialOutStream *outStream
      , ICompressProgressInfo *compressProgress
      , ISequentialInStream **inStreamMainRes
      
      _7Z_DECODER_CRYPRO_VARS_DECL
      
      #if !defined(_7ZIP_ST) && !defined(_SFX)
      , bool mtMode, UInt32 numThreads
      #endif
      );
};

}}

#endif
