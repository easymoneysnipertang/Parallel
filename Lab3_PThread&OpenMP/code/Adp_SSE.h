#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include <mmintrin.h>   //mmx
#include <xmmintrin.h>  //sse
#include <emmintrin.h>  //sse2
#include <pmmintrin.h>  //sse3
#include <immintrin.h>
#include"InvertedIndex.h"
#include"Adp.h"
InvertedIndex ADP_SSE(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// Ԥ����
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].length = index[queryList[i]].docIdList.size();
	}

	while (list[0].cursor < list[0].length) {// ��̵��б�ǿ�
		bool isFind = true;
		int s = 1;
		unsigned int e = index[list[0].key].docIdList[list[0].cursor];
		__m128i ee = _mm_set1_epi32(e);

		while (s != num && isFind == true) {
			isFind = false;
			while (list[s].cursor < list[s].length - 3) {// ���s�б�
				// ��������
				//__m128i ii = _mm_loadu_epi32(&index[list[s].key].docIdList[list[s].cursor]);
				__m128i ii = _mm_load_si128((__m128i*) & index[list[s].key].docIdList[list[s].cursor]);
				__m128i result = _mm_set1_epi32(0);
				result = _mm_cmpeq_epi32(ee, ii);
				int mask = _mm_movemask_epi8(result);// �õ�����
				if (mask != 0) {// �鿴�ȽϽ��
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])// ��ʱbreak����������cursor
					break;
				list[s].cursor += 4;
			}
			if (!isFind) {
				while (list[s].cursor < list[s].length) {// ʣ���ֶ�
					if (e == index[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < index[list[s].key].docIdList[list[s].cursor])
						break;
					list[s].cursor++;// ��ǰ���ʹ�����û�ҵ����ʵģ�������
				}
			}
			s++;// ��һ������
		}
		list[0].cursor++;// ��ǰԪ���ѱ����ʹ�
		if (s == num && isFind) 
			S.docIdList.push_back(e);
		//sort(list, list + num);// ���ţ���δ̽��Ԫ���ٵ��б�ǰ��
	}
	return S;
}
