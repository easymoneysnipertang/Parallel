#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include <emmintrin.h>  //sse2
#include <pmmintrin.h>  //sse3
#include <immintrin.h>
#include"InvertedIndex.h"
#include"Hash.h"
InvertedIndex HASH_SSE(int* queryList, vector<InvertedIndex>& index, int num) {
    InvertedIndex s = index[queryList[0]];// ѡ����̵ĵ����б�
    for (int i = 1; i < num; i++) {// ��ʣ���б���
        // SVS��˼�룬��s����������
        int count = 0;
        int length = s.docIdList.size();

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
                __m128i ss = _mm_set1_epi32(s.docIdList[j]);// ���32λ

                for (begin; begin <= end - 3; begin += 4) {
                    __m128i ii;
                    ii = _mm_loadu_epi32(&index[queryList[i]].docIdList[begin]);// һ��ȡ�ĸ�
                    __m128i tmp = _mm_set1_epi32(0);
                    tmp = _mm_cmpeq_epi32(ss, ii);// �Ƚ�����ÿһλ
                    int mask = _mm_movemask_epi8(tmp);// תΪ���룬�����һ�������,�Ͳ�����ȫ0

                    if (mask != 0) {// �鿴�ȽϽ��
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
    }

    return s;
}