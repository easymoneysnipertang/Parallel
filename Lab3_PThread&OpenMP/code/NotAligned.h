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
	InvertedIndex s = index[queryList[0]];// ȡ��̵��б�

	// ��ʣ���б���
	for (int i = 1; i < num; i++) {
		int count = 0;// s��ͷ�������һ��
		int t = 0;
		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for (int j = 0; j < s.docIdList.size(); j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			__m128i ss = _mm_set1_epi32(s.docIdList[j]);// ���32λ

			// ���Բ�����ı���
			// �ڿ�ʼ��ģ4ʣ��Ĳ��ִ�������Ժ�ÿ�μ��ı�֤���ܷ���

			for (; t < 2; t++) {
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��������
					break;
			}

			//cout << &index[queryList[i]].docIdList[0] << endl << endl;
			for (; t < index[queryList[i]].docIdList.size()-3; t += 4) {// ͬʱ���ĸ�Ԫ�رȽ�
				// ����i�б�������Ԫ��
				__m128i ii;
				ii = _mm_loadu_epi32(&index[queryList[i]].docIdList[t]);// һ��ȡ�ĸ�
				__m128i tmp = _mm_set1_epi32(0);
				tmp = _mm_cmpeq_epi32(ss, ii);// �Ƚ�����ÿһλ
				int mask = _mm_movemask_epi8(tmp);// תΪ���룬�����һ�������,�Ͳ�����ȫ0

				if (mask != 0) {// �鿴�ȽϽ��
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��ʱbreak�����ⳬ������һԪ������ͷ����
					break;
			}
			if (!isFind && (t >= index[queryList[i]].docIdList.size() - 3)) {// ����ʣ��Ԫ��
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

// adp_sse_notAlign
InvertedIndex ADP_NotAlign(int* queryList, vector<InvertedIndex>& index, int num) {
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
			// ���Բ�����ı���
			// �ڿ�ʼ��ģ4ʣ��Ĳ��ִ�������Ժ�ÿ�μ��ı�֤���ܷ���
			int tail = list[s].length % 4;
			while (list[s].cursor < tail) {// ʣ���ֶ�
				if (e == index[list[s].key].docIdList[list[s].cursor]) {
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])
					break;
				list[s].cursor++;// ��ǰ���ʹ�����û�ҵ����ʵģ�������
			}

			while (list[s].cursor < list[s].length) {// ���s�б�
				// ��������
				__m128i ii = _mm_loadu_epi32(&index[list[s].key].docIdList[list[s].cursor]);
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

			s++;// ��һ������
		}
		list[0].cursor++;// ��ǰԪ���ѱ����ʹ�
		if (s == num && isFind) 
			S.docIdList.push_back(e);
		//sort(list, list + num);// ���ţ���δ̽��Ԫ���ٵ��б�ǰ��
	}
	return S;
}