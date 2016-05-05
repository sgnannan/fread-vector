#ifndef MYLZMA_H
#define MYLZMA_H

#include "./lzma/LzmaEnc.h"
#include "./lzma/LzmaDec.h"
#include "./lzma/Alloc.h"

void MyCompress(
  std::vector<unsigned char> &outBuf,
  const std::vector<unsigned char> &inBuf);

#endif