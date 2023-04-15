#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include <emmintrin.h>  //sse2
#include <pmmintrin.h>  //sse3
#include <immintrin.h>
#include"InvertedIndex.h"
struct alignas(16) BitMapSSE
{
	alignas(16) vector<int> bits;			//id对应位
	alignas(16) vector<int> firstIndex;		//一级索引
	alignas(16) vector<int> secondIndex;	//二级索引
	BitMapSSE(int size= 30000000)
	{
		//开辟空间
		this->bits.resize(((size / 32 + 1) / 4 + 1)*4);
		this->firstIndex.resize(((size / 1024 + 1) / 4 + 1)*4);
		this->secondIndex.resize(((size / 32768 + 1) / 4 + 1)*4);
	}

	void set(int x) //将x对应的索引置位
	{
		//具体位置索引
		int index0 = x / 32; //数组索引即区间
		int index1 = index0 / 1024;
		int index2 = index1 / 1024;
		int offset0 = x % 32;
		int offset1 = offset0 / 32;
		int offset2 = offset1 / 32;

		this->bits[index0] |= (1 << offset0); //左移,置位为1
		this->firstIndex[index1] |= (1 << offset1);
		this->secondIndex[index2] |= (1 << offset2);
	}
};
alignas(16) BitMapSSE chosenListSSE;//选中的列表
alignas(16) BitMapSSE bitmapListSSE[2000] ;//开空间
alignas(16) BitMapSSE bitmapQListsSSE[5];//开空间

void bitMapSSEProcessing(vector<InvertedIndex>& invertedLists, int count)
{
	for (int i = 0; i < count; i++)
		for (int j = 0; j < invertedLists[i].length; j++)
			bitmapListSSE[i].set(invertedLists[i].docIdList[j]);	//建立id对应bitmap
}
InvertedIndex BITMAP_SSE(int* list, vector<InvertedIndex>& idx, int num)
{
	
	for (int i = 0; i < num; i++)
		bitmapQListsSSE[i] = bitmapListSSE[list[i]];//建立id对应bitmap
	chosenListSSE = bitmapQListsSSE[0];
	for (int i = 1; i < num; i++)
	{
		for (int j = 0; j < bitmapQListsSSE[0].secondIndex.size(); j+=4)
		{
			alignas(16) int resSec[4]; 
			__m128i cSecIndex = _mm_loadu_epi32(&chosenListSSE.secondIndex[j]);//二级索引比较
			__m128i cSecOpIndex = _mm_loadu_epi32(&bitmapQListsSSE[i].secondIndex[j]);
			__m128i comSecRes = _mm_and_si128(cSecIndex, cSecOpIndex);//按位与
			_mm_store_si128((__m128i*) resSec, comSecRes);
			for(int k=0;k<4;k++)
				if(resSec[k]!=0)
					for (int t = (j+k) * 32; t < (j+k) * 32 ; t+=4)
					{
						alignas(16) int resFir[4];
						__m128i cFirIndex = _mm_loadu_epi32(&chosenListSSE.firstIndex[t]);//二级索引比较;//一级索引比较
						__m128i cFirOpIndex = _mm_loadu_epi32(&bitmapQListsSSE[i].firstIndex[t]);
						__m128i comFirRes = _mm_and_si128(cFirIndex, cFirOpIndex);//按位与

						_mm_store_si128((__m128i*) resFir, comFirRes);//得到4个4整数表示比较结果
						for(int c=0;c<4;c++)
							if (resFir[c]!=0)
							{
								for (int l = (t+c) * 32; l < (t+c) * 32 ; l+=4)
								{
									__m128i cIndex = _mm_loadu_epi32(&chosenListSSE.firstIndex[l]);//关键字比较
									__m128i cOpIndex = _mm_loadu_epi32(&bitmapQListsSSE[i].firstIndex[l]);
									__m128i comRes = _mm_and_si128(cIndex, cOpIndex);//按位与
									_mm_store_si128((__m128i*)&chosenListSSE.bits[l], comRes);
								}
							}
					}
		}
	}
	InvertedIndex x;
	return x;
}

//下面是用SIMD只展开一层,实测效率较低
//InvertedIndex BITMAPSSE2(int* list, vector<InvertedIndex>& idx, int num)
//{
//	for (int i = 0; i < num; i++)
//		bitmapQListsSSE[i] = bitmapListSSE[list[i]];//建立id对应bitmap
//	chosenListSSE = bitmapQListsSSE[0];
//	for (int i = 1; i < num; i++)
//	{
//		for (int j = 0; j < chosenListSSE.secondIndex.size(); j++)
//		{
//			chosenListSSE.secondIndex[j] &= bitmapQListsSSE[i].secondIndex[j];//按位与
//			if (chosenListSSE.secondIndex[j])
//			{
//				for (int t = j * 32; t < j * 32 + 32; t++)
//				{
//					chosenListSSE.firstIndex[t] &= bitmapQListsSSE[i].firstIndex[t];//按位与
//					if (chosenListSSE.firstIndex[t])
//					{
//						for (int l = t * 32; l < t * 32 + 32; l+=4)
//						{
//							__m128i cIndex = _mm_load_si128((__m128i*) & chosenListSSE.bits[l]);//按位与
//							__m128i cOpIndex = _mm_load_si128((__m128i*) & bitmapQListsSSE[i].bits[l]);
//							__m128i comRes = _mm_and_si128(cIndex, cOpIndex);
//							_mm_store_si128((__m128i*) & chosenListSSE.bits[l], comRes);
//						}
//					}
//				}
//			}
//		}
//	}
//	//下面是将bitmap转为docId的代码
//	//for (int i = 0; i < chosenList.bits.size(); i++)
//	//{
//	//	int cnt = i * 32;
//	//	int bit = chosenList.bits[i];
//	//	while (bit != 0)
//	//	{
//	//		if ((bit & 1) != 0)
//	//			cout << cnt << ' ';
//	//		bit = bit >> 1;
//	//		++cnt;
//	//	}
//	//}
//	InvertedIndex x;
//	return x;
//}