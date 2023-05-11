#pragma once
#include<string>
#include<list>
#include<vector>
#include<algorithm>
#include <mmintrin.h>   //mmx
#include <xmmintrin.h>  //sse
#include <emmintrin.h>  //sse2
#include <pmmintrin.h>  //sse3
#include <omp.h>
#include"InvertedIndex.h"
extern vector<InvertedIndex> invertedLists;
struct BitMap
{
	vector<int> bits;			//id对应位
	vector<int> firstIndex;		//一级索引
	vector<int> secondIndex;	//二级索引
	BitMap(int size= 25205200)
	{
		//开辟空间
		this->bits.resize(size / 32 + 1);
		this->firstIndex.resize(size / 1024 + 1);
		this->secondIndex.resize(size / 32768 + 1);
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
BitMap bitmapList[2000];//开空间
BitMap chosenList;
list<int> skipPointer;
void bitMapProcessing(vector<InvertedIndex>& invertedLists, int count)
{
	for (int i = 0; i < count; i++)
		for (int j = 0; j < invertedLists[i].docIdList.size(); j++)
			bitmapList[i].set(invertedLists[i].docIdList[j]);	//建立id对应bitmap
	//建立跳表
	for (int i = 0; i < bitmapList[0].secondIndex.size(); i++)
		skipPointer.push_back(i);

}
InvertedIndex S_BITMAP(int* list, vector<InvertedIndex>& idx, int num)
{
	
	int len = bitmapList[list[0]].secondIndex.size();
	len = len / 4;

	for (int i = 0; i < 4; i++)
		copy(bitmapList[list[0]].secondIndex.begin() + len * i, bitmapList[list[0]].secondIndex.begin() + len * (i + 1), chosenList.secondIndex.begin() + len * i);
	len = bitmapList[list[0]].firstIndex.size();
	len = len / 4;

	for (int i = 0; i < 4; i++)
		copy(bitmapList[list[0]].firstIndex.begin() + len * i, bitmapList[list[0]].firstIndex.begin() + len * (i + 1), chosenList.firstIndex.begin() + len * i);
	len = bitmapList[list[0]].bits.size();
	len = len / 4;

	for (int i = 0; i < 4; i++)
		copy(bitmapList[list[0]].bits.begin() + len * i, bitmapList[list[0]].bits.begin() + len * (i + 1), chosenList.bits.begin() + len * i);
	
	//copy(bitmapList[list[0]].secondIndex.begin(), bitmapList[list[0]].secondIndex.end(), chosenList.secondIndex.begin());
	//copy(bitmapList[list[0]].firstIndex.begin(), bitmapList[list[0]].firstIndex.end(), chosenList.firstIndex.begin());
	//copy(bitmapList[list[0]].bits.begin(), bitmapList[list[0]].bits.end(), chosenList.bits.begin());

	//chosenList = bitmapList[list[0]];
	InvertedIndex res;
	for (int i = 1; i < num; i++)
	{
		for (int j = 0; j < chosenList.secondIndex.size(); j++)
		{
			chosenList.secondIndex[j] &= bitmapList[list[i]].secondIndex[j];//按位与
			if (chosenList.secondIndex[j])
			{
				for (int t = j * 32; t < j * 32 + 32; t++)
				{
					chosenList.firstIndex[t] &= bitmapList[list[i]].firstIndex[t];//按位与
					if (chosenList.firstIndex[t])
					{
						for (int l = t * 32; l < t * 32 + 32; l++)
						{
							chosenList.bits[l] &= bitmapList[list[i]].bits[l];//按位与
							if (i==num-1&&chosenList.bits[l])
							{
								int bit = chosenList.bits[l];//取出该段
								for (int m = 0; m < 32; m++)
								{
									if ((bit & 1) != 0)
										res.docIdList.push_back(32 * l + m);
										bit = bit >> 1;
								}
							}
						}
					}
				}
			}
		}
	}
	return res;
}
InvertedIndex S_BITMAP_omp(int* list, vector<InvertedIndex>& idx, int num)
{
#pragma omp parallel num_threads(NUM_THREADS)
	{
		int len = bitmapList[list[0]].secondIndex.size();
		len = len / 4;
#pragma omp for
		for (int i = 0; i < 4; i++)
			copy(bitmapList[list[0]].secondIndex.begin() + len * i, bitmapList[list[0]].secondIndex.begin() + len * (i + 1), chosenList.secondIndex.begin() + len * i);
		len = bitmapList[list[0]].firstIndex.size();
		len = len / 4;
#pragma omp for
		for (int i = 0; i < 4; i++)
			copy(bitmapList[list[0]].firstIndex.begin() + len * i, bitmapList[list[0]].firstIndex.begin() + len * (i + 1), chosenList.firstIndex.begin() + len * i);
		len = bitmapList[list[0]].bits.size();
		len = len / 4;
#pragma omp for
		for (int i = 0; i < 4; i++)
			copy(bitmapList[list[0]].bits.begin() + len * i, bitmapList[list[0]].bits.begin() + len * (i + 1), chosenList.bits.begin() + len * i);
	}
	//copy(bitmapList[list[0]].secondIndex.begin(), bitmapList[list[0]].secondIndex.end(), chosenList.secondIndex.begin());
	//copy(bitmapList[list[0]].firstIndex.begin(), bitmapList[list[0]].firstIndex.end(), chosenList.firstIndex.begin());
	//copy(bitmapList[list[0]].bits.begin(), bitmapList[list[0]].bits.end(), chosenList.bits.begin());

	//chosenList = bitmapList[list[0]];
	InvertedIndex res;
	for (int i = 1; i < num; i++)
	{
		for (int j = 0; j < chosenList.secondIndex.size(); j++)
		{
			chosenList.secondIndex[j] &= bitmapList[list[i]].secondIndex[j];//按位与
			if (chosenList.secondIndex[j])
			{
				for (int t = j * 32; t < j * 32 + 32; t++)
				{
					chosenList.firstIndex[t] &= bitmapList[list[i]].firstIndex[t];//按位与
					if (chosenList.firstIndex[t])
					{
						for (int l = t * 32; l < t * 32 + 32; l++)
						{
							chosenList.bits[l] &= bitmapList[list[i]].bits[l];//按位与
							if (i == num - 1 && chosenList.bits[l])
							{
								int bit = chosenList.bits[l];//取出该段
								for (int m = 0; m < 32; m++)
								{
									if ((bit & 1) != 0)
										res.docIdList.push_back(32 * l + m);
									bit = bit >> 1;
								}
							}
						}
					}
				}
			}
		}
	}
	return res;
}
//class BitMap2
//{
//public:
//	BitMap2(int size = 25205200)
//	{
//		//开辟空间
//		this->bits.resize(size / 32 + 1);
//	}
//
//	void set(int data)
//	{
//		int index0 = data / 32; //数组索引即区间
//		int offset0 = data % 32; //具体位置索引
//
//		this->bits[index0] |= (1 << offset0); //左移4位置为1
//	}
//
//	void reset(int data)
//	{
//		int index = data / 32;
//		int temp = data % 32;
//		this->bits[index] &= ~(1 << temp); //取反
//	}
//	vector<int> bits;
//};
//for (int i = 0; i < bitmapList[list[0]].bits.size(); i++)
//{
//	int cnt = i * 32;
//	int bit = bitmapList[list[0]].bits[i];
//	while (bit != 0)
//	{
//		if ((bit & 1) != 0)
//			cout << cnt << ' ';
//		bit = bit >> 1;
//		++cnt;
//	}
//}
InvertedIndex SKIP_BITMAP(int* queryList, vector<InvertedIndex>& idx, int num)
{
	InvertedIndex res;
	chosenList = bitmapList[queryList[0]];
	for (int i = 1; i < num; i++)
	{
		list<int>::iterator tmp;
		list<int>::iterator j = skipPointer.begin();
		while (j!=skipPointer.end())
		{
			chosenList.secondIndex[*j] &= bitmapList[queryList[i]].secondIndex[*j];//按位与
			if (chosenList.secondIndex[*j])
			{
				for (int t = (*j) * 32; t < (*j) * 32 + 32; t++)
				{
					chosenList.firstIndex[t] &= bitmapList[queryList[i]].firstIndex[t];//按位与
					if (chosenList.firstIndex[t])
					{
						for (int l = t * 32; l < t * 32 + 32; l++)
						{
							chosenList.bits[l] &= bitmapList[queryList[i]].bits[l];//按位与
							if (i == num - 1 && chosenList.bits[l])
							{
								int bit = chosenList.bits[l];//取出该段
								for (int m = 0; m < 32; m++)
								{
									if ((bit & 1) != 0)
										res.docIdList.push_back(32 * l + m);
									bit = bit >> 1;
								}
							}
						}
							
					}
				}
				j++;
			}
			else
			{
				tmp = j;
				j++;
				skipPointer.erase(tmp);
			}
		}
	}
	return res;
}