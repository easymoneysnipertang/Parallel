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
//	for (int i = 0; i < num; i++)// Ԥ����
//	{
//		list[i].cursor = 0;
//		list[i].key = queryList[i];
//		list[i].length = index[queryList[i]].length;
//	}
//
//	while (list[0].cursor < list[0].length) {// ��̵��б�ǿ�
//		bool isFind = true;
//		int s = 1;
//		unsigned int e = index[list[0].key].docIdList[list[0].cursor];
//		uint32x4_t ss = vmovq_n_u32(e);// ����ĸ�λ��
//
//		// ����������4�ı���������λ�ò�0�������ٴ�������
//		int length = ceil(list[s].length / 4) * 4;
//		for (int l = list[s].length; l < length; l++) {// ĩβ��0�����Ը��õĶ���
//			index[list[s].key].docIdList[l] = 0;
//		}
//
//		while (s != num && isFind == true) {
//			isFind = false;
//			while (list[s].cursor < length) {// ���s�б�
//				// ��������
//				uint32x4_t ii = vld1q_u32(&index[list[s].key].docIdList[list[s].cursor]);// ��ȡ�ĸ�����
//				uint32x4_t temp = vceqq_u32(ss, ii);
//				//uint32_t mask = vmovemaskq_u32(temp);// �ò���
//				unsigned int result[4] = { 0 };
//				vst1q_u32(result, temp);
//				if (result[0] == 1 || result[1] == 1 || result[2] == 1 || result[3] == 1) {
//					isFind = true;
//					break;
//				}
//				else if (e < index[list[s].key].docIdList[list[s].cursor])// ��ʱbreak����������cursor
//					break;
//				list[s].cursor += 4;
//			}
//			s++;// ��һ������
//		}
//		list[0].cursor++;// ��ǰԪ���ѱ����ʹ�
//		if (s == num && isFind) {
//			S.docIdList.push_back(e);
//			S.length++;
//		}
//		//sort(list, list + num);// ���ţ���δ̽��Ԫ���ٵ��б�ǰ��
//	}
//	return S;
//}