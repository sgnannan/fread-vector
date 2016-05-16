#ifndef MYLZMA2_H
#define MYLZMA2_H

#include "./lzma/7zTypes.h"

void CompressInc(std::vector<unsigned char> &outBuf, const std::vector<unsigned char> &inBuf);

void UncompressInc(std::vector<unsigned char> &outBuf, const std::vector<unsigned char> &inBuf);

void CompressWithLZMA2(std::vector<unsigned char> &outBuf, const std::vector<unsigned char> &inBuf, Byte* ptrProperties);

void UnCompressWithLZMA2(std::vector<unsigned char> &outBuf, const std::vector<unsigned char> &inBuf);

#endif
