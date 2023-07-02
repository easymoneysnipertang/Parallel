#pragma once
#include<iostream>
#include<bitset>
#include<vector>
#include<string>
#include<omp.h>
#include<fstream>
#include <bitset>
#include <algorithm>
#include<sstream>
#include<math.h>
#include<emmintrin.h>
#include"bitOp.h"
#include"dgap.h"

#define PFOR_THRESHOLD 0.9
using namespace std;
void pfdCompress(const vector<unsigned>& invertedIndex, vector<unsigned>& result, int& idx)
{
	vector<unsigned> deltaId;
	dgapTransform(invertedIndex, deltaId);

	vector<unsigned> iCopy = deltaId;
	//-----------------------------------计算a，b的值-----------------------------------
	//排序
	sort(iCopy.begin(), iCopy.end());

	//普通数据长度
	unsigned normalLen = (unsigned)iCopy.size();
	if (normalLen == 0)
		return;

	//普通数据使用b位存储，b最大32，也就是6位
	unsigned b = max(ceil(log2(iCopy[(unsigned)((normalLen-1) * PFOR_THRESHOLD)] + 1)),1);

	//异常数据还要加上a位, 最后一位的位数减b，a最多32，6位
	unsigned a = ceil(log2((iCopy[normalLen-1]+1))) - b;

	//异常值下标
	vector<unsigned> exceptionIndex;

	//-----------------------------------写入result，结构：正常长度(normalLen)+位数(b)+正常数据1，2，3...-----------------------------------
	//正常部分
	int normalBitCnt = 32 + 6 + normalLen * b;
	result.resize( (int)ceil((idx +normalBitCnt) / 32.0));//原有+新加入bit

	writeBitData(result, idx, normalLen, 32);//写入元素个数
	writeBitData(result, idx + 32, b, 6); //写入元素位数

	idx += 38;//从index+38位开始写正常数据
	for (int i = 0; i < normalLen; i++)
	{
		unsigned mask = ((long long)pow(2, b) - 1);// 害怕了，到时候瞅瞅能不能改吧
		unsigned low = deltaId[i] & mask;//保留低b位

		writeBitData(result, idx, low, b);
		idx += b;
		if (deltaId[i] > mask)
		{
			//记录下标
			exceptionIndex.push_back(i);
		}
	}

	//----------------------写入result，结构：异常长度(exceptionLen)+ 索引位数+ 位数(a)+索引1，异常数据1，索引2，异常数据2，...-------------------------
	//异常部分
	unsigned exceptionLen = (unsigned)exceptionIndex.size();
	if (exceptionLen == 0)
	{
		result.resize((int)ceil((idx + 32) / 32.0));
		writeBitData(result, idx, 0, 32);//写入元素个数
		idx += 32;
		return;
	}

	//异常数据的索引的位数,最多32，也就是6位
	unsigned exceptionIdxBitNum = ceil(log2((exceptionIndex[exceptionLen-1] + 1)));

	//更新长度
	unsigned exceptionBitCnt = 32 + 6 + 6 + exceptionLen * (exceptionIdxBitNum + a);
	result.resize((int)ceil((idx + exceptionBitCnt) / 32.0));

	writeBitData(result, idx, exceptionLen, 32);//写入元素个数
	writeBitData(result, idx + 32, exceptionIdxBitNum, 6); //写入索引位数
	writeBitData(result, idx + 38, a, 6); //写入元素位数

	idx += 44;//从index+44位开始写异常数据
	for (int i = 0; i < exceptionLen; i++)
	{
		int expIdx = exceptionIndex[i];

		unsigned mask = ((long long)pow(2, a) - 1);// 害怕了，到时候瞅瞅能不能改吧
		unsigned high = (deltaId[expIdx]>>b)&mask;//保留高a位

		writeBitData(result, idx, expIdx, exceptionIdxBitNum);
		writeBitData(result, idx+ exceptionIdxBitNum, high, a);
		idx += exceptionIdxBitNum + a;
	}
}

// invertedIndex: pfd压缩好的倒排索引
// result：解缩后的索引
// idx：从第idx个bit开始解压
// return：解压后大小（bit)
vector<unsigned> pfdDecompress(const vector<unsigned>& compressedLists, int& idx)
{
	vector<unsigned> vec_u_deltaId; //deltaId 数组

	//正常部分
	//-----------------------------------写入result，结构：正常长度(normalLen)+位数(b)+正常数据1，2，3...-----------------------------------
	//读长度
	unsigned u_normalLen = readBitData(compressedLists, idx, 32);
	if (u_normalLen == 0)
		return vec_u_deltaId;

	//读位数
	unsigned u_normalBitNum = readBitData(compressedLists, idx + 32, 6);
	idx += 38;

	//读正常数据
	vec_u_deltaId.reserve(u_normalLen);
	for (int i = 0; i < u_normalLen; i++)
	{
		unsigned u_deltaId = readBitData(compressedLists, idx, u_normalBitNum);
		idx += u_normalBitNum;
		vec_u_deltaId.push_back(u_deltaId);//存入数组
	}

	//读异常数据
	//----------------------写入result，结构：异常长度(exceptionLen)+ 索引位数+ 位数(a)+索引1，异常数据1，索引2，异常数据2，...-------------------------
	//读长度
	unsigned u_exceptionLen = readBitData(compressedLists, idx, 32);
	if (u_exceptionLen == 0) //没有异常数据，无需再读
	{
		idx += 32;
		for (int i = 1; i < u_normalLen; i++)//还原，结束
		{
			vec_u_deltaId[i] += vec_u_deltaId[i - 1];
		}
		return vec_u_deltaId;
	}
	//读索引位数
	unsigned u_exceptionIndexBitNum = readBitData(compressedLists, idx + 32, 6);

	//读异常数据位数
	unsigned u_exceptionDataBitNum = readBitData(compressedLists, idx + 38, 6);
	idx += 44;

	//读索引，再读数据交替
	for (int i = 0; i < u_exceptionLen; i++)
	{
		unsigned u_exceptionIndex = readBitData(compressedLists, idx, u_exceptionIndexBitNum);
		unsigned u_exceptionHigh = readBitData(compressedLists, idx+ u_exceptionIndexBitNum, u_exceptionDataBitNum);
		idx += u_exceptionIndexBitNum + u_exceptionDataBitNum;

		vec_u_deltaId[u_exceptionIndex] |= (u_exceptionHigh << u_normalBitNum);//将normal位留出来，取或还原
	}

	//------------------------------------使用dgap逆变换还原docId-----------------------------------------------
	for (int i = 1; i < u_normalLen; i++)
	{
		vec_u_deltaId[i] += vec_u_deltaId[i - 1];
	}

	return vec_u_deltaId;
}