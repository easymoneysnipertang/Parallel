#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include"InvertedIndex.h"
#include<omp.h>

bool isParallel_in=false;
// hash分段实现
struct HashBucket {// 一个hash段，记录他在倒排列表中的起始位置
	int begin = -1;
	int end = -1;
};
HashBucket** hashBucket;
// 预处理，将倒排列表放入hash段里
void preprocessing(vector<InvertedIndex>& index, int count) {
	hashBucket = new HashBucket * [count];
	for (int i = 0; i < count; i++)
		hashBucket[i] = new HashBucket[407000];// 26000000/65536

	for (int i = 0; i < count; i++) {
		for (int j = 0; j < index[i].docIdList.size(); j++) {
			int value = index[i].docIdList[j];// 拿出当前列表第j个值
			int hashValue = value/64 ;// TODO:check
			if (hashBucket[i][hashValue].begin == -1) // 该段内还没有元素
				hashBucket[i][hashValue].begin = j;
			hashBucket[i][hashValue].end = j;// 添加到该段尾部
		}
	}
}
InvertedIndex HASH(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// 选出最短的倒排列表
	int count = 0;
	for (int i = 1; i < num; i++) {// 与剩余列表求交
		// SVS的思想，将s挨个按表求交
		count = 0;
		int length = s.docIdList.size();
		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j]/64 ;
			// 找到该hash值在当前待求交列表中对应的段
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// 该值肯定找不到，不用往后做了
				continue;
			}
			else {
				for (begin; begin <= end; begin++) {
					if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
						// 在该段中找到了当前值
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin]) {
						break;
					}
				}
				if (isFind) {
					// 覆盖
					s.docIdList[count++] = s.docIdList[j];
				}
			}
		}
		if (count < length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
	}

	return s;
}

InvertedIndex HASH_omp(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// 选出最短的倒排列表
	int count = 0;
#pragma omp parallel num_threads(NUM_THREADS),shared(count)
	for (int i = 1; i < num; i++) {// 与剩余列表求交
		// SVS的思想，将s挨个按表求交
		count = 0;
		int length = s.docIdList.size();
#pragma omp for 
		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j] / 64;
			// 找到该hash值在当前待求交列表中对应的段
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// 该值肯定找不到，不用往后做了
				continue;
			}
			else {
				for (begin; begin <= end; begin++) {
					if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
						// 在该段中找到了当前值
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin]) {
						break;
					}
				}
#pragma omp critical
				if (isFind) {
					// 覆盖
					s.docIdList[count++] = s.docIdList[j];
				}
			}
		}
#pragma omp single
		if (count < length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
	}

	return s;
}