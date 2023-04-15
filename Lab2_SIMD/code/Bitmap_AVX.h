#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include <emmintrin.h>  //sse2
#include <pmmintrin.h>  //sse3
#include <immintrin.h>
#include"InvertedIndex.h"
struct alignas(32) BitMapAVX
{
	alignas(32) vector<int> bits;			//id对应位
	alignas(32) vector<int> firstIndex;		//一级索引
	alignas(32) vector<int> secondIndex;	//二级索引
	BitMapAVX(int size = 30000000)
	{
		//开辟空间
		this->bits.resize(((size / 32 + 1) / 8 + 1) * 8);//八的倍数
		this->firstIndex.resize(((size / 1024 + 1) / 8 + 1) * 8);
		this->secondIndex.resize(((size / 32768 + 1) / 8 + 1) * 8);
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
alignas(32) BitMapAVX chosenListAVX;//选中的列表
alignas(32) BitMapAVX bitmapListAVX[2000];//开空间
alignas(32) BitMapAVX bitmapQListsAVX[5];//开空间

void bitMapAVXProcessing(vector<InvertedIndex>& invertedLists, int count)
{
	for (int i = 0; i < count; i++)
		for (int j = 0; j < invertedLists[i].length; j++)
			bitmapListAVX[i].set(invertedLists[i].docIdList[j]);	//建立id对应bitmap
}
InvertedIndex BITMAP_AVX(int* list, vector<InvertedIndex>& idx, int num)
{

	for (int i = 0; i < num; i++)
		bitmapQListsAVX[i] = bitmapListAVX[list[i]];//建立id对应bitmap
	chosenListAVX = bitmapQListsAVX[0];
	for (int i = 1; i < num; i++)
	{
		for (int j = 0; j < bitmapQListsAVX[0].secondIndex.size(); j += 8)
		{
			alignas(32) int resSec[8];
			__m256i cSecIndex = _mm256_loadu_epi32(&chosenListAVX.secondIndex[j]);//二级索引比较
			__m256i cSecOpIndex = _mm256_loadu_epi32(&bitmapQListsAVX[i].secondIndex[j]);
			__m256i comSecRes = _mm256_and_si256(cSecIndex, cSecOpIndex);//按位与
			_mm256_storeu_si256((__m256i*) resSec, comSecRes);
			for (int k = 0; k < 8; k++)
				if (resSec[k] != 0)
					for (int t = (j + k) * 32; t < (j + k) * 32; t += 8)
					{
						alignas(32) int resFir[8];
						__m256i cFirIndex = _mm256_loadu_epi32(&chosenListAVX.firstIndex[t]);//二级索引比较;//一级索引比较
						__m256i cFirOpIndex = _mm256_loadu_epi32(&bitmapQListsAVX[i].firstIndex[t]);
						__m256i comFirRes = _mm256_and_si256(cFirIndex, cFirOpIndex);//按位与

						_mm256_store_si256((__m256i*) resFir, comFirRes);//得到4个4整数表示比较结果
						for (int c = 0; c < 8; c++)
							if (resFir[c] != 0)
							{
								for (int l = (t + c) * 32; l < (t + c) * 32; l += 8)
								{
									__m256i cIndex = _mm256_loadu_epi32(&chosenListAVX.firstIndex[l]);//关键字比较
									__m256i cOpIndex = _mm256_loadu_epi32(&bitmapQListsAVX[i].firstIndex[l]);
									__m256i comRes = _mm256_and_si256(cIndex, cOpIndex);//按位与
									_mm256_store_si256((__m256i*) & chosenListAVX.bits[l], comRes);
								}
							}
					}
		}
	}
	InvertedIndex x;
	return x;
}
