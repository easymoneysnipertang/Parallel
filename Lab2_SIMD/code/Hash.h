#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include"InvertedIndex.h"
// hash�ֶ�ʵ��
struct HashBucket {// һ��hash�Σ���¼���ڵ����б��е���ʼλ��
	int begin = -1;
	int end = -1;
};
HashBucket** hashBucket;
// Ԥ�������������б�����hash����
void preprocessing(vector<InvertedIndex>& index, int count) {
	hashBucket = new HashBucket * [count];
	for (int i = 0; i < count; i++)
		hashBucket[i] = new HashBucket[407000];// 26000000/65536

	for (int i = 0; i < count; i++) {
		for (int j = 0; j < index[i].length; j++) {
			int value = index[i].docIdList[j];// �ó���ǰ�б���j��ֵ
			int hashValue = value/64 ;// TODO:check
			if (hashBucket[i][hashValue].begin == -1) // �ö��ڻ�û��Ԫ��
				hashBucket[i][hashValue].begin = j;
			hashBucket[i][hashValue].end = j;// ���ӵ��ö�β��
		}
	}
}
InvertedIndex HASH(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// ѡ����̵ĵ����б�
	for (int i = 1; i < num; i++) {// ��ʣ���б���
		// SVS��˼�룬��s����������
		int count = 0;
		int length = s.length;

		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j]/64 ;
			// �ҵ���hashֵ�ڵ�ǰ�����б��ж�Ӧ�Ķ�
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// ��ֵ�϶��Ҳ�����������������
				continue;
			}
			else {
				for (begin; begin <= end; begin++) {
					if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
						// �ڸö����ҵ��˵�ǰֵ
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin]) {
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