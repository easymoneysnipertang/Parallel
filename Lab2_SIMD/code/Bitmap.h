#pragma once
#include<string>
#include<list>
#include<vector>
#include<algorithm>
#include <mmintrin.h>   //mmx
#include <xmmintrin.h>  //sse
#include <emmintrin.h>  //sse2
#include <pmmintrin.h>  //sse3
#include"InvertedIndex.h"
struct BitMap
{
	vector<int> bits;			//id��Ӧλ
	vector<int> firstIndex;		//һ������
	vector<int> secondIndex;	//��������
	BitMap(int size= 25205200)
	{
		//���ٿռ�
		this->bits.resize(size / 32 + 1);
		this->firstIndex.resize(size / 1024 + 1);
		this->secondIndex.resize(size / 32768 + 1);
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
BitMap chosenList;
BitMap bitmapList[2000];//���ռ�
BitMap bitmapQLists[5];//���ռ�
list<int> skipJ;
void bitMapProcessing(vector<InvertedIndex>& invertedLists, int count)
{
	for (int i = 0; i < count; i++)
		for (int j = 0; j < invertedLists[i].length; j++)
			bitmapList[i].set(invertedLists[i].docIdList[j]);	//����id��Ӧbitmap
	//��������
	for (int i = 0; i < bitmapList[0].secondIndex.size(); i++)
		skipJ.push_back(i);

}
InvertedIndex S_BITMAP(int* list, vector<InvertedIndex>& idx, int num)
{
	for (int i = 0; i < num; i++)
		bitmapQLists[i] = bitmapList[list[i]];//����id��Ӧbitmap
	bool isZero = true;
	chosenList = bitmapQLists[0];
	for (int i = 1; i < num; i++)
	{
		for (int j = 0; j < bitmapQLists[0].secondIndex.size(); j++)
		{
			chosenList.secondIndex[j] &= bitmapQLists[i].secondIndex[j];//��λ��
			if (chosenList.secondIndex[j])
			{
				for (int t = j * 32; t < j * 32 + 32; t++)
				{
					chosenList.firstIndex[t] &= bitmapQLists[i].firstIndex[t];//��λ��
					if (chosenList.firstIndex[t])
					{
						for (int l = t * 32; l < t * 32 + 32; l++)
							chosenList.bits[l] &= bitmapQLists[i].bits[l];//��λ��
					}
				}
			}
			
		}
	}
	//�����ǽ�bitmapתΪdocId�Ĵ���
	//for (int i = 0; i < chosenList.bits.size(); i++)
	//{
	//	int cnt = i * 32;
	//	int bit = chosenList.bits[i];
	//	while (bit != 0)
	//	{
	//		if ((bit & 1) != 0)
	//			cout << cnt << ' ';
	//		bit = bit >> 1;
	//		++cnt;
	//	}
	//}
	InvertedIndex x;
	return x;
}

class BitMap2
{
public:
	BitMap2(int size = 25205200)
	{
		//���ٿռ�
		this->bits.resize(size / 32 + 1);
	}

	void set(int data)
	{
		int index0 = data / 32; //��������������
		int offset0 = data % 32; //����λ������

		this->bits[index0] |= (1 << offset0); //����4λ��Ϊ1
	}

	void reset(int data)
	{
		int index = data / 32;
		int temp = data % 32;
		this->bits[index] &= ~(1 << temp); //ȡ��
	}
	vector<int> bits;
};
BitMap2 chosenList2(25205200);

InvertedIndex compare2(int* list, vector<InvertedIndex>& idx, int num)
{
	vector<InvertedIndex> indexLists;
	for (int i = 0; i < num; i++)
	{
		indexLists.push_back(idx[list[i]]);
	}
	sort(indexLists.begin(), indexLists.end());
	vector<BitMap2> bitmap;
	for (int i = 0; i < num; i++)
	{
		for (int j = 0; j < indexLists[i].length; j++)
		{
			bitmap[i].set(indexLists[i].docIdList[j]);
		}
	}
	chosenList2 = bitmap[0];
	for (int i = 1; i < num; i++)
	{
		for (int j = 0; j < bitmap[0].bits.size(); j++)
			chosenList2.bits[j] &= bitmap[i].bits[j];
	}
	//for (int i = 0; i < chosenList.bits.size(); i++)
	//{
	//	int cnt = i * 32;
	//	int bit = chosenList.bits[i];
	//	while (bit != 0)
	//	{
	//		if ((bit & 1) != 0)
	//			cout << cnt << ' ';
	//		bit = bit >> 1;
	//		++cnt;
	//	}
	//}
	InvertedIndex x;
	return x;
}

InvertedIndex SKIP_BITMAP(int* queryList, vector<InvertedIndex>& idx, int num)
{
	for (int i = 0; i < num; i++)
		bitmapQLists[i] = bitmapList[queryList[i]];//����id��Ӧbitmap
	bool isZero = true;
	chosenList = bitmapQLists[0];
	for (int i = 1; i < num; i++)
	{
		list<int>::iterator tmp;
		list<int>::iterator j = skipJ.begin();
		while (j!=skipJ.end())
		{
			chosenList.secondIndex[*j] &= bitmapQLists[i].secondIndex[*j];//��λ��
			if (chosenList.secondIndex[*j])
			{
				for (int t = (*j) * 32; t < (*j) * 32 + 32; t++)
				{
					chosenList.firstIndex[t] &= bitmapQLists[i].firstIndex[t];//��λ��
					if (chosenList.firstIndex[t])
					{
						for (int l = t * 32; l < t * 32 + 32; l++)
							chosenList.bits[l] &= bitmapQLists[i].bits[l];//��λ��
					}
				}
				j++;
			}
			else
			{
				tmp = j;
				j++;
				skipJ.erase(tmp);
			}
		}
	}
	InvertedIndex x;
	return x;
}