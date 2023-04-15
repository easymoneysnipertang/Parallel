#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include <xmmintrin.h>  //sse
#include <emmintrin.h>  //sse2
#include <pmmintrin.h>  //sse3
#include <immintrin.h>
#include"Adp.h"
#include"InvertedIndex.h"

// adp_avx
InvertedIndex ADP_AVX(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].length;
	}

	while (list[0].cursor < list[0].length) {// 最短的列表非空
		bool isFind = true;
		int s = 1;
		unsigned int e = index[list[0].key].docIdList[list[0].cursor];
		__m256i ee = _mm256_set1_epi32(e);

		while (s != num && isFind == true) {
			isFind = false;
			while (list[s].cursor < list[s].length - 7) {// 检查s列表
				// 构造向量
				__m256i ii = _mm256_loadu_epi32(&index[list[s].key].docIdList[list[s].cursor]);
				__m256i result = _mm256_set1_epi32(0);
				result = _mm256_cmpeq_epi32(ee, ii);
				int mask = _mm256_movemask_epi8(result);// 得到掩码
				if (mask != 0) {// 查看比较结果
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])// 及时break，避免重置cursor
					break;
				list[s].cursor += 8;
			}
			if (!isFind) {
				while (list[s].cursor < list[s].length) {// 剩余字段
					if (e == index[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < index[list[s].key].docIdList[list[s].cursor])
						break;
					list[s].cursor++;// 当前访问过，且没找到合适的，往后移
				}
			}
			s++;// 下一个链表
		}
		list[0].cursor++;// 当前元素已被访问过
		if (s == num && isFind) {
			S.docIdList.push_back(e);
			S.length++;
		}
		//sort(list, list + num);// 重排，将未探查元素少的列表前移
	}
	return S;
}
