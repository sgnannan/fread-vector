#include <stdio.h>
#include <vector>
#include <assert.h>

#include <inttypes.h>
#include "MyLZMA2.h"

#include "./lzma/Lzma2Enc.h"
#include "./lzma/Lzma2Dec.h"
#include "./lzma/Alloc.h"
#include "./lzma/7zTypes.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define IN_BUF_SIZE (1 << 16)
#define OUT_BUF_SIZE (1 << 16)

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

SRes OnProgress(void *p, UInt64 inSize, UInt64 outSize)
{
	// Update progress bar.
	return SZ_OK;
}

static ICompressProgress g_ProgressCallback = { &OnProgress };

//参考例子进行挑选
static void * AllocForLzma(void *p, size_t size) { return malloc(size); }
static void FreeForLzma(void *p, void *address) { free(address); }
static ISzAlloc SzAllocForLzma = { &AllocForLzma, &FreeForLzma };

void CompressInc(std::vector<unsigned char> &outBuf, const std::vector<unsigned char> &inBuf)
{
	CLzmaEncHandle enc = LzmaEnc_Create(&g_Alloc);
	assert(enc);

	CLzmaEncProps props;
	LzmaEncProps_Init(&props);
	//props.writeEndMark = 1; // 0 or 1

	SRes res = LzmaEnc_SetProps(enc, &props);
	assert(res == SZ_OK);

	unsigned char tempprops[5];

	unsigned propsSize = LZMA_PROPS_SIZE;
	//outBuf.resize(propsSize);		//fill with zero, which cost me a week to check the mistake.

	res = LzmaEnc_WriteProperties(enc, tempprops, &propsSize);
	assert(res == SZ_OK && propsSize == LZMA_PROPS_SIZE);

	VectorInStream inStream = { &VectorInStream_Read, &inBuf, 0 };
	VectorOutStream outStream = { &VectorOutStream_Write, &outBuf };

	res = LzmaEnc_Encode(enc,
		(ISeqOutStream*)&outStream, (ISeqInStream*)&inStream,
		0, &g_Alloc, &g_Alloc);
	assert(res == SZ_OK);

	UInt64 resLen = inBuf.size();
	//printf("uint64 resLen : %" PRIu64 "\n", resLen);	//print uint64

	Byte header[8];
	for (int i = 0; i < 8; i++)
		header[i] = (Byte)(resLen >> (8 * i));

	FILE *file = fopen("data3.dat", "wb+");

	fwrite(tempprops, 1, propsSize, file);
	fwrite(&header[0], 1, 8, file);
	fwrite(&outBuf[0], 1, outBuf.size(), file);

	fclose(file);

	LzmaEnc_Destroy(enc, &g_Alloc, &g_Alloc);
}

void CompressWithLZMA2(std::vector<unsigned char> &outBuf, const std::vector<unsigned char> &inBuf, Byte* ptrProperties)
{
	VectorInStream inStream = { &VectorInStream_Read, &inBuf, 0 };
	VectorOutStream outStream = { &VectorOutStream_Write, &outBuf };

	CLzma2EncHandle enc;
	enc = Lzma2Enc_Create(&g_Alloc, &g_BigAlloc);

	assert(enc);

	CLzma2EncProps props;
	Lzma2EncProps_Init(&props);			//Ivan need change the parameters later
	//props.lzmaProps.writeEndMark = 1; // 0 or 1
	//props.lzmaProps.level = 6;
	//props.lzmaProps.dictSize = 1 << 14;
	//props.lzmaProps.numThreads = 8;
	//props.numTotalThreads = 8;

	SRes res = Lzma2Enc_SetProps(enc, &props);
	assert(res == SZ_OK);

	outBuf.resize(1);		// no need

	outBuf[0] = Lzma2Enc_WriteProperties(enc);

	*ptrProperties = outBuf[0];		//no need this parameter.

	UInt64 resLen = inBuf.size();

	Byte header[8];
	for (int i = 0; i < 8; i++)
		header[i] = (Byte)(resLen >> (8 * i));

	res = Lzma2Enc_Encode(enc, &outStream.SeqOutStream, &inStream.SeqInStream, 0);		//res = Lzma2Enc_Encode(enc, (ISeqOutStream*)&outStream, (ISeqInStream*)&inStream, 0);

	assert(res == SZ_OK);

	Lzma2Enc_Destroy(enc);

	FILE *fout = fopen("data.dc.dat", "wb+");	//change the file name.
	
	fwrite(&outBuf[0], 1, 1, fout);
	fwrite(&header[0], 1, 8, fout);
	fwrite(&outBuf[1], 1, outBuf.size() - 1, fout);
	fclose(fout);
}

