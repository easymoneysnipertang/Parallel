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
    InvertedIndex s = index[queryList[0]];// 选出最短的倒排列表
    for (int i = 1; i < num; i++) {// 与剩余列表求交
        // SVS的思想，将s挨个按表求交
        int count = 0;
        int length = s.docIdList.size();

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
                __m128i ss = _mm_set1_epi32(s.docIdList[j]);// 填充32位

                for (begin; begin <= end - 3; begin += 4) {
                    __m128i ii;
                    ii = _mm_loadu_epi32(&index[queryList[i]].docIdList[begin]);// 一次取四个
                    __m128i tmp = _mm_set1_epi32(0);
                    tmp = _mm_cmpeq_epi32(ss, ii);// 比较向量每一位
                    int mask = _mm_movemask_epi8(tmp);// 转为掩码，如果有一个数相等,就不会是全0

                    if (mask != 0) {// 查看比较结果
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
    }

    return s;
}