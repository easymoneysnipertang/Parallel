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
	alignas(16) vector<int> bits;			//id��Ӧλ
	alignas(16) vector<int> firstIndex;		//һ������
	alignas(16) vector<int> secondIndex;	//��������
	BitMapNeon(int size = 25205200)
	{
		//���ٿռ�
		this->bits.resize(((size / 32 + 1) / 4 + 1) * 4);
		this->firstIndex.resize(((size / 1024 + 1) / 4 + 1) * 4);
		this->secondIndex.resize(((size / 32768 + 1) / 4 + 1) * 4);
	}

	void set(int x) //��x��Ӧ��������λ
	{
		//����λ������
		int index0 = x / 32; //��������������
		int index1 = index0 / 1024;
		int index2 = index1 / 1024;
		int offset0 = x % 32;
		int offset1 = offset0 / 32;
		int offset2 = offset1 / 32;

		this->bits[index0] |= (1 << offset0); //����,��λΪ1
		this->firstIndex[index1] |= (1 << offset1);
		this->secondIndex[index2] |= (1 << offset2);
	}
};
alignas(16) BitMapNeon chosenListNeon;//ѡ�е��б�
alignas(16) BitMapNeon bitmapListNeon[2000];//���ռ�
alignas(16) BitMapNeon bitmapQListsNeon[5];//���ռ�

void bitMapNeonProcessing(vector<InvertedIndex>& invertedLists, int count)
{
	for (int i = 0; i < count; i++)
		for (int j = 0; j < invertedLists[i].length; j++)
			bitmapListNeon[i].set(invertedLists[i].docIdList[j]);	//����id��Ӧbitmap
}
InvertedIndex BITMAP_NEON(int* list, vector<InvertedIndex>& idx, int num)
{

	for (int i = 0; i < num; i++)
		bitmapQListsNeon[i] = bitmapListNeon[list[i]];//����id��Ӧbitmap
	chosenListNeon = bitmapQListsNeon[0];
	for (int i = 1; i < num; i++)
	{
		for (int j = 0; j < bitmapQListsNeon[0].secondIndex.size(); j += 4)
		{
			alignas(16) int resSec[4];
			int32x4_t cSecIndex = vld1q_s32(&chosenListNeon.secondIndex[j]);//���������Ƚ�
			int32x4_t cSecOpIndex = vld1q_s32(&bitmapQListsNeon[i].secondIndex[j]);
			int32x4_t comSecRes = vandq_s32(cSecIndex, cSecOpIndex);//��λ��
			vst1q_s32(resSec, comSecRes);
			for (int k = 0; k < 4; k++)//�õ�4��4������ʾ�ȽϽ��
				if (resSec[k] != 0)
					for (int t = (j + k) * 32; t < (j + k) * 32; t += 4)
					{
						alignas(16) int resFir[4];
						int32x4_t cFirIndex = vld1q_s32(&chosenListNeon.firstIndex[t]);//���������Ƚ�;//һ�������Ƚ�
						int32x4_t cFirOpIndex = vld1q_s32(&bitmapQListsNeon[i].firstIndex[t]);
						int32x4_t comFirRes = vandq_s32(cFirIndex, cFirOpIndex);//��λ��

						vst1q_s32( resFir, comFirRes);//�õ�4��4������ʾ�ȽϽ��
						for (int c = 0; c < 4; c++)
							if (resFir[c] != 0)
							{
								for (int l = (t + c) * 32; l < (t + c) * 32; l += 4)
								{
									int32x4_t cIndex = vld1q_s32(&chosenListNeon.firstIndex[l]);//�ؼ��ֱȽ�
									int32x4_t cOpIndex = vld1q_s32(&bitmapQListsNeon[i].firstIndex[l]);
									int32x4_t comRes = vandq_s32(cIndex, cOpIndex);//��λ��
									vst1q_s32(& chosenListNeon.bits[l], comRes);
								}
							}
					}
		}
	}
	InvertedIndex x;
	return x;
}