void UncompressInc(
	std::vector<unsigned char> &outBuf,
	const std::vector<unsigned char> &inBuf)
{
	CLzmaDec dec;

	UInt64 unpackSize = 0;
	for (int i = 0; i < 8; i++)
		unpackSize += (UInt64)inBuf[5 + i] << (i * 8);

	LzmaDec_Construct(&dec);
	SRes res = LzmaDec_Allocate(&dec, &inBuf[0], LZMA_PROPS_SIZE, &SzAllocForLzma);
	assert(res == SZ_OK);

	LzmaDec_Init(&dec);

	//printf("uint64 resLen : %" PRIu64 "\n", unpackSize);		//print uint64

	outBuf.resize(unpackSize);
	unsigned outPos = 0, inPos = LZMA_PROPS_SIZE;
	ELzmaStatus status;
	const unsigned BUF_SIZE = 10240;
	while (outPos < outBuf.size())
	{
		unsigned destLen = min(BUF_SIZE, outBuf.size() - outPos);
		unsigned srcLen = min(BUF_SIZE, inBuf.size() - 8 - inPos);

		res = LzmaDec_DecodeToBuf(&dec,
			&outBuf[outPos], &destLen,
			&inBuf[inPos+8], &srcLen,
			(outPos + destLen == outBuf.size())
			? LZMA_FINISH_END : LZMA_FINISH_ANY, &status);
		assert(res == SZ_OK);
		inPos += srcLen;
		outPos += destLen;
		if (status == LZMA_STATUS_FINISHED_WITH_MARK)
			break;
	}

	LzmaDec_Free(&dec, &g_Alloc);
	outBuf.resize(outPos);

	FILE *fout = fopen("data33.dc", "wb+");
	fwrite(&outBuf[0], 1, outBuf.size(), fout);
	fclose(fout);

}

void UnCompressWithLZMA2(std::vector<unsigned char> &outBuf, const std::vector<unsigned char> &inBuf)
{
	CLzma2Dec dec;
	Lzma2Dec_Construct(&dec);

	UInt64 unpackSize = 0;

	SRes res = Lzma2Dec_Allocate(&dec, inBuf[0], &g_Alloc);

	assert(res == SZ_OK);

	Lzma2Dec_Init(&dec);

	for (int i = 0; i < 8; i++)
		unpackSize += (UInt64)inBuf[1+i] << (i * 8);

	outBuf.resize(unpackSize);

	unsigned outPos = 0, inPos = 9;
	ELzmaStatus status;
	const unsigned BUF_SIZE = 10240;

	while (outPos < outBuf.size())
	{
		SizeT destLen = min(BUF_SIZE, outBuf.size() - outPos);
		SizeT srcLen = min(BUF_SIZE, inBuf.size() - inPos);

		res = Lzma2Dec_DecodeToBuf(&dec,
			&outBuf[outPos],
			&destLen,
			&inBuf[inPos],
			&srcLen,
			(outPos + destLen == outBuf.size()) ? LZMA_FINISH_END : LZMA_FINISH_ANY,
			&status);
		unsigned int outbufsize = outBuf.size();

		assert(res == SZ_OK);
		inPos += srcLen;
		outPos += destLen;

		if (status == LZMA_STATUS_FINISHED_WITH_MARK)
		{
			break;
		}
	}

	Lzma2Dec_Free(&dec, &g_Alloc);

	outBuf.resize(outPos);

	FILE *fout = fopen("data22.dc", "wb+");
	fwrite(&outBuf[0], 1, outBuf.size(), fout);
	fclose(fout);
}