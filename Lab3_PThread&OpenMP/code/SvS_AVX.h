#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include <xmmintrin.h>  //sse
#include <emmintrin.h>  //sse2
#include <pmmintrin.h>  //sse3
#include <immintrin.h>
#include"InvertedIndex.h"

// svs_avx
InvertedIndex SVS_AVX(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// ȡ��̵��б�

	// ��ʣ���б���
	for (int i = 1; i < num; i++) {
		int count = 0;// s��ͷ�������һ��
		int t = 0;
		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for (int j = 0; j < s.docIdList.size(); j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			__m256i ss = _mm256_set1_epi32(s.docIdList[j]);// ���32λ

			for (; t < index[queryList[i]].docIdList.size() - 7; t += 8) {// ͬʱ��˸�Ԫ�رȽ�
				// ����i�б�������Ԫ��
				__m256i ii;
				ii = _mm256_loadu_epi32(&index[queryList[i]].docIdList[t]);// һ��ȡ�˸�
				__m256i tmp = _mm256_set1_epi32(0);
				tmp = _mm256_cmpeq_epi32(ss, ii);// �Ƚ�����ÿһλ
				int mask = _mm256_movemask_epi8(tmp);// תΪ���룬�����һ�������,�Ͳ�����ȫ0

				if (mask != 0) {// �鿴�ȽϽ��
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��ʱbreak�����ⳬ������һԪ������ͷ����
					break;
			}
			if (!isFind && (t >= index[queryList[i]].docIdList.size() - 7)) {// ����ʣ��Ԫ��
				for (; t < index[queryList[i]].docIdList.size(); t++)
				{
					if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��������
						break;
				}
			}
			if (isFind)// ����
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.docIdList.size())// ������ɾ��
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
	}
	return s;
}
