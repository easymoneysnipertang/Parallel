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
//#include<stdlib.h>//��Ϊlinux���õļ�ʱ��
//#include"InvertedIndex.h"
//using namespace std;
//InvertedIndex SVS_NEON(int* queryList, vector<InvertedIndex>& index, int num) {
//	InvertedIndex s = index[queryList[0]];// ȡ��̵��б�
//
//	// ��ʣ���б���
//	for (int i = 1; i < num; i++) {
//		int count = 0;// s��ͷ�������һ��
//		int t = 0;
//		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
//		for (int j = 0; j < s.length; j++) {// ����Ԫ�ض��÷���һ��
//			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����
//
//			// ����������4�ı���������λ�ò�0�������ٴ�������
//			int length = ceil(index[queryList[i]].length / 4) * 4;
//			for (int l = index[queryList[i]].length; l < length; l++) {// ĩβ��0�����Ը��õĶ���
//				index[queryList[i]].docIdList[l] = 0;
//			}
//			uint32x4_t ss = vmovq_n_u32(s.docIdList[j]);// ����ĸ�λ��
//
//			for (; t < length; t += 4) {
//				uint32x4_t ii = vld1q_u32(&index[i].docIdList[t]);// ��ȡ�ĸ�����
//				uint32x4_t temp = vceqq_u32(ss, ii);
//				//uint32_t mask = vmovemaskq_u32(temp);// �ò���
//				unsigned int result[4] = { 0 };
//				vst1q_u32(result, temp);
//				if (result[0] == 1 || result[1] == 1 || result[2] == 1 || result[3] == 1) {
//					isFind = true;
//					break;
//				}
//				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��ʱbreak�����ⳬ������һԪ������ͷ����
//					break;
//
//			}
//			if (isFind)// ����
//				s.docIdList[count++] = s.docIdList[j];
//		}
//		if (count < s.length)// ������ɾ��
//			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
//		s.length = count;
//	}
//	return s;
//}
