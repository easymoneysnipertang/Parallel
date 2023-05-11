#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <algorithm>
#include <arm_neon.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include"Adp.h"
#include"Hash.h"
#include"InvertedIndex.h"
struct alignas(16) BitMapNeon
{
	alignas(16) vector<int> bits;			//id对应位
	alignas(16) vector<int> firstIndex;		//一级索引
	alignas(16) vector<int> secondIndex;	//二级索引
	BitMapNeon(int size = 25205200)
	{
		//开辟空间
		this->bits.resize(((size / 32 + 1) / 4 + 1) * 4);
		this->firstIndex.resize(((size / 1024 + 1) / 4 + 1) * 4);
		this->secondIndex.resize(((size / 32768 + 1) / 4 + 1) * 4);
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
alignas(16) BitMapNeon chosenListNeon;//选中的列表
alignas(16) BitMapNeon bitmapListNeon[2000];//开空间
alignas(16) BitMapNeon bitmapQListsNeon[5];//开空间

void bitMapNeonProcessing(vector<InvertedIndex>& invertedLists, int count)
{
	for (int i = 0; i < count; i++)
		for (int j = 0; j < invertedLists[i].length; j++)
			bitmapListNeon[i].set(invertedLists[i].docIdList[j]);	//建立id对应bitmap
}
InvertedIndex BITMAP_NEON(int* list, vector<InvertedIndex>& idx, int num)
{

	for (int i = 0; i < num; i++)
		bitmapQListsNeon[i] = bitmapListNeon[list[i]];//建立id对应bitmap
	chosenListNeon = bitmapQListsNeon[0];
	for (int i = 1; i < num; i++)
	{
		for (int j = 0; j < bitmapQListsNeon[0].secondIndex.size(); j += 4)
		{
			alignas(16) int resSec[4];
			int32x4_t cSecIndex = vld1q_s32(&chosenListNeon.secondIndex[j]);//二级索引比较
			int32x4_t cSecOpIndex = vld1q_s32(&bitmapQListsNeon[i].secondIndex[j]);
			int32x4_t comSecRes = vandq_s32(cSecIndex, cSecOpIndex);//按位与
			vst1q_s32(resSec, comSecRes);
			for (int k = 0; k < 4; k++)//得到4个4整数表示比较结果
				if (resSec[k] != 0)
					for (int t = (j + k) * 32; t < (j + k) * 32; t += 4)
					{
						alignas(16) int resFir[4];
						int32x4_t cFirIndex = vld1q_s32(&chosenListNeon.firstIndex[t]);//二级索引比较;//一级索引比较
						int32x4_t cFirOpIndex = vld1q_s32(&bitmapQListsNeon[i].firstIndex[t]);
						int32x4_t comFirRes = vandq_s32(cFirIndex, cFirOpIndex);//按位与

						vst1q_s32( resFir, comFirRes);//得到4个4整数表示比较结果
						for (int c = 0; c < 4; c++)
							if (resFir[c] != 0)
							{
								for (int l = (t + c) * 32; l < (t + c) * 32; l += 4)
								{
									int32x4_t cIndex = vld1q_s32(&chosenListNeon.firstIndex[l]);//关键字比较
									int32x4_t cOpIndex = vld1q_s32(&bitmapQListsNeon[i].firstIndex[l]);
									int32x4_t comRes = vandq_s32(cIndex, cOpIndex);//按位与
									vst1q_s32(& chosenListNeon.bits[l], comRes);
								}
							}
					}
		}
	}
	InvertedIndex x;
	return x;
}