#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include <xmmintrin.h>  //sse
#include <emmintrin.h>  //sse2
#include <pmmintrin.h>  //sse3
#include <immintrin.h>

#include"InvertedIndex.h"
InvertedIndex SVS_SSE(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// 取最短的列表

	// 与剩余列表求交
	for (int i = 1; i < num; i++) {
		int count = 0;// s从头往后遍历一遍
		int t = 0;
		// s列表中的每个元素都拿出来比较
		for (int j = 0; j < s.length; j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			__m128i ss = _mm_set1_epi32(s.docIdList[j]);// 填充32位

			for (; t < index[queryList[i]].length - 3; t += 4) {// 同时与四个元素比较
				// 遍历i列表中所有元素
				__m128i ii; 
				ii = _mm_load_si128((__m128i*) & index[queryList[i]].docIdList[t]);// 一次取四个
				__m128i tmp = _mm_set1_epi32(0);
				tmp = _mm_cmpeq_epi32(ss, ii);// 比较向量每一位
				int mask = _mm_movemask_epi8(tmp);// 转为掩码，如果有一个数相等,就不会是全0

				if (mask != 0) {// 查看比较结果
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 及时break，避免超过，下一元素需重头再来
					break;
			}
			if (!isFind && (t >= index[queryList[i]].length - 3)) {// 处理剩余元素
				for (; t < index[queryList[i]].length; t++)
				{
					if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 升序排列
						break;
				}
			}
			if (isFind)// 覆盖
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}
	return s;
}
