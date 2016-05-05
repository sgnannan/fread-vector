#include <stdio.h>
#include <vector>
#include <assert.h>


#include "MyCompress.h"

#define LZMA_PROPS_SIZE 5
#define min(a,b) (((a) < (b)) ? (a) : (b))

SRes OnProgress(void *p, UInt64 inSize, UInt64 outSize)
{
  // Update progress bar.
  return SZ_OK;
}

static ICompressProgress g_ProgressCallback = { &OnProgress };

static void * AllocForLzma(void *p, size_t size) { return malloc(size); }
static void FreeForLzma(void *p, void *address) { free(address); }
static ISzAlloc SzAllocForLzma = { &AllocForLzma, &FreeForLzma };


void MyCompress(
	std::vector<unsigned char> &outBuf,
	const std::vector<unsigned char> &inBuf)
{
	size_t propsSize = LZMA_PROPS_SIZE;
	size_t resLen = inBuf.size();
	size_t destLen = inBuf.size() + inBuf.size() / 3 + 128;
	outBuf.resize(propsSize + destLen);

	CLzmaEncProps props;
	LzmaEncProps_Init(&props);
	props.dictSize = 1 << 16; // 64 KB
	props.writeEndMark = 1; // 0 or 1

	int res = LzmaEncode(
		&outBuf[LZMA_PROPS_SIZE], &destLen,
		&inBuf[0], inBuf.size(),
		&props, &outBuf[0], &propsSize, props.writeEndMark,
		&g_ProgressCallback, &SzAllocForLzma, &SzAllocForLzma);
	assert(res == SZ_OK && propsSize == LZMA_PROPS_SIZE);

	outBuf.resize(propsSize + destLen);

	FILE *file = fopen("data1.dat", "w");

	int zero = 0;
	fwrite(&outBuf[0],1,propsSize,file);
	fwrite(&resLen,1,4,file);
	fwrite(&zero,1,4,file);
	fwrite(&outBuf[LZMA_PROPS_SIZE],1,destLen,file);

	fclose(file);
}

typedef struct
{
  ISeqInStream SeqInStream;
  const std::vector<unsigned char> *Buf;
  unsigned BufPos;
} VectorInStream;

SRes VectorInStream_Read(void *p, void *buf, size_t *size)
{
  VectorInStream *ctx = (VectorInStream*)p;
  *size = min(*size, ctx->Buf->size() - ctx->BufPos);
  if (*size)
    memcpy(buf, &(*ctx->Buf)[ctx->BufPos], *size);
  ctx->BufPos += *size;
  return SZ_OK;
}

typedef struct
{
	ISeqOutStream SeqOutStream;
	std::vector<unsigned char> *Buf;
} VectorOutStream;

size_t VectorOutStream_Write(void *p, const void *buf, size_t size)
{
	VectorOutStream *ctx = (VectorOutStream*)p;
	if (size)
	{
		unsigned oldSize = ctx->Buf->size();
		ctx->Buf->resize(oldSize + size);
		memcpy(&(*ctx->Buf)[oldSize], buf, size);
	}
	return size;
}

void CompressInc(
	std::vector<unsigned char> &outBuf,
	const std::vector<unsigned char> &inBuf)
{
	CLzmaEncHandle enc = LzmaEnc_Create(&SzAllocForLzma);
	assert(enc);

	CLzmaEncProps props;
	LzmaEncProps_Init(&props);
	props.writeEndMark = 1; // 0 or 1

	SRes res = LzmaEnc_SetProps(enc, &props);
	assert(res == SZ_OK);

	unsigned propsSize = LZMA_PROPS_SIZE;
	outBuf.resize(propsSize);

	res = LzmaEnc_WriteProperties(enc, &outBuf[0], &propsSize);
	assert(res == SZ_OK && propsSize == LZMA_PROPS_SIZE);

	VectorInStream inStream = { &VectorInStream_Read, &inBuf, 0 };
	VectorOutStream outStream = { &VectorOutStream_Write, &outBuf };

	res = LzmaEnc_Encode(enc,
		(ISeqOutStream*)&outStream, (ISeqInStream*)&inStream,
		0, &SzAllocForLzma, &SzAllocForLzma);
		assert(res == SZ_OK);

		printf("sgnannan: OK!");

	LzmaEnc_Destroy(enc, &SzAllocForLzma, &SzAllocForLzma);

	FILE *file = fopen("data2.dat", "w");

	size_t resLen = inBuf.size();
	int zero = 0;
	fwrite(&outBuf[0],1,propsSize,file);
	fwrite(&resLen,1,4,file);
	fwrite(&zero,1,4,file);
	fwrite(&outBuf,1,outBuf.size(),file);

	fclose(file);

}