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
	//-----------------------------------����a��b��ֵ-----------------------------------
	//����
	sort(iCopy.begin(), iCopy.end());

	//��ͨ���ݳ���
	unsigned normalLen = (unsigned)iCopy.size();
	if (normalLen == 0)
		return;

	//��ͨ����ʹ��bλ�洢��b���32��Ҳ����6λ
	unsigned b = max(ceil(log2(iCopy[(unsigned)((normalLen-1) * PFOR_THRESHOLD)] + 1)),1);

	//�쳣���ݻ�Ҫ����aλ, ���һλ��λ����b��a���32��6λ
	unsigned a = ceil(log2((iCopy[normalLen-1]+1))) - b;

	//�쳣ֵ�±�
	vector<unsigned> exceptionIndex;

	//-----------------------------------д��result���ṹ����������(normalLen)+λ��(b)+��������1��2��3...-----------------------------------
	//��������
	int normalBitCnt = 32 + 6 + normalLen * b;
	result.resize( (int)ceil((idx +normalBitCnt) / 32.0));//ԭ��+�¼���bit

	writeBitData(result, idx, normalLen, 32);//д��Ԫ�ظ���
	writeBitData(result, idx + 32, b, 6); //д��Ԫ��λ��

	idx += 38;//��index+38λ��ʼд��������
	for (int i = 0; i < normalLen; i++)
	{
		unsigned mask = ((long long)pow(2, b) - 1);// �����ˣ���ʱ�����ܲ��ܸİ�
		unsigned low = deltaId[i] & mask;//������bλ

		writeBitData(result, idx, low, b);
		idx += b;
		if (deltaId[i] > mask)
		{
			//��¼�±�
			exceptionIndex.push_back(i);
		}
	}

	//----------------------д��result���ṹ���쳣����(exceptionLen)+ ����λ��+ λ��(a)+����1���쳣����1������2���쳣����2��...-------------------------
	//�쳣����
	unsigned exceptionLen = (unsigned)exceptionIndex.size();
	if (exceptionLen == 0)
	{
		result.resize((int)ceil((idx + 32) / 32.0));
		writeBitData(result, idx, 0, 32);//д��Ԫ�ظ���
		idx += 32;
		return;
	}

	//�쳣���ݵ�������λ��,���32��Ҳ����6λ
	unsigned exceptionIdxBitNum = ceil(log2((exceptionIndex[exceptionLen-1] + 1)));

	//���³���
	unsigned exceptionBitCnt = 32 + 6 + 6 + exceptionLen * (exceptionIdxBitNum + a);
	result.resize((int)ceil((idx + exceptionBitCnt) / 32.0));

	writeBitData(result, idx, exceptionLen, 32);//д��Ԫ�ظ���
	writeBitData(result, idx + 32, exceptionIdxBitNum, 6); //д������λ��
	writeBitData(result, idx + 38, a, 6); //д��Ԫ��λ��

	idx += 44;//��index+44λ��ʼд�쳣����
	for (int i = 0; i < exceptionLen; i++)
	{
		int expIdx = exceptionIndex[i];

		unsigned mask = ((long long)pow(2, a) - 1);// �����ˣ���ʱ�����ܲ��ܸİ�
		unsigned high = (deltaId[expIdx]>>b)&mask;//������aλ

		writeBitData(result, idx, expIdx, exceptionIdxBitNum);
		writeBitData(result, idx+ exceptionIdxBitNum, high, a);
		idx += exceptionIdxBitNum + a;
	}
}

// invertedIndex: pfdѹ���õĵ�������
// result�������������
// idx���ӵ�idx��bit��ʼ��ѹ
// return����ѹ���С��bit)
vector<unsigned> pfdDecompress(const vector<unsigned>& compressedLists, int& idx)
{
	vector<unsigned> vec_u_deltaId; //deltaId ����

	//��������
	//-----------------------------------д��result���ṹ����������(normalLen)+λ��(b)+��������1��2��3...-----------------------------------
	//������
	unsigned u_normalLen = readBitData(compressedLists, idx, 32);
	if (u_normalLen == 0)
		return vec_u_deltaId;

	//��λ��
	unsigned u_normalBitNum = readBitData(compressedLists, idx + 32, 6);
	idx += 38;

	//����������
	vec_u_deltaId.reserve(u_normalLen);
	for (int i = 0; i < u_normalLen; i++)
	{
		unsigned u_deltaId = readBitData(compressedLists, idx, u_normalBitNum);
		idx += u_normalBitNum;
		vec_u_deltaId.push_back(u_deltaId);//��������
	}

	//���쳣����
	//----------------------д��result���ṹ���쳣����(exceptionLen)+ ����λ��+ λ��(a)+����1���쳣����1������2���쳣����2��...-------------------------
	//������
	unsigned u_exceptionLen = readBitData(compressedLists, idx, 32);
	if (u_exceptionLen == 0) //û���쳣���ݣ������ٶ�
	{
		idx += 32;
		for (int i = 1; i < u_normalLen; i++)//��ԭ������
		{
			vec_u_deltaId[i] += vec_u_deltaId[i - 1];
		}
		return vec_u_deltaId;
	}
	//������λ��
	unsigned u_exceptionIndexBitNum = readBitData(compressedLists, idx + 32, 6);

	//���쳣����λ��
	unsigned u_exceptionDataBitNum = readBitData(compressedLists, idx + 38, 6);
	idx += 44;

	//���������ٶ����ݽ���
	for (int i = 0; i < u_exceptionLen; i++)
	{
		unsigned u_exceptionIndex = readBitData(compressedLists, idx, u_exceptionIndexBitNum);
		unsigned u_exceptionHigh = readBitData(compressedLists, idx+ u_exceptionIndexBitNum, u_exceptionDataBitNum);
		idx += u_exceptionIndexBitNum + u_exceptionDataBitNum;

		vec_u_deltaId[u_exceptionIndex] |= (u_exceptionHigh << u_normalBitNum);//��normalλ��������ȡ��ԭ
	}

	//------------------------------------ʹ��dgap��任��ԭdocId-----------------------------------------------
	for (int i = 1; i < u_normalLen; i++)
	{
		vec_u_deltaId[i] += vec_u_deltaId[i - 1];
	}

	return vec_u_deltaId;
}