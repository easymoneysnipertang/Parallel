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
	alignas(16) vector<int> bits;			//id��Ӧλ
	alignas(16) vector<int> firstIndex;		//һ������
	alignas(16) vector<int> secondIndex;	//��������
	BitMapSSE(int size= 25205200)
	{
		//���ٿռ�
		this->bits.resize(((size / 32 + 1) / 4 + 1)*4);
		this->firstIndex.resize(((size / 1024 + 1) / 4 + 1)*4);
		this->secondIndex.resize(((size / 32768 + 1) / 4 + 1)*4);
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
alignas(16) BitMapSSE bitmapListSSE[2000] ;//���ռ�
alignas(16) BitMapSSE bitmapQListsSSE[];//���ռ�

void bitMapSSEProcessing(vector<InvertedIndex>& invertedLists, int count)
{
	for (int i = 0; i < count; i++)
		for (int j = 0; j < invertedLists[i].docIdList.size(); j++)
			bitmapListSSE[i].set(invertedLists[i].docIdList[j]);	//����id��Ӧbitmap
	for (int i = 0; i < count; i++)
		bitmapQListsSSE[i] = bitmapListSSE[i];//����id��Ӧbitmap
}
InvertedIndex BITMAP_SSE(int* list, vector<InvertedIndex>& idx, int num)
{
	
	for (int i = 1; i < num; i++)
	{
		for (int j = 0; j < bitmapQListsSSE[0].secondIndex.size(); j+=4)
		{
			alignas(16) int resSec[4]; 
			__m128i cSecIndex = _mm_loadu_epi32(&bitmapQListsSSE[list[0]].secondIndex[j]);//���������Ƚ�
			__m128i cSecOpIndex = _mm_loadu_epi32(&bitmapQListsSSE[list[i]].secondIndex[j]);
			__m128i comSecRes = _mm_and_si128(cSecIndex, cSecOpIndex);//��λ��
			_mm_store_si128((__m128i*) resSec, comSecRes);
			for(int k=0;k<4;k++)
				if(resSec[k]!=0)
					for (int t = (j+k) * 32; t < (j+k) * 32 ; t+=4)
					{
						alignas(16) int resFir[4];
						__m128i cFirIndex = _mm_loadu_epi32(&bitmapQListsSSE[list[0]].firstIndex[t]);//���������Ƚ�;//һ�������Ƚ�
						__m128i cFirOpIndex = _mm_loadu_epi32(&bitmapQListsSSE[list[i]].firstIndex[t]);
						__m128i comFirRes = _mm_and_si128(cFirIndex, cFirOpIndex);//��λ��

						_mm_store_si128((__m128i*) resFir, comFirRes);//�õ�4��4������ʾ�ȽϽ��
						for(int c=0;c<4;c++)
							if (resFir[c]!=0)
							{
								for (int l = (t+c) * 32; l < (t+c) * 32 ; l+=4)
								{
									__m128i cIndex = _mm_loadu_epi32(&bitmapQListsSSE[list[0]].firstIndex[l]);//�ؼ��ֱȽ�
									__m128i cOpIndex = _mm_loadu_epi32(&bitmapQListsSSE[list[i]].firstIndex[l]);
									__m128i comRes = _mm_and_si128(cIndex, cOpIndex);//��λ��
									_mm_store_si128((__m128i*)&bitmapQListsSSE[list[0]].bits[l], comRes);
								}
							}
					}
		}
	}
	InvertedIndex x;
	return x;
}

//��������SIMDֻչ��һ��,ʵ��Ч�ʽϵ�
//InvertedIndex BITMAPSSE2(int* list, vector<InvertedIndex>& idx, int num)
//{
//	for (int i = 0; i < num; i++)
//		bitmapQListsSSE[i] = bitmapListSSE[list[i]];//����id��Ӧbitmap
//	bitmapQListsSSE[list[0]] = bitmapQListsSSE[0];
//	for (int i = 1; i < num; i++)
//	{
//		for (int j = 0; j < bitmapQListsSSE[list[0]].secondIndex.size(); j++)
//		{
//			bitmapQListsSSE[list[0]].secondIndex[j] &= bitmapQListsSSE[i].secondIndex[j];//��λ��
//			if (bitmapQListsSSE[list[0]].secondIndex[j])
//			{
//				for (int t = j * 32; t < j * 32 + 32; t++)
//				{
//					bitmapQListsSSE[list[0]].firstIndex[t] &= bitmapQListsSSE[i].firstIndex[t];//��λ��
//					if (bitmapQListsSSE[list[0]].firstIndex[t])
//					{
//						for (int l = t * 32; l < t * 32 + 32; l+=4)
//						{
//							__m128i cIndex = _mm_load_si128((__m128i*) & bitmapQListsSSE[list[0]].bits[l]);//��λ��
//							__m128i cOpIndex = _mm_load_si128((__m128i*) & bitmapQListsSSE[i].bits[l]);
//							__m128i comRes = _mm_and_si128(cIndex, cOpIndex);
//							_mm_store_si128((__m128i*) & bitmapQListsSSE[list[0]].bits[l], comRes);
//						}
//					}
//				}
//			}
//		}
//	}
//	//�����ǽ�bitmapתΪdocId�Ĵ���
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