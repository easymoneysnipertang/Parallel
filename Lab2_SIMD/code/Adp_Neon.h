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
//#include"Adp.h"
//#include"InvertedIndex.h"
//using namespace std;
//// adp_noen
//InvertedIndex ADP_NEON(int* queryList, vector<InvertedIndex>& index, int num) {
//	InvertedIndex S;
//	QueryItem* list = new QueryItem[num]();
//	for (int i = 0; i < num; i++)// 预处理
//	{
//		list[i].cursor = 0;
//		list[i].key = queryList[i];
//		list[i].length = index[queryList[i]].length;
//	}
//
//	while (list[0].cursor < list[0].length) {// 最短的列表非空
//		bool isFind = true;
//		int s = 1;
//		unsigned int e = index[list[0].key].docIdList[list[0].cursor];
//		uint32x4_t ss = vmovq_n_u32(e);// 填充四个位置
//
//		// 将长度做成4的倍数，多余位置补0，不用再处理余数
//		int length = ceil(list[s].length / 4) * 4;
//		for (int l = list[s].length; l < length; l++) {// 末尾补0，可以更好的对齐
//			index[list[s].key].docIdList[l] = 0;
//		}
//
//		while (s != num && isFind == true) {
//			isFind = false;
//			while (list[s].cursor < length) {// 检查s列表
//				// 构造向量
//				uint32x4_t ii = vld1q_u32(&index[list[s].key].docIdList[list[s].cursor]);// 读取四个数据
//				uint32x4_t temp = vceqq_u32(ss, ii);
//				//uint32_t mask = vmovemaskq_u32(temp);// 用不了
//				unsigned int result[4] = { 0 };
//				vst1q_u32(result, temp);
//				if (result[0] == 1 || result[1] == 1 || result[2] == 1 || result[3] == 1) {
//					isFind = true;
//					break;
//				}
//				else if (e < index[list[s].key].docIdList[list[s].cursor])// 及时break，避免重置cursor
//					break;
//				list[s].cursor += 4;
//			}
//			s++;// 下一个链表
//		}
//		list[0].cursor++;// 当前元素已被访问过
//		if (s == num && isFind) {
//			S.docIdList.push_back(e);
//			S.length++;
//		}
//		//sort(list, list + num);// 重排，将未探查元素少的列表前移
//	}
//	return S;
//}