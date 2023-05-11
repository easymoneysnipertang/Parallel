#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include <xmmintrin.h>  //sse
#include <emmintrin.h>  //sse2
#include <pmmintrin.h>  //sse3
#include <immintrin.h>

#include"InvertedIndex.h"

// svs_sse_notAlign
InvertedIndex SVS_NotAlign(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// 取最短的列表

	// 与剩余列表求交
	for (int i = 1; i < num; i++) {
		int count = 0;// s从头往后遍历一遍
		int t = 0;
		// s列表中的每个元素都拿出来比较
		for (int j = 0; j < s.docIdList.size(); j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			__m128i ss = _mm_set1_epi32(s.docIdList[j]);// 填充32位

			// 测试不对齐的表现
			// 在开始把模4剩余的部分处理掉，以后每次加四保证都能访问

			for (; t < 2; t++) {
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 升序排列
					break;
			}

			//cout << &index[queryList[i]].docIdList[0] << endl << endl;
			for (; t < index[queryList[i]].docIdList.size()-3; t += 4) {// 同时与四个元素比较
				// 遍历i列表中所有元素
				__m128i ii;
				ii = _mm_loadu_epi32(&index[queryList[i]].docIdList[t]);// 一次取四个
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
			if (!isFind && (t >= index[queryList[i]].docIdList.size() - 3)) {// 处理剩余元素
				for (; t < index[queryList[i]].docIdList.size(); t++)
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
		if (count < s.docIdList.size())// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
	}
	return s;
}

// adp_sse_notAlign
InvertedIndex ADP_NotAlign(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].docIdList.size();
	}

	while (list[0].cursor < list[0].length) {// 最短的列表非空
		bool isFind = true;
		int s = 1;
		unsigned int e = index[list[0].key].docIdList[list[0].cursor];
		__m128i ee = _mm_set1_epi32(e);

		while (s != num && isFind == true) {
			isFind = false;
			// 测试不对齐的表现
			// 在开始把模4剩余的部分处理掉，以后每次加四保证都能访问
			int tail = list[s].length % 4;
			while (list[s].cursor < tail) {// 剩余字段
				if (e == index[list[s].key].docIdList[list[s].cursor]) {
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])
					break;
				list[s].cursor++;// 当前访问过，且没找到合适的，往后移
			}

			while (list[s].cursor < list[s].length) {// 检查s列表
				// 构造向量
				__m128i ii = _mm_loadu_epi32(&index[list[s].key].docIdList[list[s].cursor]);
				__m128i result = _mm_set1_epi32(0);
				result = _mm_cmpeq_epi32(ee, ii);
				int mask = _mm_movemask_epi8(result);// 得到掩码
				if (mask != 0) {// 查看比较结果
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])// 及时break，避免重置cursor
					break;
				list[s].cursor += 4;
			}

			s++;// 下一个链表
		}
		list[0].cursor++;// 当前元素已被访问过
		if (s == num && isFind) 
			S.docIdList.push_back(e);
		//sort(list, list + num);// 重排，将未探查元素少的列表前移
	}
	return S;
}