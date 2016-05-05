#include<stdio.h>
#include <vector>
#include <assert.h>

#include "MyCompress.h"

#define LZMA_PROPS_SIZE 5

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
	fwrite(&outBuf[4],1,destLen,file);

	fclose(file);
	
	

}

