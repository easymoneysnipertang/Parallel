#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include<omp.h>
#include"InvertedIndex.h"
#include"ThreadParam.h"
using namespace std;

// svs实现
InvertedIndex SVS(int* queryList, vector<InvertedIndex>& index, int num) 
{
	InvertedIndex s = index[queryList[0]];// 取最短的列表
	int count = 0;
	// 与剩余列表求交
	for (int i = 1; i < num; i++) {
		count = 0;// s从头往后遍历一遍
		int t = 0;
		// s列表中的每个元素都拿出来比较
		for (int j = 0; j < s.docIdList.size(); j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			for (; t < index[queryList[i]].docIdList.size(); t++) {
				// 遍历i列表中所有元素
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 升序排列
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
	InvertedIndex s = index[queryList[0]];// 取最短的列表
	int count = 0;
	// 与剩余列表求交
#pragma omp parallel num_threads(NUM_THREADS),shared(count)
	for (int i = 1; i < num; i++) {
		count = 0;// s从头往后遍历一遍
		int t = 0;
		// s列表中的每个元素都拿出来比较
#pragma omp for
		for (int j = 0; j < s.docIdList.size(); j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			for (; t < index[queryList[i]].docIdList.size(); t++) {
				// 遍历i列表中所有元素
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 升序排列
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