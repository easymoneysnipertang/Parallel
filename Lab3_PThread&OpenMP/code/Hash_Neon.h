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
using namespace std;
InvertedIndex HASH_NEON(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// 选出最短的倒排列表
	for (int i = 1; i < num; i++) {// 与剩余列表求交
		// SVS的思想，将s挨个按表求交
		int count = 0;
		int length = s.length;

		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j] / 512;
			// 找到该hash值在当前待求交列表中对应的段
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// 该值肯定找不到，不用往后做了
				continue;
			}
			else {// 在该段里进行查找，一次查4个
				uint32x4_t ss = vmovq_n_u32(s.docIdList[j]);// 填充四个位置

				for (begin; begin <= end - 3; begin += 4) {
					uint32x4_t ii = vld1q_u32(&index[i].docIdList[begin]);// 读取四个数据

					uint32x4_t temp = vceqq_u32(ss, ii);
					unsigned int result[4] = { 0 };
					vst1q_u32(result, temp);
					if (result[0] == 1 || result[1] == 1 || result[2] == 1 || result[3] == 1) {
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin])// 及时break，避免超过，下一元素需重头再来
						break;
				}
				if (!isFind && (begin > end - 3)) {// 处理剩余元素
					for (; begin <= end; begin++)
					{
						if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
							isFind = true;
							break;
						}
						else if (s.docIdList[j] < index[queryList[i]].docIdList[begin])// 升序排列
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
		s.length = count;
	}

	return s;
}