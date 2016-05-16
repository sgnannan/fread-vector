#include<stdio.h>
#include <vector>
#include<windows.h>

#include "MyLZMA2.h"

using std::vector;

int main()
{
	//Ivan, read to buffer, copy from ram later
	FILE *file = fopen("data.dc", "rb+");

	long curpos,length;
	curpos=ftell(file);
	fseek(file,0L,SEEK_END);
	length=ftell(file);
	fseek(file,curpos,SEEK_SET);

	unsigned char* inbuff=(unsigned char*)malloc(length);
	size_t readlength = fread(inbuff, 1, length, file);

	if(readlength != length)
	{
		printf("read err\n");
		fclose(file);
		return false;
	}

	fclose(file);

	vector<unsigned char> vinbuff(inbuff, inbuff + length);
	vector<unsigned char> voutbuff;

	int TimeStart = GetTickCount();

	Byte m_properties;
	Byte * ptrPeroperties = &m_properties;

	CompressWithLZMA2(voutbuff, vinbuff, ptrPeroperties);

	int TotalTime = (GetTickCount() - TimeStart) / 1000;
	printf("time is %d!\n", TotalTime);

	//for(vector<unsigned char>::iterator it = tempbuff.begin();it!=tempbuff.end();it++)
	//{
	//   printf("%c", *it);
	//}

	printf("start to uncompress\n");

	FILE *file2 = fopen("data.dc.dat", "rb+");

	curpos = ftell(file);
	fseek(file, 0L, SEEK_END);
	length = ftell(file);
	fseek(file, curpos, SEEK_SET);

	unsigned char* inbuff1 = (unsigned char*)malloc(length);
	size_t readlength1 = fread(inbuff1, 1, length, file);

	if (readlength1 != length)
	{
		printf("read err\n");
		fclose(file);
		return false;
	}
	vector<unsigned char> vinbuff1(inbuff1, inbuff1 + length);
	vector<unsigned char> voutbuff1;

	UnCompressWithLZMA2(voutbuff1, vinbuff1);

	printf("ok\n");
		
	int temp;
	scanf("%d", &temp);
}
