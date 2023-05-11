#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include<omp.h>
#include"InvertedIndex.h"
#include"ThreadParam.h"
using namespace std;

// svsʵ��
InvertedIndex SVS(int* queryList, vector<InvertedIndex>& index, int num) 
{
	InvertedIndex s = index[queryList[0]];// ȡ��̵��б�
	int count = 0;
	// ��ʣ���б���
	for (int i = 1; i < num; i++) {
		count = 0;// s��ͷ�������һ��
		int t = 0;
		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for (int j = 0; j < s.docIdList.size(); j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			for (; t < index[queryList[i]].docIdList.size(); t++) {
				// ����i�б�������Ԫ��
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��������
					break;
			}
			if (isFind)
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.docIdList.size())
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
	}
	return s;
}

InvertedIndex SVS_omp(int* queryList, vector<InvertedIndex>& index, int num)
{
	InvertedIndex s = index[queryList[0]];// ȡ��̵��б�
	int count = 0;
	// ��ʣ���б���
#pragma omp parallel num_threads(NUM_THREADS),shared(count)
	for (int i = 1; i < num; i++) {
		count = 0;// s��ͷ�������һ��
		int t = 0;
		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
#pragma omp for
		for (int j = 0; j < s.docIdList.size(); j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			for (; t < index[queryList[i]].docIdList.size(); t++) {
				// ����i�б�������Ԫ��
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��������
					break;
			}
#pragma omp critical
			if (isFind)
				s.docIdList[count++] = s.docIdList[j];
		}
#pragma omp single
		if (count < s.docIdList.size())
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
	}
	return s;
}