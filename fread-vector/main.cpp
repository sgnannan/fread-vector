#include<stdio.h>
#include <vector>
#include<windows.h>

#include "MyCompress.h"

using std::vector;

int main()
{
	//read to buffer
	FILE *file = fopen("data.dg", "rb+");

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

	MyCompress(voutbuff, vinbuff);

	int TotalTime = (GetTickCount() - TimeStart) / 1000;
	printf("time is %d!\n", TotalTime);

	//for(vector<unsigned char>::iterator it = tempbuff.begin();it!=tempbuff.end();it++)
	//{
	//   printf("%c", *it);
	//}

	int temp;
	scanf("%d", &temp);
}
