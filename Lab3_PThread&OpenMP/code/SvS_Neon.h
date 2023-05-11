//#pragma once
//#include <iostream>
//#include <fstream>
//#include <vector>
//#include <math.h>
//#include <algorithm>
//#include <arm_neon.h>
//#include <stdio.h>
//#include <time.h>
//#include <string>
//#include <sstream>
//#include<stdlib.h>//改为linux所用的计时包
//#include"InvertedIndex.h"
//using namespace std;
//InvertedIndex SVS_NEON(int* queryList, vector<InvertedIndex>& index, int num) {
//	InvertedIndex s = index[queryList[0]];// 取最短的列表
//
//	// 与剩余列表求交
//	for (int i = 1; i < num; i++) {
//		int count = 0;// s从头往后遍历一遍
//		int t = 0;
//		// s列表中的每个元素都拿出来比较
//		for (int j = 0; j < s.length; j++) {// 所有元素都得访问一遍
//			bool isFind = false;// 标志，判断当前count位是否能求交
//
//			// 将长度做成4的倍数，多余位置补0，不用再处理余数
//			int length = ceil(index[queryList[i]].length / 4) * 4;
//			for (int l = index[queryList[i]].length; l < length; l++) {// 末尾补0，可以更好的对齐
//				index[queryList[i]].docIdList[l] = 0;
//			}
//			uint32x4_t ss = vmovq_n_u32(s.docIdList[j]);// 填充四个位置
//
//			for (; t < length; t += 4) {
//				uint32x4_t ii = vld1q_u32(&index[i].docIdList[t]);// 读取四个数据
//				uint32x4_t temp = vceqq_u32(ss, ii);
//				//uint32_t mask = vmovemaskq_u32(temp);// 用不了
//				unsigned int result[4] = { 0 };
//				vst1q_u32(result, temp);
//				if (result[0] == 1 || result[1] == 1 || result[2] == 1 || result[3] == 1) {
//					isFind = true;
//					break;
//				}
//				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 及时break，避免超过，下一元素需重头再来
//					break;
//
//			}
//			if (isFind)// 覆盖
//				s.docIdList[count++] = s.docIdList[j];
//		}
//		if (count < s.length)// 最后才做删除
//			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
//		s.length = count;
//	}
//	return s;
//}
