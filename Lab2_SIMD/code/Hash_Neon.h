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
	InvertedIndex s = index[queryList[0]];// ѡ����̵ĵ����б�
	for (int i = 1; i < num; i++) {// ��ʣ���б���
		// SVS��˼�룬��s����������
		int count = 0;
		int length = s.length;

		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j] / 512;
			// �ҵ���hashֵ�ڵ�ǰ�����б��ж�Ӧ�Ķ�
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// ��ֵ�϶��Ҳ�����������������
				continue;
			}
			else {// �ڸö�����в��ң�һ�β�4��
				uint32x4_t ss = vmovq_n_u32(s.docIdList[j]);// ����ĸ�λ��

				for (begin; begin <= end - 3; begin += 4) {
					uint32x4_t ii = vld1q_u32(&index[i].docIdList[begin]);// ��ȡ�ĸ�����

					uint32x4_t temp = vceqq_u32(ss, ii);
					unsigned int result[4] = { 0 };
					vst1q_u32(result, temp);
					if (result[0] == 1 || result[1] == 1 || result[2] == 1 || result[3] == 1) {
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin])// ��ʱbreak�����ⳬ������һԪ������ͷ����
						break;
				}
				if (!isFind && (begin > end - 3)) {// ����ʣ��Ԫ��
					for (; begin <= end; begin++)
					{
						if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
							isFind = true;
							break;
						}
						else if (s.docIdList[j] < index[queryList[i]].docIdList[begin])// ��������
							break;
					}
				}
				if (isFind) {
					// ����
					s.docIdList[count++] = s.docIdList[j];
				}
			}
		}
		if (count < length)// ������ɾ��
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}

	return s;
}